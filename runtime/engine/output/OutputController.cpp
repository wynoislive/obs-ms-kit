#include "OutputController.hpp"
#include <obs-module.h>

namespace mskit::engine {

bool OutputController::RegisterSession(const std::string& node_id, std::shared_ptr<IOutputSession> session) {
    if (!session) return false;

    std::unique_lock<std::shared_mutex> lock(registry_mutex);
    if (sessions.find(node_id) != sessions.end()) {
        blog(LOG_WARNING, "[MSK-CORE] Session ID %s already registered.", node_id.c_str());
        return false;
    }

    sessions[node_id] = session;
    blog(LOG_INFO, "[MSK-CORE] Session %s successfully bound to control registry.", node_id.c_str());
    return true;
}

bool OutputController::UnregisterSession(const std::string& node_id) {
    std::shared_ptr<IOutputSession> target_session;
    
    {
        std::unique_lock<std::shared_mutex> lock(registry_mutex);
        auto it = sessions.find(node_id);
        if (it == sessions.end()) return false;
        
        target_session = it->second;
        sessions.erase(it);
    }

    // Stop execution outside of the registry lock window to prevent system deadlock
    if (target_session) {
        target_session->StopPipeline();
    }
    
    blog(LOG_INFO, "[MSK-CORE] Session %s removed from control registry.", node_id.c_str());
    return true;
}

void OutputController::GlobalTriggerStart() {
    std::shared_lock<std::shared_mutex> lock(registry_mutex);
    blog(LOG_INFO, "[MSK-CORE] Executing parallel startup sequence across all nodes.");
    
    for (auto const& [id, session] : sessions) {
        // Isolated Failure Domain: If one node fails initialization, continue starting others
        if (session->GetState() == mskit::SessionState::Stopped) {
            session->StartPipeline();
        }
    }
}

void OutputController::GlobalTriggerStop() {
    std::shared_lock<std::shared_mutex> lock(registry_mutex);
    blog(LOG_INFO, "[MSK-CORE] Executing graceful teardown sequence across all nodes.");
    
    for (auto const& [id, session] : sessions) {
        session->StopPipeline();
    }
}

void OutputController::EnforceResourceThrottle(uint32_t max_allowed_priority) {
    std::shared_lock<std::shared_mutex> lock(registry_mutex);
    blog(LOG_WARNING, "[MSK-RULES] Resource constraints breached. Throttling low-priority sessions.");

    for (auto const& [id, session] : sessions) {
        // High-priority numbers signify low-importance destinations in our budget coordinator
        // If a destination's priority number is higher than allowed, force a controlled fallback
        if (session->GetPriorityLevel() > max_allowed_priority) {
            blog(LOG_INFO, "[MSK-RULES] Throttling stream node: %s", id.c_str());
            // Here, the controller will invoke safe quality adaptation or pauses on the node runtime
        }
    }
}

std::shared_ptr<IOutputSession> OutputController::GetSession(const std::string& node_id) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex);
    auto it = sessions.find(node_id);
    if (it != sessions.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<IOutputSession>> OutputController::GetAllActiveSessions() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex);
    std::vector<std::shared_ptr<IOutputSession>> active_list;
    active_list.reserve(sessions.size());
    
    for (auto const& [id, session] : sessions) {
        active_list.push_back(session);
    }
    return active_list;
}

} // namespace mskit::engine
