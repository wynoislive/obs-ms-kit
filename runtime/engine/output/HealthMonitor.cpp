#include "HealthMonitor.hpp"
#include <obs-module.h>

namespace mskit::engine {

HealthMonitor::HealthMonitor(const std::string& id) : node_id(id) {}

void HealthMonitor::UpdateHealthState(OutputHealth new_health) {
    OutputHealth old_health = current_health.exchange(new_health);

    if (old_health != new_health) {
        blog(LOG_INFO, "[MSK-OUTPUT] [%s] Health Monitor Flag Mutation: %d -> %d",
             node_id.c_str(),
             static_cast<int>(old_health),
             static_cast<int>(new_health));

        // Note: In an upcoming milestone, this state shift hook will publish
        // an alert directly to the lightweight Kernel Event Bus
    }
}

void HealthMonitor::UpdateReconnectCount(uint32_t attempts) {
    reconnect_attempts.store(attempts);
}

void HealthMonitor::RegisterErrorNotification(const std::string& error_msg) {
    std::lock_guard<std::mutex> lock(error_mutex);
    last_error_string = error_msg;

    blog(LOG_ERROR, "[MSK-OUTPUT] [%s] Critical Notification Registered: %s",
         node_id.c_str(), error_msg.c_str());
}

void HealthMonitor::ClearActiveErrors() {
    std::lock_guard<std::mutex> lock(error_mutex);
    last_error_string.clear();
}

std::string HealthMonitor::GetLastRegisteredError() const {
    std::lock_guard<std::mutex> lock(error_mutex);
    return last_error_string;
}

} // namespace mskit::engine
