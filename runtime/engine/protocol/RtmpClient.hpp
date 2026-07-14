#pragma once

#include "IProtocolClient.hpp"
#include <obs-module.h>
#include <mutex>
#include <atomic>

namespace mskit::engine {

class RtmpClient : public IProtocolClient {
private:
    std::string client_id;
    mutable std::mutex connection_mutex;

    // Native libobs service reference handle
    obs_service_t* obs_rtmp_service = nullptr;
    std::atomic<bool> connected{false};

    // Real-time network telemetries for the Rule Engine tracking loops
    mutable std::mutex telemetry_mutex;
    int64_t last_rtt_ms = 0;
    double congestion_index = 0.0;

public:
    explicit RtmpClient(const std::string& id);
    virtual ~RtmpClient() override;

    // IProtocolClient Interface Overrides
    virtual bool Connect(const std::string& url, const std::string& key) override;
    virtual void Disconnect() override;
    virtual bool SendPacket(struct encoder_packet* packet) override;

    virtual bool IsConnected() const override { return connected.load(); }
    virtual void GetNetworkTelemetry(int64_t& out_rtt_ms,
                                     double& out_congestion_factor) const override;
};

} // namespace mskit::engine
