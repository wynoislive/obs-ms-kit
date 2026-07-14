#pragma once

#include <obs-module.h>
#include <obs-encoder.h>
#include <string>

namespace mskit::engine {

class IProtocolClient {
public:
    virtual ~IProtocolClient() = default;

    // Control Plane: Connect/Disconnect endpoint handlers
    virtual bool Connect(const std::string& url, const std::string& key) = 0;
    virtual void Disconnect() = 0;

    // Data Plane: Consumes packed frame payloads directly from the NetworkBuffer
    virtual bool SendPacket(struct encoder_packet* packet) = 0;

    // Diagnostics & State Observers for the Rule Engine
    virtual bool IsConnected() const = 0;
    virtual void GetNetworkTelemetry(int64_t& out_rtt_ms,
                                     double& out_congestion_factor) const = 0;
};

} // namespace mskit::engine
