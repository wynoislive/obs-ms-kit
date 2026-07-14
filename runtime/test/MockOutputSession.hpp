#pragma once
#include "../engine/output/IOutputSession.hpp"
#include "../engine/output/HealthMonitor.hpp"
#include <mutex>

namespace mskit::test {

class MockOutputSession : public mskit::IOutputSession {
private:
    std::string node_id;
    mskit::OutputProfile profile;
    bool active = false;

    // Direct injection hooks for testing
    uint32_t simulated_bitrate = 0;
    double simulated_fps = 60.0;
    uint32_t simulated_dropped_frames = 0;
    int64_t simulated_rtt_ms = 0;
    double simulated_congestion_factor = 0.0;

    std::shared_ptr<mskit::engine::HealthMonitor> health_monitor;

public:
    MockOutputSession(const std::string& id, const mskit::OutputProfile& initial_profile)
        : node_id(id), profile(initial_profile), simulated_bitrate(initial_profile.video_bitrate) {
        health_monitor = std::make_shared<mskit::engine::HealthMonitor>(id);
    }

    // Direct telemetry injection controls
    void InjectNetworkTelemetry(int64_t rtt, double congestion) {
        simulated_rtt_ms = rtt;
        simulated_congestion_factor = congestion;
    }

    void InjectLiveTelemetry(uint32_t bitrate, double fps, uint32_t dropped) {
        simulated_bitrate = bitrate;
        simulated_fps = fps;
        simulated_dropped_frames = dropped;
    }

    void InjectHealthState(mskit::engine::OutputHealth state) {
        health_monitor->UpdateHealthState(state);
    }

    // IOutputSession Interface Implementation
    virtual const std::string& GetSessionId() const override { return node_id; }
    virtual bool IsActive() const override { return active; }
    virtual void Open() override { active = true; }
    virtual void Close() override { active = false; }
    virtual void StartPipeline() override { Open(); }
    virtual void StopPipeline() override { Close(); }

    virtual bool Initialize(const mskit::OutputProfile& initial_profile) override {
        profile = initial_profile;
        return true;
    }

    virtual void SwapProfile(const mskit::OutputProfile& new_profile) override { profile = new_profile; }
    virtual mskit::OutputProfile GetProfile() const override { return profile; }
    virtual uint32_t GetPriorityLevel() const override { return 1; }

    virtual bool UpdateBitrate(uint32_t new_bitrate) override {
        simulated_bitrate = new_bitrate;
        return true;
    }

    virtual void GetLiveTelemetry(uint32_t& out_bitrate, double& out_fps, uint32_t& out_dropped) const override {
        out_bitrate = simulated_bitrate;
        out_fps = simulated_fps;
        out_dropped = simulated_dropped_frames;
    }

    virtual void GetNetworkTelemetry(int64_t& out_rtt, double& out_congestion) const override {
        out_rtt = simulated_rtt_ms;
        out_congestion = simulated_congestion_factor;
    }

    virtual std::shared_ptr<mskit::engine::HealthMonitor> GetHealthMonitor() const override {
        return health_monitor;
    }

    virtual mskit::SessionState GetState() const override {
        return active ? mskit::SessionState::Streaming : mskit::SessionState::Stopped;
    }

    virtual mskit::OutputHealth GetHealth() const override {
        return static_cast<mskit::OutputHealth>(health_monitor->GetHealthState());
    }

    virtual mskit::OutputRuntimeState GetRuntimeState() const override {
        mskit::OutputRuntimeState state;
        state.state = GetState();
        state.health = GetHealth();
        state.current_bitrate = simulated_bitrate;
        state.output_fps = simulated_fps;
        state.dropped_frames = simulated_dropped_frames;
        state.rtt_ms = static_cast<uint32_t>(simulated_rtt_ms);
        return state;
    }

    virtual void ProcessVideoFrame(obs_source_t* source) override {
        (void)source;
    }
};

} // namespace mskit::test
