#pragma once
#include "IScheduler.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>

namespace mskit::kernel {

class Scheduler : public IScheduler {
private:
    // Partitioned priority buckets to avoid internal sorting overhead
    std::queue<std::function<void()>> queue_critical;
    std::queue<std::function<void()>> queue_high;
    std::queue<std::function<void()>> queue_medium;
    std::queue<std::function<void()>> queue_low;

    mutable std::mutex queue_mutex;
    std::condition_variable condition;

    std::vector<std::thread> worker_pool;
    std::atomic<bool> stop_requested{false};

    void WorkerLoop();

public:
    explicit Scheduler(size_t thread_count = 2);
    virtual ~Scheduler() override;

    // IService / IScheduler Interface Overrides
    virtual const char* GetServiceName() const override { return "Scheduler"; }
    virtual void PushTask(TaskPriority priority, std::function<void()> task) override;
    virtual void ProcessExecutionQueue() override;
};

} // namespace mskit::kernel
