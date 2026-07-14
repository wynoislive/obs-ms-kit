#pragma once
#include "IOutputSession.hpp"
#include "../config/OutputProfile.hpp"
#include "OutputRuntimeState.hpp"
#include <mutex>
#include <atomic>

namespace mskit::engine {

class OutputSession : public IOutputSession {
private:
    std::string session_id;
    
    // Control Plane Storage: Protected by safe configuration swaps
    std::mutex config_mutex;
    OutputProfile current_profile;
    
    // Data Plane Status: Atomic tracking to prevent thread polling stalls
    std::atomic<mskit::SessionState> current_state{mskit::SessionState::Stopped};
    std::atomic<mskit::OutputHealth> current_health{mskit::OutputHealth::Stopped};
    
    // Runtime Metric Accumulators
    mutable std::mutex telemetry_mutex;
    OutputRuntimeState runtime_metrics;

    // Internal State Machine Driver Transitions
    bool TransitionTo(mskit::SessionState target_state);
    void HandleStateAction(mskit::SessionState state);

    // Data-plane pipeline components
    std::unique_ptr<class IScaler> scaler;
    std::unique_ptr<class IEncoderInstance> encoder;
    std::shared_ptr<class NetworkBuffer> network_buffer;
    std::shared_ptr<class IProtocolClient> protocol_client;
    std::unique_ptr<class ReconnectPolicy> reconnect_policy;
    std::shared_ptr<class HealthMonitor> health_monitor;

    std::mutex encoder_mutex;
    bool is_initialized = false;

public:
    OutputSession(const std::string& id, const OutputProfile& initial_profile);
    virtual ~OutputSession() override;

    // IOutputSession Lifecycle Overrides
    virtual bool Initialize(const OutputProfile& profile) override;
    virtual void StartPipeline() override;
    virtual void StopPipeline() override;
    virtual void SwapProfile(const OutputProfile& new_profile) override;

    // Thread-Safe Status Inspectors
    virtual mskit::SessionState GetState() const override { return current_state.load(); }
    virtual mskit::OutputHealth GetHealth() const override { return current_health.load(); }
    virtual mskit::OutputRuntimeState GetRuntimeState() const override;
    virtual const std::string& GetSessionId() const override { return session_id; }
    virtual uint32_t GetPriorityLevel() const override;
    virtual OutputProfile GetProfile() const override;

    virtual bool IsActive() const override;
    virtual void GetLiveTelemetry(uint32_t& out_bitrate, double& out_fps, uint32_t& out_dropped) const override;
    virtual void GetNetworkTelemetry(int64_t& out_rtt_ms, double& out_congestion_factor) const override;
    virtual std::shared_ptr<mskit::engine::HealthMonitor> GetHealthMonitor() const override;
    virtual bool UpdateBitrate(uint32_t new_bitrate_kbps) override;

    // Data Plane entry points
    virtual void ProcessVideoFrame(obs_source_t* source) override;
    void PipelineToBuffer(struct encoder_packet* packet);
};

} // namespace mskit::engine
