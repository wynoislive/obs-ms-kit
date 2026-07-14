#pragma once
#include "../config/OutputProfile.hpp"
#include "OutputRuntimeState.hpp"
#include <obs-module.h>
#include <string>

namespace mskit {

class IOutputSession {
public:
    virtual ~IOutputSession() = default;

    // Control Plane Lifecycle Control Hooks
    virtual bool Initialize(const OutputProfile& profile) = 0;
    virtual void StartPipeline() = 0;
    virtual void StopPipeline() = 0;
    
    // ADR-009: Atomically update properties via Profile Swapping
    virtual void SwapProfile(const OutputProfile& new_profile) = 0;

    // Read-Only Status & Telemetry Queries
    virtual SessionState GetState() const = 0;
    virtual OutputHealth GetHealth() const = 0;
    virtual OutputRuntimeState GetRuntimeState() const = 0;
    virtual const std::string& GetSessionId() const = 0;
    virtual uint32_t GetPriorityLevel() const = 0;

    // Data Plane entry point called by FrameDistributor
    virtual void ProcessVideoFrame(obs_source_t* source) = 0;
};

} // namespace mskit
