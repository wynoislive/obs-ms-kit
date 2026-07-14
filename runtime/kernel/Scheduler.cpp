#include "Scheduler.hpp"
#include <obs-module.h>

namespace mskit::kernel {

Scheduler::Scheduler(size_t thread_count) {
    for (size_t i = 0; i < thread_count; ++i) {
        worker_pool.emplace_back(&Scheduler::WorkerLoop, this);
    }
    blog(LOG_INFO, "[MSK-CORE] Scheduler initialized with %zu worker threads.",
         thread_count);
}

Scheduler::~Scheduler() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop_requested.store(true);
    }
    condition.notify_all();

    for (std::thread& worker : worker_pool) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    blog(LOG_INFO, "[MSK-CORE] Scheduler worker pool joined and destroyed.");
}

void Scheduler::PushTask(TaskPriority priority, std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (stop_requested.load()) return;

        switch (priority) {
            case TaskPriority::Critical: queue_critical.push(std::move(task)); break;
            case TaskPriority::High:     queue_high.push(std::move(task));     break;
            case TaskPriority::Medium:   queue_medium.push(std::move(task));   break;
            case TaskPriority::Low:      queue_low.push(std::move(task));      break;
        }
    }
    condition.notify_one();
}

void Scheduler::ProcessExecutionQueue() {
    // Optional polling entry fallback hook to satisfy external drivers
}

void Scheduler::WorkerLoop() {
    while (true) {
        std::function<void()> target_task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this]() {
                return stop_requested.load() ||
                       !queue_critical.empty() || !queue_high.empty() ||
                       !queue_medium.empty()   || !queue_low.empty();
            });

            if (stop_requested.load() &&
                queue_critical.empty() && queue_high.empty() &&
                queue_medium.empty()   && queue_low.empty()) {
                return;
            }

            // Strict deterministic priority hierarchy (ADR-007 Budgets)
            if (!queue_critical.empty()) {
                target_task = std::move(queue_critical.front());
                queue_critical.pop();
            } else if (!queue_high.empty()) {
                target_task = std::move(queue_high.front());
                queue_high.pop();
            } else if (!queue_medium.empty()) {
                target_task = std::move(queue_medium.front());
                queue_medium.pop();
            } else if (!queue_low.empty()) {
                target_task = std::move(queue_low.front());
                queue_low.pop();
            }
        }

        if (target_task) {
            target_task();
        }
    }
}

} // namespace mskit::kernel
