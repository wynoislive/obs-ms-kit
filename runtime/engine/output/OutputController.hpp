#pragma once
#include "IOutputNode.hpp"
#include <vector>
#include <unordered_map>
#include <shared_mutex>

namespace mskit {

class OutputController {
private:
    std::unordered_map<std::string, std::shared_ptr<IOutputNode>> active_nodes;
    mutable std::shared_mutex controller_mutex;

public:
    OutputController() = default;
    ~OutputController() = default;

    // Master Control Plane Methods
    bool RegisterDestination(const OutputNodeConfig& config);
    bool RemoveDestination(const std::string& node_id);
    
    void GlobalTriggerStart();
    void GlobalTriggerStop();
    
    // Resource Coordination Hook for the Rule Engine
    void ThrottleNodePriority(uint32_t threshold_priority);
    
    std::shared_ptr<IOutputNode> FindNode(const std::string& node_id) const;
};

} // namespace mskit
