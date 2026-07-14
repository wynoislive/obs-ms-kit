#pragma once

#include <string>
#include <cstdint>
#include <atomic>
#include <mutex>

namespace mskit::engine {

enum class OutputHealth : uint8_t {
    Healthy,
    Warning,
    Congested,
    Reconnecting,
    Recovering,
    Stopped,
    Error
};

class HealthMonitor {
private:
    std::string node_id;

    // Data Plane Status: Atomic storage flags to guarantee lock-free reads
    std::atomic<OutputHealth> current_health{OutputHealth::Stopped};
    std::atomic<uint32_t> reconnect_attempts{0};

    // Control Plane Status: Isolated string allocations protected by mutex locks
    mutable std::mutex error_mutex;
    std::string last_error_string;

public:
    explicit HealthMonitor(const std::string& id);
    ~HealthMonitor() = default;

    // Control Plane / Data Plane Mutation Mutators
    void UpdateHealthState(OutputHealth new_health);
    void UpdateReconnectCount(uint32_t attempts);
    void RegisterErrorNotification(const std::string& error_msg);
    void ClearActiveErrors();

    // Thread-Safe Status Observers for UI / Rule Engine
    OutputHealth GetHealthState() const { return current_health.load(); }
    uint32_t GetReconnectAttempts() const { return reconnect_attempts.load(); }
    std::string GetLastRegisteredError() const;
    const std::string& GetNodeId() const { return node_id; }
};

} // namespace mskit::engine
