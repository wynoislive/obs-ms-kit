#include "RtmpClient.hpp"
#include <obs-module.h>

namespace mskit::engine {

RtmpClient::RtmpClient(const std::string& id) : client_id(id) {}

RtmpClient::~RtmpClient() {
    Disconnect();
}

bool RtmpClient::Connect(const std::string& url, const std::string& key) {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (connected.load()) return true;

    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] Connecting protocol layer to server endpoints...",
         client_id.c_str());

    // 1. Instantiate the isolated RTMP service container via libobs registry hooks
    obs_data_t* service_settings = obs_data_create();
    obs_data_set_string(service_settings, "server", url.c_str());
    obs_data_set_string(service_settings, "key", key.c_str());

    obs_rtmp_service = obs_service_create(
        "rtmp_custom",
        (client_id + "_rtmp_service").c_str(),
        service_settings,
        nullptr);

    obs_data_release(service_settings);

    if (!obs_rtmp_service) {
        blog(LOG_ERROR,
             "[MSK-PROTOCOL] [%s] Failed to initialize native RTMP service factory context.",
             client_id.c_str());
        return false;
    }

    connected.store(true);
    blog(LOG_INFO, "[MSK-PROTOCOL] [%s] RTMP service context created successfully.",
         client_id.c_str());
    return true;
}

void RtmpClient::Disconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (!connected.load()) return;

    blog(LOG_INFO,
         "[MSK-PROTOCOL] [%s] Tearing down connection loops and closing network sockets.",
         client_id.c_str());

    if (obs_rtmp_service) {
        obs_service_release(obs_rtmp_service);
        obs_rtmp_service = nullptr;
    }

    connected.store(false);
}

bool RtmpClient::SendPacket(struct encoder_packet* packet) {
    std::lock_guard<std::mutex> lock(connection_mutex);
    if (!connected.load() || !obs_rtmp_service) return false;

    // Data Plane: Pipeline flush out to the physical streaming socket boundary.
    // In the standard OBS pipeline, libobs internally routes packets from the
    // output to the service via obs_output_set_service(). This method serves
    // as a hook point for custom packet injection in future milestone stages
    // when we fully decouple from libobs's internal output routing.

    (void)packet;
    return true;
}

void RtmpClient::GetNetworkTelemetry(int64_t& out_rtt_ms,
                                      double& out_congestion_factor) const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    out_rtt_ms = last_rtt_ms;
    out_congestion_factor = congestion_index;
}

} // namespace mskit::engine
