#include "OutputController.hpp"
#include "OutputSession.hpp"
#include "../protocol/RtmpClient.hpp"
#include "../protocol/SrtClient.hpp"
#include "../protocol/WhipClient.hpp"
#include "ReconnectPolicy.hpp"
#include "HealthMonitor.hpp"
#include "../video/Scaler.hpp"
#include "../encoder/EncoderInstance.hpp"
#include "../network/NetworkBuffer.hpp"
#include <obs-module.h>
#include <util/platform.h>

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

std::shared_ptr<IOutputSession> OutputController::CreateSession(
    const std::string& node_id,
    const std::string& protocol,
    const std::string& url,
    const std::string& key,
    const OutputProfile& profile) 
{
    // 1. Acquire unique writer lock to prevent concurrent modification race conditions
    std::unique_lock<std::shared_mutex> lock(registry_mutex);

    // 2. Reject registrations that would cause an ID collision
    if (sessions.find(node_id) != sessions.end()) {
        blog(LOG_ERROR, "[MSK-CONTROLLER] Cannot create session: ID '%s' is already registered.", node_id.c_str());
        return nullptr;
    }

    blog(LOG_INFO, "[MSK-CONTROLLER] Assembling dependencies for output target node: '%s'", node_id.c_str());

    // Copy and adapt profile parameters
    OutputProfile session_profile = profile;
    session_profile.destination_name = node_id;
    session_profile.endpoint_url = url;
    session_profile.credential_key = key;

    // 3. Instantiate the correct concrete Protocol Transport Client (ADR-004)
    std::shared_ptr<IProtocolClient> transport;
    if (protocol == "RTMP") {
        session_profile.protocol = StreamProtocol::RTMP;
        transport = std::make_shared<RtmpClient>(node_id);
    } else if (protocol == "SRT") {
        session_profile.protocol = StreamProtocol::SRT;
        transport = std::make_shared<SrtClient>(node_id);
    } else if (protocol == "WHIP") {
        session_profile.protocol = StreamProtocol::WHIP;
        transport = std::make_shared<WhipClient>(node_id);
    } else {
        blog(LOG_ERROR, "[MSK-CONTROLLER] Unknown transport protocol type requested: '%s'", protocol.c_str());
        return nullptr;
    }

    // 4. Instantiate the remaining isolated runtime sub-components
    auto health_monitor = std::make_shared<HealthMonitor>(node_id);
    
    ReconnectConfig reconnect_cfg;
    reconnect_cfg.fallback_url = ""; // Populated dynamically if backup streams are configured
    auto reconnect_policy = std::make_unique<ReconnectPolicy>(reconnect_cfg);

    auto network_buffer = std::make_shared<NetworkBuffer>(100); // 100-packet bounded limit

    auto scaler = std::make_unique<Scaler>(session_profile.target_width, session_profile.target_height);
    auto encoder = std::make_unique<EncoderInstance>(node_id);

    // 5. Construct the final concrete OutputSession
    // Passing all the runtime parts cleanly via move semantics
    auto session = std::make_shared<OutputSession>(
        node_id,
        session_profile,
        url,
        key,
        std::move(scaler),
        std::move(encoder),
        network_buffer,
        transport,
        std::move(reconnect_policy),
        health_monitor
    );

    // 6. Push the newly assembled session into the registry mapping
    sessions[node_id] = session;

    blog(LOG_INFO, "[MSK-CONTROLLER] Target session '%s' successfully spawned and registered.", node_id.c_str());
    return session;
}

bool OutputController::DestroySession(const std::string& node_id) {
    std::unique_lock<std::shared_mutex> lock(registry_mutex);
    
    auto it = sessions.find(node_id);
    if (it == sessions.end()) {
        return false;
    }

    // Force disconnect and clean up native resources safely before deletion
    auto session = it->second;
    if (session->IsActive()) {
        session->StopPipeline();
    }

    sessions.erase(it);
    blog(LOG_INFO, "[MSK-CONTROLLER] Target session '%s' successfully destroyed and unregistered.", node_id.c_str());
    return true;
}

} // namespace mskit::engine
