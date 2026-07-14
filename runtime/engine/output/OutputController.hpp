#pragma once
#include "OutputRegistry.hpp"
#include <memory>
#include <shared_mutex>

namespace mskit {

class OutputController {
private:
    std::shared_ptr<OutputRegistry> registry;
    mutable std::shared_mutex controller_mutex;

public:
    explicit OutputController(std::shared_ptr<OutputRegistry> reg) : registry(reg) {}
    ~OutputController() = default;

    // Master Control Plane Orchestrations
    void StartSession(const std::string& session_id) {
        std::shared_lock<std::shared_mutex> lock(controller_mutex);
        if (auto session = registry->FindSession(session_id)) {
            session->StartPipeline();
        }
    }

    void StopSession(const std::string& session_id) {
        std::shared_lock<std::shared_mutex> lock(controller_mutex);
        if (auto session = registry->FindSession(session_id)) {
            session->StopPipeline();
        }
    }

    void SwapSessionProfile(const std::string& session_id, const OutputProfile& new_profile) {
        std::shared_lock<std::shared_mutex> lock(controller_mutex);
        if (auto session = registry->FindSession(session_id)) {
            session->SwapProfile(new_profile);
        }
    }

    void GlobalTriggerStart() {
        // TBD: Global start across all registered sessions
    }

    void GlobalTriggerStop() {
        // TBD: Global stop across all registered sessions
    }

    // Resource Coordination Hook for the Rule Engine
    void ThrottleSessionPriority(uint32_t threshold_priority) {
        // TBD: Throttling sessions based on priority constraints
    }
};

} // namespace mskit
