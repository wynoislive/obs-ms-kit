#pragma once
#include "../../kernel/IService.hpp"
#include "../output/OutputController.hpp"
#include "../output/HealthMonitor.hpp"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <string>
#include <atomic>

namespace mskit::engine {

class PerformanceRuleEngine : public mskit::kernel::IService {
private:
    std::weak_ptr<OutputController> controller_hook;
    std::atomic<bool> is_running{false};

    struct ControlState {
        uint32_t stable_ticks = 0;
        uint32_t current_throttled_bitrate = 0;
        uint32_t last_dropped_frames = 0;
    };
    std::unordered_map<std::string, ControlState> state_registry;
    mutable std::mutex state_mutex;

    void ExecuteControlLoopIteration();
    void EvaluateSessionHealth(std::shared_ptr<IOutputSession>& session);

public:
    PerformanceRuleEngine() = default;
    virtual ~PerformanceRuleEngine() override { Stop(); }

    // IService Overrides
    virtual const char* GetServiceName() const override { return "PerformanceRuleEngine"; }

    // Control Plane Lifecycle Hooks
    void Initialize(std::shared_ptr<OutputController> controller);
    void Start();
    void Stop();
};

} // namespace mskit::engine
