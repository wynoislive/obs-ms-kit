#pragma once

#include "IOutputSession.hpp"
#include "../config/OutputProfile.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <string>
#include <vector>

namespace mskit::engine {

class OutputController {
private:
    // Core session registry bucket
    std::unordered_map<std::string, std::shared_ptr<IOutputSession>> sessions;
    
    // Shared mutex to optimize data-plane read-heavy lookups vs control-plane writes
    mutable std::shared_mutex registry_mutex;

public:
    OutputController() = default;
    ~OutputController() { GlobalTriggerStop(); }

    // Control Plane: Node Lifecycle Management
    bool RegisterSession(const std::string& node_id, std::shared_ptr<IOutputSession> session);
    bool UnregisterSession(const std::string& node_id);

    void GlobalTriggerStart();
    void GlobalTriggerStop();

    // Resource Coordination: Invoked by the Performance Rule Engine
    void EnforceResourceThrottle(uint32_t max_allowed_priority);

    // Thread-Safe Accessors
    std::shared_ptr<IOutputSession> GetSession(const std::string& node_id) const;
    std::vector<std::shared_ptr<IOutputSession>> GetAllActiveSessions() const;
};

} // namespace mskit::engine
