#pragma once
#include "../config/OutputConfig.hpp"
#include <memory>

namespace mskit {

class IOutputNode {
public:
    virtual ~IOutputNode() = default;

    // Control Plane: Explicit State Control Lifecycle Hooks
    virtual bool Initialize(const OutputNodeConfig& config) = 0;
    virtual void StartPipeline() = 0;
    virtual void StopPipeline() = 0;
    
    // ADR-009: Atomically update properties via Configuration Swapping
    virtual void SwapConfiguration(const OutputNodeConfig& new_config) = 0;

    // Read-Only Status Expositions for UI & Monitoring Docks
    virtual NodeState GetCurrentState() const = 0;
    virtual void GetLiveTelemetry(uint32_t& out_bitrate, double& out_fps, uint64_t& out_dropped) const = 0;
    virtual const std::string& GetNodeId() const = 0;
};

} // namespace mskit
