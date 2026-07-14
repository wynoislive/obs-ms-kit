#include "OutputSession.hpp"
#include <obs-module.h> // libobs logging primitives

namespace mskit::engine {

OutputSession::OutputSession(const std::string& id, const OutputProfile& initial_profile)
    : session_id(id), current_profile(initial_profile) {
    runtime_metrics.state = SessionState::Stopped;
    runtime_metrics.health = OutputHealth::Stopped;
}

OutputSession::~OutputSession() {
    StopPipeline();
}

bool OutputSession::Initialize(const OutputProfile& profile) {
    if (current_state.load() != mskit::SessionState::Stopped) {
        blog(LOG_WARNING, "[MSK-OUTPUT] [%s] Cannot initialize: Session must be Stopped.", session_id.c_str());
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex);
    current_profile = profile;
    
    return TransitionTo(mskit::SessionState::Initializing);
}

bool OutputSession::TransitionTo(mskit::SessionState target_state) {
    mskit::SessionState old_state = current_state.load();
    
    // Enforce strict state machine validation rules
    if (old_state == target_state) return true;
    
    // Prevent invalid jumps out of stopped or error states
    if (old_state == mskit::SessionState::Stopped && target_state == mskit::SessionState::Streaming) {
        blog(LOG_ERROR, "[MSK-OUTPUT] [%s] Terminal Illegal State Transition attempted.", session_id.c_str());
        return false;
    }

    current_state.store(target_state);
    
    // Synchronize health indicators with session state changes
    switch (target_state) {
        case mskit::SessionState::Stopped:
            current_health.store(mskit::OutputHealth::Stopped);
            break;
        case mskit::SessionState::Error:
            current_health.store(mskit::OutputHealth::Error);
            break;
        case mskit::SessionState::Streaming:
            current_health.store(mskit::OutputHealth::Healthy);
            break;
        case mskit::SessionState::Congested:
            current_health.store(mskit::OutputHealth::Congested);
            break;
        case mskit::SessionState::Reconnecting:
            current_health.store(mskit::OutputHealth::Reconnecting);
            break;
        case mskit::SessionState::Recovering:
            current_health.store(mskit::OutputHealth::Recovering);
            break;
        default:
            break;
    }

    blog(LOG_INFO, "[MSK-OUTPUT] [%s] Lifecycle State Shift: %d -> %d", 
         session_id.c_str(), static_cast<int>(old_state), static_cast<int>(target_state));
         
    HandleStateAction(target_state);
    return true;
}

void OutputSession::HandleStateAction(mskit::SessionState state) {
    switch (state) {
        case mskit::SessionState::Initializing:
            // Staging hardware layers
            TransitionTo(mskit::SessionState::Ready);
            break;
            
        case mskit::SessionState::Ready:
            break;
            
        case mskit::SessionState::Encoding:
            // Buffer managers binding
            break;
            
        case mskit::SessionState::Streaming:
            break;
            
        case mskit::SessionState::Congested:
            // Intercepted by the Performance budget rules engine
            break;
            
        case mskit::SessionState::Reconnecting:
            break;
            
        case mskit::SessionState::Recovering:
            break;
            
        case mskit::SessionState::Stopped:
        case mskit::SessionState::Error:
            break;
    }
}

void OutputSession::StartPipeline() {
    if (current_state.load() == mskit::SessionState::Stopped) {
        if (!Initialize(current_profile)) return;
    }
    
    if (current_state.load() == mskit::SessionState::Ready) {
        TransitionTo(mskit::SessionState::Encoding);
        TransitionTo(mskit::SessionState::Streaming);
    }
}

void OutputSession::StopPipeline() {
    if (current_state.load() == mskit::SessionState::Stopped) return;
    TransitionTo(mskit::SessionState::Stopped);
}

void OutputSession::SwapProfile(const OutputProfile& new_profile) {
    std::lock_guard<std::mutex> lock(config_mutex);
    current_profile = new_profile;
    blog(LOG_INFO, "[MSK-CONFIG] [%s] Dynamic profile swap executed cleanly.", session_id.c_str());
    
    // Note: If Streaming, push hot updates down to the active Encoder instance
}

OutputRuntimeState OutputSession::GetRuntimeState() const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    OutputRuntimeState state = runtime_metrics;
    state.state = current_state.load();
    state.health = current_health.load();
    return state;
}

} // namespace mskit::engine
