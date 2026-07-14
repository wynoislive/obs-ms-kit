#pragma once

#include "IProtocolClient.hpp"
#include <obs.h>
#include <mutex>
#include <atomic>
#include <string>

namespace mskit::engine {

class SrtClient : public IProtocolClient {
private:
    std::string client_id;
    mutable std::mutex connection_mutex;

    // Low-level libobs output stream context for MPEG-TS / SRT transmission
    obs_output_t* obs_srt_output = nullptr;
    std::atomic<bool> connected{false};

    // Telemetry storage fields for real-time Performance Rule Engine loops
    mutable std::mutex telemetry_mutex;
    mutable int64_t last_rtt_ms = 0;
    mutable double congestion_index = 0.0;

    // Telemetry delta sampling markers
    mutable uint64_t last_ts_bytes = 0;
    mutable uint64_t last_telemetry_time_ns = 0;

public:
    explicit SrtClient(const std::string& id);
    virtual ~SrtClient() override;

    // IProtocolClient Interface Implementation Overrides
    virtual bool Connect(const std::string& url, const std::string& key) override;
    virtual void Disconnect() override;
    virtual bool SendPacket(struct encoder_packet* packet) override;

    virtual bool IsConnected() const override { return connected.load(); }
    virtual void GetNetworkTelemetry(int64_t& out_rtt_ms,
                                     double& out_congestion_factor) const override;
};

} // namespace mskit::engine
