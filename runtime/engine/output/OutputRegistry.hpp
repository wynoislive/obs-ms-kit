#pragma once
#include "IOutputSession.hpp"
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <string>

namespace mskit {

class OutputRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<IOutputSession>> sessions;
    mutable std::shared_mutex registry_mutex;

public:
    OutputRegistry() = default;
    ~OutputRegistry() = default;

    bool RegisterSession(const std::string& session_id, std::shared_ptr<IOutputSession> session) {
        std::unique_lock<std::shared_mutex> lock(registry_mutex);
        if (sessions.find(session_id) != sessions.end()) {
            return false;
        }
        sessions[session_id] = session;
        return true;
    }

    bool RemoveSession(const std::string& session_id) {
        std::unique_lock<std::shared_mutex> lock(registry_mutex);
        auto it = sessions.find(session_id);
        if (it == sessions.end()) {
            return false;
        }
        sessions.erase(it);
        return true;
    }

    std::shared_ptr<IOutputSession> FindSession(const std::string& session_id) const {
        std::shared_lock<std::shared_mutex> lock(registry_mutex);
        auto it = sessions.find(session_id);
        if (it == sessions.end()) {
            return nullptr;
        }
        return it->second;
    }
};

} // namespace mskit
