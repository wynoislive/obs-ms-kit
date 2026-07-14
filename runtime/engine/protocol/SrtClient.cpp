#include "SrtClient.hpp"
#include <util/platform.h>
#include <sstream>

namespace mskit::engine {

SrtClient::SrtClient(const std::string& id)
    : client_id(id), last_telemetry_time_ns(os_gettime_ns()) {}

SrtClient::~SrtClient() {
    Disconnect();
}

bool SrtClient::Connect(const std::string& url, const std::string& key) {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (connected.load()) return true;

    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] Constructing low-latency SRT caller tuning arrays...",
         client_id.c_str());

    // 1. Build an optimized low-latency query parameter URL layout
    //    libobs targets microsecond boundaries for SRT configuration flags (120000 = 120ms)
    std::string finalized_srt_url = url;
    if (finalized_srt_url.find('?') == std::string::npos) {
        std::ostringstream oss;
        oss << finalized_srt_url
            << "?mode=caller"
            << "&latency=120000"       // 120ms low-latency buffering window
            << "&connect_timeout=3000" // 3-second connection handshake timeout
            << "&tlpktdrop=1"          // Drop late packets immediately to protect frame times
            << "&oheadbw=25";          // 25% bandwidth headroom reserved for ARQ retransmissions

        if (!key.empty()) {
            // Secure connection tracking maps passphrase natively via AES keys
            oss << "&passphrase=" << key << "&pbkeylen=32";
        }
        finalized_srt_url = oss.str();
    }

    // 2. Instantiate the isolated streaming output stack via "ffmpeg_mpegts"
    obs_data_t* output_settings = obs_data_create();
    obs_data_set_string(output_settings, "url", finalized_srt_url.c_str());

    obs_srt_output = obs_output_create("ffmpeg_mpegts", (client_id + "_srt_output").c_str(), output_settings, nullptr);
    obs_data_release(output_settings);

    if (!obs_srt_output) {
        blog(LOG_ERROR, "[MSK-PROTOCOL] [%s] Failed to instantiate ffmpeg_mpegts SRT output module.",
             client_id.c_str());
        return false;
    }

    // 3. Initiate native connection capturing loops
    if (!obs_output_start(obs_srt_output)) {
        blog(LOG_ERROR, "[MSK-PROTOCOL] [%s] Low-level SRT socket handshake activation failed.",
             client_id.c_str());
        obs_output_release(obs_srt_output);
        obs_srt_output = nullptr;
        return false;
    }

    connected.store(true);
    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] SRT low-latency transport client cleanly initialized.",
         client_id.c_str());
    return true;
}

void SrtClient::Disconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (!connected.load()) return;

    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] Tearing down SRT transport channel links.",
         client_id.c_str());

    if (obs_srt_output) {
        obs_output_stop(obs_srt_output);
        obs_output_release(obs_srt_output);
        obs_srt_output = nullptr;
    }

    connected.store(false);
}

bool SrtClient::SendPacket(struct encoder_packet* packet) {
    // Media frames are multiplexed and distributed directly through internal libobs output loops
    (void)packet;
    return true;
}

void SrtClient::GetNetworkTelemetry(int64_t& out_rtt_ms, double& out_congestion_factor) const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);

    if (!connected.load() || !obs_srt_output) {
        out_rtt_ms = 0;
        out_congestion_factor = 0.0;
        return;
    }

    uint64_t now_ns = os_gettime_ns();
    uint64_t elapsed_ns = now_ns - last_telemetry_time_ns;

    // Throttle stats extraction loops to 500ms bounds to satisfy performance budgets (ADR-007)
    if (elapsed_ns >= 500000000ULL) {

        // 1. Query raw congestion limits (returns a smooth 0.0f to 1.0f factor)
        float libobs_congestion = obs_output_get_congestion(obs_srt_output);

        // 2. Parse absolute packet drop telemetry vectors from the driver layer
        uint64_t total_bytes = obs_output_get_total_bytes(obs_srt_output);
        uint64_t dropped_frames = obs_output_get_frames_dropped(obs_srt_output);

        // 3. Calculate compound rule indices
        congestion_index = static_cast<double>(libobs_congestion);
        if (dropped_frames > 0) {
            congestion_index += 0.15; // Escalation offset factor for local network blocks
        }
        if (congestion_index > 1.0) congestion_index = 1.0;

        // Derive mathematical latency approximations based on the congestion index
        last_rtt_ms = static_cast<int64_t>(congestion_index * 120.0);

        // Reset tracking delta metrics variables
        last_ts_bytes = total_bytes;
        last_telemetry_time_ns = now_ns;
    }

    out_rtt_ms = last_rtt_ms;
    out_congestion_factor = congestion_index;
}

} // namespace mskit::engine
