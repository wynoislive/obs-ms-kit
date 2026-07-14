#include "EncoderInstance.hpp"
#include <obs-module.h>

namespace mskit::engine {

// ─────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────

EncoderInstance::EncoderInstance(const std::string& id)
    : instance_id(id) {}

EncoderInstance::~EncoderInstance() {
    Close();
}

// ─────────────────────────────────────────────────────────────
// Private: Bind a native libobs encoder context
// ─────────────────────────────────────────────────────────────

bool EncoderInstance::BindObsEncoder(const std::string& encoder_id, const OutputProfile& profile) {
    // 1. Create a clean settings container from the immutable OutputProfile
    obs_data_t* settings = obs_data_create();

    obs_data_set_int(settings, "bitrate", profile.video_bitrate);
    obs_data_set_int(settings, "width", profile.target_width);
    obs_data_set_int(settings, "height", profile.target_height);
    obs_data_set_string(settings, "rate_control", "CBR");
    obs_data_set_int(settings, "keyint_sec", 2);

    // Map hardware-specific presets
    if (profile.encoder_preset == "NVENC") {
        obs_data_set_string(settings, "preset", "p4");
        obs_data_set_string(settings, "profile", "high");
    } else if (profile.encoder_preset == "QSV") {
        obs_data_set_string(settings, "target_usage", "balanced");
        obs_data_set_string(settings, "profile", "high");
    } else if (profile.encoder_preset == "AMF") {
        obs_data_set_string(settings, "preset", "quality");
        obs_data_set_string(settings, "profile", "high");
    } else {
        // x264 software fallback
        obs_data_set_string(settings, "preset", "veryfast");
        obs_data_set_string(settings, "profile", "high");
    }

    // 2. Instantiate a fully distinct encoder context inside libobs
    //    This prevents thread cross-talk or shared failure leaks between streams
    obs_video_encoder = obs_video_encoder_create(
        encoder_id.c_str(),
        (instance_id + "_video_encoder").c_str(),
        settings,
        nullptr);

    obs_data_release(settings);

    if (!obs_video_encoder) {
        blog(LOG_ERROR, "[MSK-ENCODER] [%s] Failed to create native libobs encoder type: %s",
             instance_id.c_str(), encoder_id.c_str());
        return false;
    }

    // Connect the encoder to the global video core mix loop handler
    obs_encoder_set_video(obs_video_encoder, obs_get_video());

    return true;
}

// ─────────────────────────────────────────────────────────────
// Control Plane: Open / Close / UpdateBitrate
// ─────────────────────────────────────────────────────────────

bool EncoderInstance::Open(const OutputProfile& profile) {
    std::lock_guard<std::mutex> lock(encoder_mutex);
    if (is_initialized) return false;

    // Dynamically pick hardware runtime hooks based on creator config
    std::string target_encoder_id = "obs_x264"; // Default fallback CPU software loop

#if defined(_WIN32)
    // --- Windows Hardware Selection Matrix ---
    if (profile.encoder_preset == "NVENC") {
        target_encoder_id = "jim_nvenc";
    } else if (profile.encoder_preset == "QSV") {
        target_encoder_id = "obs_qsv11";
    } else if (profile.encoder_preset == "AMF") {
        target_encoder_id = "amd_amf_h264";
    }

#elif defined(__APPLE__)
    // --- macOS Hardware Selection Matrix (Apple Silicon / Intel VT) ---
    // VideoToolbox handles hardware-accelerated H.264 & HEVC directly on Apple SOCs
    if (profile.encoder_preset == "NVENC" || profile.encoder_preset == "QSV" || profile.encoder_preset == "AMF") {
        blog(LOG_WARNING, "[MSK-ENCODER] Windows specific HW codec requested on macOS. Mapping to Apple VideoToolbox.");
        target_encoder_id = "vt_h264"; 
    } else if (profile.encoder_preset == "VideoToolbox") {
        target_encoder_id = "vt_h264";
    }

#elif defined(__linux__)
    // --- Linux Hardware Selection Matrix (VAAPI / FFmpeg) ---
    if (profile.encoder_preset == "NVENC") {
        target_encoder_id = "jim_nvenc"; // NVENC is fully supported on Linux with proprietary drivers
    } else if (profile.encoder_preset == "QSV" || profile.encoder_preset == "AMF") {
        blog(LOG_WARNING, "[MSK-ENCODER] Mapping hardware codec to Linux VAAPI.");
        target_encoder_id = "obs_vaapi_h264";
    }
#endif

    if (!BindObsEncoder(target_encoder_id, profile)) {
        blog(LOG_ERROR, "[MSK-ENCODER] [%s] Failed to create native libobs encoder type: %s. Attempting x264 software fallback.",
             instance_id.c_str(), target_encoder_id.c_str());
        if (!BindObsEncoder("obs_x264", profile)) {
            return false;
        }
    }

    is_initialized = true;
    blog(LOG_INFO, "[MSK-ENCODER] [%s] Encoder open sequence completed successfully (%s).",
         instance_id.c_str(), target_encoder_id.c_str());
    return true;
}

void EncoderInstance::Close() {
    std::lock_guard<std::mutex> lock(encoder_mutex);
    if (!is_initialized) return;

    // Release resources back to libobs cleanly
    obs_encoder_release(obs_video_encoder);
    obs_video_encoder = nullptr;

    is_initialized = false;
    blog(LOG_INFO, "[MSK-ENCODER] [%s] Encoder closed and memory contexts released.",
         instance_id.c_str());
}

bool EncoderInstance::UpdateBitrate(uint32_t new_bitrate_kbps) {
    std::lock_guard<std::mutex> lock(encoder_mutex);
    if (!is_initialized || !obs_video_encoder) return false;

    // ADR-009: Hot-swapping parameters via controlled configuration blocks safely
    obs_data_t* settings = obs_encoder_get_settings(obs_video_encoder);
    obs_data_set_int(settings, "bitrate", new_bitrate_kbps);
    obs_encoder_update(obs_video_encoder, settings);
    obs_data_release(settings);

    blog(LOG_INFO, "[MSK-CONFIG] [%s] Hot bitrate modification applied to active encoder: %u kbps",
         instance_id.c_str(), new_bitrate_kbps);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Data Plane: Frame Ingestion
// ─────────────────────────────────────────────────────────────

bool EncoderInstance::EncodeVideoFrame(const std::vector<uint8_t>& frame_bytes,
                                       uint32_t linesize,
                                       uint64_t timestamp_ns) {
    std::lock_guard<std::mutex> lock(encoder_mutex);
    if (!is_initialized || !obs_video_encoder) return false;

    // Intercepts packed row byte buffers directly from the double-buffered Scaler.
    // In the standard OBS pipeline, libobs internally feeds video frames from the
    // video output to the encoder via obs_encoder_set_video(). This method serves
    // as a hook point for custom frame injection paths in future milestone stages
    // (e.g., when we decouple fully from libobs's internal video_output routing).

    (void)frame_bytes;
    (void)linesize;
    (void)timestamp_ns;
    return true;
}

// ─────────────────────────────────────────────────────────────
// Observer: Codec Identification
// ─────────────────────────────────────────────────────────────

const char* EncoderInstance::GetEncoderType() const {
    std::lock_guard<std::mutex> lock(encoder_mutex);
    if (!obs_video_encoder) return "inactive";
    return obs_encoder_get_codec(obs_video_encoder);
}

} // namespace mskit::engine
