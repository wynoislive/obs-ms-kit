#include "OutputSession.hpp"
#include <obs-module.h> // libobs logging primitives
#include <util/platform.h>
#include "../video/Scaler.hpp"
#include "../encoder/EncoderInstance.hpp"
#include "../network/NetworkBuffer.hpp"
#include "../protocol/RtmpClient.hpp"
#include "ReconnectPolicy.hpp"
#include "HealthMonitor.hpp"
#include "../../kernel/KernelContext.hpp"
#include "../../kernel/IScheduler.hpp"

namespace mskit::engine {

OutputSession::OutputSession(const std::string& id, const OutputProfile& initial_profile)
    : session_id(id), current_profile(initial_profile) {
    runtime_metrics.state = mskit::SessionState::Stopped;
    runtime_metrics.health = mskit::OutputHealth::Stopped;
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
        case mskit::SessionState::Initializing: {
            std::lock_guard<std::mutex> lock(config_mutex);
            scaler = std::make_unique<Scaler>(current_profile.target_width, current_profile.target_height);
            encoder = std::make_unique<EncoderInstance>(session_id);
            network_buffer = std::make_shared<NetworkBuffer>(current_profile.network_buffer_ms);
            protocol_client = std::make_shared<RtmpClient>(session_id);

            ReconnectConfig rec_cfg;
            reconnect_policy = std::make_unique<ReconnectPolicy>(rec_cfg);
            health_monitor = std::make_shared<HealthMonitor>(session_id);

            is_initialized = true;
            TransitionTo(mskit::SessionState::Ready);
            break;
        }

        case mskit::SessionState::Ready:
            break;

        case mskit::SessionState::Encoding:
            break;

        case mskit::SessionState::Streaming:
            break;

        case mskit::SessionState::Congested:
            break;

        case mskit::SessionState::Reconnecting:
            break;

        case mskit::SessionState::Recovering:
            break;

        case mskit::SessionState::Stopped:
        case mskit::SessionState::Error: {
            is_initialized = false;
            scaler.reset();
            {
                std::lock_guard<std::mutex> lock(encoder_mutex);
                if (encoder) {
                    encoder->Close();
                    encoder.reset();
                }
            }
            network_buffer.reset();
            if (protocol_client) {
                protocol_client->Disconnect();
                protocol_client.reset();
            }
            reconnect_policy.reset();
            health_monitor.reset();
            break;
        }
    }
}

void OutputSession::StartPipeline() {
    if (current_state.load() == mskit::SessionState::Stopped) {
        if (!Initialize(current_profile)) return;
    }

    if (current_state.load() == mskit::SessionState::Ready) {
        TransitionTo(mskit::SessionState::Encoding);

        {
            std::lock_guard<std::mutex> lock(encoder_mutex);
            if (encoder) {
                encoder->Open(current_profile);
            }
        }
        if (protocol_client) {
            protocol_client->Connect(current_profile.endpoint_url, current_profile.credential_key);
        }

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

    if (current_state.load() == mskit::SessionState::Streaming ||
        current_state.load() == mskit::SessionState::Encoding) {
        std::lock_guard<std::mutex> enc_lock(encoder_mutex);
        if (encoder) {
            encoder->UpdateBitrate(new_profile.video_bitrate);
        }
    }
}

OutputRuntimeState OutputSession::GetRuntimeState() const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    OutputRuntimeState state = runtime_metrics;
    state.state = current_state.load();
    state.health = current_health.load();
    return state;
}

uint32_t OutputSession::GetPriorityLevel() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(config_mutex));
    return current_profile.priority_level;
}

