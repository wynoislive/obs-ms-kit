#include "PerformanceRuleEngine.hpp"
#include "../../kernel/KernelContext.hpp"
#include "../../kernel/IScheduler.hpp"
#include <util/platform.h>

namespace mskit::engine {

void PerformanceRuleEngine::Initialize(std::shared_ptr<OutputController> controller) {
    std::lock_guard<std::mutex> lock(state_mutex);
    controller_hook = controller;
    blog(LOG_INFO, "[MSK-ANALYTICS] PerformanceRuleEngine connected to master engine controller.");
}

void PerformanceRuleEngine::Start() {
    if (is_running.exchange(true)) return;
    blog(LOG_INFO, "[MSK-ANALYTICS] Activating stateful HealthMonitor control loop.");
    ExecuteControlLoopIteration();
}

void PerformanceRuleEngine::Stop() {
    is_running.store(false);
}

void PerformanceRuleEngine::ExecuteControlLoopIteration() {
    if (!is_running.load()) return;

    auto controller = controller_hook.lock();
    if (controller) {
        std::vector<std::shared_ptr<IOutputSession>> active_sessions = controller->GetAllActiveSessions();
        for (auto& session : active_sessions) {
            EvaluateSessionHealth(session);
        }
    }

    // Cooperatively reschedule the task loop in 1000ms windows via the Kernel Scheduler
    auto kernel_ctx = mskit::kernel::KernelContext::GetInstance();
    auto scheduler = kernel_ctx->GetRegistry().ResolveService<mskit::kernel::IScheduler>();

    if (scheduler && is_running.load()) {
        scheduler->PushTask(mskit::kernel::TaskPriority::Medium, [this]() {
            ExecuteControlLoopIteration();
        });
    }
}

void PerformanceRuleEngine::EvaluateSessionHealth(std::shared_ptr<IOutputSession>& session) {
    if (!session || !session->IsActive()) return;

    std::string node_id = session->GetSessionId();
    OutputProfile base_profile = session->GetProfile();

    // 1. Extract high-fidelity statistics from the session
    uint32_t active_bitrate = 0;
    uint32_t total_dropped_frames = 0;
    double live_fps = 0.0;
    session->GetLiveTelemetry(active_bitrate, live_fps, total_dropped_frames);

    // 2. Resolve the underlying HealthMonitor state directly
    auto health_monitor = session->GetHealthMonitor();
    if (!health_monitor) return;

    OutputHealth current_health = health_monitor->GetHealthState();

    std::lock_guard<std::mutex> lock(state_mutex);
    ControlState& state = state_registry[node_id];

    // Lazy initialization of our tracking cache parameters
    if (state.current_throttled_bitrate == 0) {
        state.current_throttled_bitrate = base_profile.video_bitrate;
        state.last_dropped_frames = total_dropped_frames;
    }

    // Calculate frame drops encountered specifically during this 1-second iteration window
    uint32_t frame_drop_delta = total_dropped_frames - state.last_dropped_frames;
    state.last_dropped_frames = total_dropped_frames;

    // --- RULE SET 1: REACTIVE THROTTLING (DOWN-STEP) ---
    // If the HealthMonitor flags a Congested state, or we see a sudden burst of frame drops, cut the bitrate immediately
    if (current_health == OutputHealth::Congested || frame_drop_delta > 5) {
        state.stable_ticks = 0; // Instantly break any active recovery scaling windows

        uint32_t minimum_floor = (base_profile.video_bitrate * 40) / 100; // Hard budget floor capped at 40% of standard profile

        if (state.current_throttled_bitrate > minimum_floor) {
            // Apply a rapid 20% down-step reduction to drain saturated network queues
            uint32_t reduction = (state.current_throttled_bitrate * 20) / 100;
            state.current_throttled_bitrate -= reduction;

            if (state.current_throttled_bitrate < minimum_floor) {
                state.current_throttled_bitrate = minimum_floor;
            }

            blog(LOG_WARNING, "[MSK-ANALYTICS] [%s] Health alert triggered (State: %d, Drop Delta: %u). Throttling down to %u kbps.",
                 node_id.c_str(), static_cast<int>(current_health), frame_drop_delta, state.current_throttled_bitrate);

            session->UpdateBitrate(state.current_throttled_bitrate);
        }
        return;
    }

    // --- RULE SET 2: PROACTIVE RECOVERY (UP-STEP) ---
    // If the HealthMonitor reports a perfectly Healthy channel, incrementally restore lost bandwidth allocation
    if (current_health == OutputHealth::Healthy && frame_drop_delta == 0 && state.current_throttled_bitrate < base_profile.video_bitrate) {
        state.stable_ticks++;

        // Enforce a strict 5-second stabilization cooldown window before allowing an up-step
        if (state.stable_ticks >= 5) {
            state.stable_ticks = 0;

            // Add a controlled 300 kbps up-step back toward the initial profile target
            state.current_throttled_bitrate += 300;
            if (state.current_throttled_bitrate > base_profile.video_bitrate) {
                state.current_throttled_bitrate = base_profile.video_bitrate;
            }

            blog(LOG_INFO, "[MSK-ANALYTICS] [%s] Channel health stabilized. Up-stepping target bandwidth to %u kbps.",
                 node_id.c_str(), state.current_throttled_bitrate);

            session->UpdateBitrate(state.current_throttled_bitrate);
        }
    } else {
        // Reset our recovery timers if the stream state is fluctuating within intermediate warning thresholds
        state.stable_ticks = 0;
    }
}

} // namespace mskit::engine
