#pragma once
#include "IService.hpp"
#include <functional>

namespace mskit::kernel {

enum class TaskPriority : uint8_t {
    Critical, // Encoder, Network Flush, Frame Distribution
    High,     // Reconnect Timers, Buffers
    Medium,   // Telemetry sweeps, Diagnostics
    Low       // UI updates, Log writes
};

class IScheduler : public IService {
public:
    virtual ~IScheduler() = default;
    virtual void PushTask(TaskPriority priority, std::function<void()> task) = 0;
    virtual void ProcessExecutionQueue() = 0;
};

} // namespace mskit::kernel
