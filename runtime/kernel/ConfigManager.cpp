#include "ConfigManager.hpp"
#include "../engine/config/OutputProfile.hpp"
#include <util/platform.h>
#include <obs.h>

namespace mskit::kernel {

void ConfigManager::Initialize(std::shared_ptr<mskit::engine::OutputController> controller) {
    std::lock_guard<std::mutex> lock(config_mutex);
    controller_hook = controller;
    blog(LOG_INFO, "[MSK-CONFIG] ConfigManager bound successfully to OutputController context.");
}

bool ConfigManager::ValidateAndMigrate(obs_data_t* raw_data) const {
    if (!raw_data) return false;

    // Strict Schema Enforcement (ADR-009)
    if (!obs_data_has_user_value(raw_data, "schema_version")) {
        blog(LOG_WARNING, "[MSK-CONFIG] Missing schema version tag. Assuming legacy v0 configuration format.");
        obs_data_set_int(raw_data, "schema_version", 1);
    }

    uint32_t profile_version = static_cast<uint32_t>(obs_data_get_int(raw_data, "schema_version"));
    if (profile_version > kCurrentSchemaVersion) {
        blog(LOG_ERROR, "[MSK-CONFIG] Profile version (%u) exceeds supported runtime schema version (%u).",
             profile_version, kCurrentSchemaVersion);
        return false;
    }

    // Provision fallback array bounds if user parameters are missing or corrupted
    obs_data_set_default_int(raw_data, "video_bitrate", 4500);
    obs_data_set_default_int(raw_data, "target_width", 1920);
    obs_data_set_default_int(raw_data, "target_height", 1080);
    obs_data_set_default_string(raw_data, "encoder_preset", "NVENC");

    return true;
}

bool ConfigManager::ImportProfile(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(config_mutex);
    auto controller = controller_hook.lock();
    if (!controller) {
        blog(LOG_ERROR, "[MSK-CONFIG] Cannot import profile: OutputController context is dead.");
        return false;
    }

    // Read raw file entries directly into libobs data containers
    obs_data_t* imported_json = obs_data_create_from_json_file(file_path.c_str());
    if (!imported_json) {
        blog(LOG_ERROR, "[MSK-CONFIG] Failed to parse or open configuration file: %s", file_path.c_str());
        return false;
    }

    if (!ValidateAndMigrate(imported_json)) {
        obs_data_release(imported_json);
        return false;
    }

    // Generate safe immutable OutputProfile layout arrays from validated JSON fields
    obs_data_array_t* destinations = obs_data_get_array(imported_json, "destinations");
    if (destinations) {
        size_t count = obs_data_array_count(destinations);
        blog(LOG_INFO, "[MSK-CONFIG] Parsing %zu destination profiles from JSON layout tree.", count);

        for (size_t i = 0; i < count; ++i) {
            obs_data_t* node_data = obs_data_array_item(destinations, i);

            std::string node_id = obs_data_get_string(node_data, "node_id");

            mskit::OutputProfile parsed_profile;
            parsed_profile.video_bitrate = static_cast<uint32_t>(obs_data_get_int(node_data, "video_bitrate"));
            parsed_profile.target_width  = static_cast<uint32_t>(obs_data_get_int(node_data, "target_width"));
            parsed_profile.target_height = static_cast<uint32_t>(obs_data_get_int(node_data, "target_height"));
            parsed_profile.encoder_preset = obs_data_get_string(node_data, "encoder_preset");

            // Look up the active session to perform a clean, thread-safe configuration swap
            auto active_session = controller->GetSession(node_id);
            if (active_session) {
                active_session->SwapProfile(parsed_profile);
            } else {
                blog(LOG_WARNING, "[MSK-CONFIG] Found config specs for inactive target node: %s", node_id.c_str());
            }

            obs_data_release(node_data);
        }
        obs_data_array_release(destinations);
    }

    obs_data_release(imported_json);
    return true;
}

bool ConfigManager::ExportProfile(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(config_mutex);
    auto controller = controller_hook.lock();
    if (!controller) return false;

    obs_data_t* root_export = obs_data_create();
    obs_data_set_int(root_export, "schema_version", kCurrentSchemaVersion);

    obs_data_array_t* destinations_array = obs_data_array_create();
    std::vector<std::shared_ptr<mskit::IOutputSession>> active_sessions = controller->GetAllActiveSessions();

    for (const auto& session : active_sessions) {
        obs_data_t* session_node = obs_data_create();

        // Query configurations directly from the active runtime snapshots
        auto profile = session->GetProfile();

        obs_data_set_string(session_node, "node_id", session->GetSessionId().c_str());
        obs_data_set_int(session_node, "video_bitrate", profile.video_bitrate);
        obs_data_set_int(session_node, "target_width", profile.target_width);
        obs_data_set_int(session_node, "target_height", profile.target_height);
        obs_data_set_string(session_node, "encoder_preset", profile.encoder_preset.c_str());

        obs_data_array_push_back(destinations_array, session_node);
        obs_data_release(session_node);
    }

    obs_data_set_array(root_export, "destinations", destinations_array);
    obs_data_array_release(destinations_array);

    // Atomic disk-flush using native libobs primitives to avoid file truncation risks
    bool save_success = obs_data_save_json(root_export, file_path.c_str());
    obs_data_release(root_export);

    if (save_success) {
        blog(LOG_INFO, "[MSK-CONFIG] Runtime configuration profile exported cleanly to: %s", file_path.c_str());
    } else {
        blog(LOG_ERROR, "[MSK-CONFIG] Disk block failure encountered during serialization export pass.");
    }

    return save_success;
}

} // namespace mskit::kernel