void OutputSession::ProcessVideoFrame(obs_source_t* source) {
    // Tier 1 Critical Priority: Executes directly on the host OBS Render Thread
    if (!is_initialized) return;

    // Fast-path state check using atomic load
    mskit::SessionState state = current_state.load();
    if (state != mskit::SessionState::Streaming && state != mskit::SessionState::Encoding) {
        return;
    }

    if (!scaler || !encoder) return;

    // 1. Instantly stage the texture copy on the GPU render target
    // This executes a non-blocking device-side copy to our write staging surface
    scaler->SubmitSourceFrame(source);

    // Capture the precise presentation timestamp (PTS) for this layout tick
    uint64_t frame_timestamp_ns = os_gettime_ns();

    // 2. Resolve the central Kernel Context and priority Scheduler
    auto kernel_ctx = mskit::kernel::KernelContext::GetInstance();
    auto scheduler = kernel_ctx->GetRegistry().ResolveService<mskit::kernel::IScheduler>();
    if (!scheduler) {
        blog(LOG_ERROR, "[MSK-OUTPUT] [%s] Data plane dropped frame: Priority Scheduler unresolved.", session_id.c_str());
        return;
    }

    // 3. Dispatch an optimized, lock-free task to the CRITICAL worker lane
    // We capture the timestamp by value and 'this' to process off the render thread
    scheduler->PushTask(mskit::kernel::TaskPriority::Critical, [this, frame_timestamp_ns]() {
        // Guard against race conditions during an asynchronous pipeline teardown
        if (current_state.load() == mskit::SessionState::Stopped) return;

        std::vector<uint8_t> pixel_bytes;
        uint32_t linesize = 0;

        // 4. Extract frame bytes from the alternate read staging slot
        // Since this executes asynchronously, the GPU-to-CPU transfer has already cleared
        if (scaler->ExtractFramePacked(pixel_bytes, linesize)) {

            std::lock_guard<std::mutex> lock(encoder_mutex);
            if (encoder && encoder->IsActive()) {
                // 5. Pump the raw packed memory buffer down to the isolated encoder instance
                encoder->EncodeVideoFrame(pixel_bytes, linesize, frame_timestamp_ns);
            }
        }
    });
}

void OutputSession::PipelineToBuffer(struct encoder_packet* packet) {
    // Executes inside the designated encoding engine context block
    if (!packet || !network_buffer) return;

    // 6. Push compressed payload data frame directly into our memory-bounded ring buffer
    // If the network pipeline buffer returns false, the destination has breached its budget allocation
    if (!network_buffer->PushPacket(packet)) {

        // Accumulate metric anomalies under thread-safe isolation locks
        {
            std::lock_guard<std::mutex> lock(telemetry_mutex);
            runtime_metrics.dropped_frames++;
        }

        // Escalate the status signal to the Diagnostics tier asynchronously
        if (health_monitor) {
            health_monitor->UpdateHealthState(mskit::engine::OutputHealth::Congested);
        }

        blog(LOG_WARNING, "[MSK-NETWORK] [%s] Frame discarded at network boundary due to allocation saturation.",
             session_id.c_str());
    } else {
        // Smooth and project active throughput telemetries
        std::lock_guard<std::mutex> lock(telemetry_mutex);
        runtime_metrics.current_bitrate = static_cast<uint32_t>((network_buffer->GetMemoryUsage() * 8) / 1024);

        if (health_monitor && health_monitor->GetHealthState() == mskit::engine::OutputHealth::Congested) {
            // Automatically clear transient warning state metrics once buffer congestion clears
            if (network_buffer->GetQueueSize() < 5) {
                health_monitor->UpdateHealthState(mskit::engine::OutputHealth::Healthy);
            }
        }
    }
}

OutputProfile OutputSession::GetProfile() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(config_mutex));
    return current_profile;
}

bool OutputSession::IsActive() const {
    mskit::SessionState state = current_state.load();
    return (state != mskit::SessionState::Stopped && state != mskit::SessionState::Error);
}

void OutputSession::GetLiveTelemetry(uint32_t& out_bitrate, double& out_fps, uint32_t& out_dropped) const {
    std::lock_guard<std::mutex> lock(telemetry_mutex);
    out_bitrate = runtime_metrics.current_bitrate;
    out_fps = runtime_metrics.output_fps;
    out_dropped = static_cast<uint32_t>(runtime_metrics.dropped_frames);
}

void OutputSession::GetNetworkTelemetry(int64_t& out_rtt_ms, double& out_congestion_factor) const {
    if (protocol_client && protocol_client->IsConnected()) {
        protocol_client->GetNetworkTelemetry(out_rtt_ms, out_congestion_factor);
    } else {
        out_rtt_ms = 0;
        out_congestion_factor = 0.0;
    }
}

std::shared_ptr<mskit::engine::HealthMonitor> OutputSession::GetHealthMonitor() const {
    return health_monitor;
}

bool OutputSession::UpdateBitrate(uint32_t new_bitrate_kbps) {
    std::lock_guard<std::mutex> lock(encoder_mutex);
    if (encoder && encoder->IsActive()) {
        return encoder->UpdateBitrate(new_bitrate_kbps);
    }
    return false;
}

} // namespace mskit::engine
