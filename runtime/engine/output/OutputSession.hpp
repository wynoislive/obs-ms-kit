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
};

} // namespace mskit::engine
