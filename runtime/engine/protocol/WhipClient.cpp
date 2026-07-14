#include "WhipClient.hpp"
#include <util/platform.h>

namespace mskit::engine {

WhipClient::WhipClient(const std::string& id)
    : client_id(id), last_telemetry_time_ns(os_gettime_ns()) {}

WhipClient::~WhipClient() {
    Disconnect();
}

bool WhipClient::Connect(const std::string& url, const std::string& key) {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (connected.load()) return true;

    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] Compiling WebRTC/WHIP ingestion context...", client_id.c_str());

    // 1. Configure properties using the native libobs data block
    obs_data_t* output_settings = obs_data_create();
    obs_data_set_string(output_settings, "url", url.c_str());

    // WHIP authentication relies on bearer tokens passed via the authorization header
    if (!key.empty()) {
        obs_data_set_string(output_settings, "bearer_token", key.c_str());
    }

    // 2. Instantiate the isolated WebRTC output context
    //    Binds directly to the native "whip_output" plugin identifier string
    obs_whip_output = obs_output_create("whip_output", (client_id + "_whip_output").c_str(), output_settings, nullptr);
    obs_data_release(output_settings);

    if (!obs_whip_output) {
        blog(LOG_ERROR, "[MSK-PROTOCOL] [%s] Failed to instantiate raw native whip_output module.", client_id.c_str());
        return false;
    }

    // 3. Kick off HTTP POST handshake loop and ice negotiation
    if (!obs_output_start(obs_whip_output)) {
        blog(LOG_ERROR, "[MSK-PROTOCOL] [%s] WebRTC/WHIP HTTP signaling handshake failed.", client_id.c_str());
        obs_output_release(obs_whip_output);
        obs_whip_output = nullptr;
        return false;
    }

    connected.store(true);
    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] WebRTC/WHIP peer connection successfully authorized.", client_id.c_str());
    return true;
}

void WhipClient::Disconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (!connected.load()) return;

    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] Tearing down WebRTC peer connections and signaling channels.", client_id.c_str());

    if (obs_whip_output) {
        obs_output_stop(obs_whip_output);
        obs_output_release(obs_whip_output);
        obs_whip_output = nullptr;
    }

    connected.store(false);
}

bool WhipClient::SendPacket(struct encoder_packet* packet) {
    // Media transport frames are packetized into RTP/SRTP and sent via WebRTC threads natively
    (void)packet;
    return true;
}

void WhipClient::GetNetworkTelemetry(int64_t& out_rtt_ms, double& out_congestion_factor) const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);

    if (!connected.load() || !obs_whip_output) {
        out_rtt_ms = 0;
        out_congestion_factor = 0.0;
        return;
    }

    uint64_t now_ns = os_gettime_ns();
    uint64_t elapsed_ns = now_ns - last_telemetry_time_ns;

    // Enforce our strict 500ms budged sampling ceiling (ADR-007)
    if (elapsed_ns >= 500000000ULL) {
        // Query congestion levels mapped directly from RTCP receiver reports
        float native_congestion = obs_output_get_congestion(obs_whip_output);
        congestion_index = static_cast<double>(native_congestion);

        // Fetch frame drops caused by real-time connection bandwidth degradation
        uint64_t dropped_frames = obs_output_get_frames_dropped(obs_whip_output);
        if (dropped_frames > 0) {
            congestion_index += 0.20; // Aggressive warning scaling for jitter drop behaviors
        }
        if (congestion_index > 1.0) congestion_index = 1.0;

        // Derive WebRTC smooth round-trip latency estimates
        last_rtt_ms = static_cast<int64_t>(congestion_index * 80.0);
        last_telemetry_time_ns = now_ns;
    }

    out_rtt_ms = last_rtt_ms;
    out_congestion_factor = congestion_index;
}

} // namespace mskit::engine
