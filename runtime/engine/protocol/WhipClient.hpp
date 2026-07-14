#pragma once

#include "IProtocolClient.hpp"
#include <obs.h>
#include <mutex>
#include <atomic>
#include <string>

namespace mskit::engine {

class WhipClient : public IProtocolClient {
private:
    std::string client_id;
    mutable std::mutex connection_mutex;

    // Native libobs WebRTC output stream instance handle
    obs_output_t* obs_whip_output = nullptr;
    std::atomic<bool> connected{false};

    // Telemetry registers for the asynchronous Rule Engine evaluation loops
    mutable std::mutex telemetry_mutex;
    mutable int64_t last_rtt_ms = 0;
    mutable double congestion_index = 0.0;

    mutable uint64_t last_telemetry_time_ns = 0;

public:
    explicit WhipClient(const std::string& id);
    virtual ~WhipClient() override;

    // IProtocolClient Interface Implementation Overrides
    virtual bool Connect(const std::string& url, const std::string& key) override;
    virtual void Disconnect() override;
    virtual bool SendPacket(struct encoder_packet* packet) override;

    virtual bool IsConnected() const override { return connected.load(); }
    virtual void GetNetworkTelemetry(int64_t& out_rtt_ms,
                                     double& out_congestion_factor) const override;
};

} // namespace mskit::engine
