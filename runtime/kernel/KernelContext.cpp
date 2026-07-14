#include "KernelContext.hpp"
#include "Scheduler.hpp"
#include "EventDispatcher.hpp"
#include "ConfigManager.hpp"
#include "../engine/output/OutputController.hpp"
#include "../engine/analytics/PerformanceRuleEngine.hpp"
#include <util/platform.h>

namespace mskit::kernel {

std::shared_ptr<KernelContext> KernelContext::global_instance = nullptr;
std::mutex KernelContext::instance_mutex;

std::shared_ptr<KernelContext> KernelContext::GetInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!global_instance) {
        global_instance = std::shared_ptr<KernelContext>(new KernelContext());
    }
    return global_instance;
}

bool KernelContext::BootKernel() {
    if (runtime_state != KernelRuntimeState::Boot) return false;

    runtime_state = KernelRuntimeState::Initialize;
    blog(LOG_INFO, "[MSK-CORE] Initializing MS-Kit Engine Kernel Services...");

    // 1. Instantiating Tier 1 Core Foundations
    auto task_scheduler = std::make_shared<Scheduler>(2); // 2 dedicated background workers
    service_registry.RegisterService<IScheduler>(task_scheduler);

    auto event_bus = std::make_shared<EventDispatcher>();
    service_registry.RegisterService<IEventDispatcher>(event_bus);

    // 2. Instantiating Engine Controller Registry
    auto output_controller = std::make_shared<mskit::engine::OutputController>();
    service_registry.RegisterService<mskit::engine::OutputController>(output_controller);

    // 3. Instantiating and Initializing the Configuration Plane
    auto config_plane = std::make_shared<ConfigManager>();
    config_plane->Initialize(output_controller);
    service_registry.RegisterService<IConfigManager>(config_plane);

    // 4. Instantiating and Activating the Performance Analytics Loop
    auto rule_engine = std::make_shared<mskit::engine::PerformanceRuleEngine>();
    rule_engine->Initialize(output_controller);
    rule_engine->Start(); // Dispatches the first 1000ms polling tick to the Scheduler
    service_registry.RegisterService<mskit::engine::PerformanceRuleEngine>(rule_engine);

    runtime_state = KernelRuntimeState::Ready;
    blog(LOG_INFO, "[MSK-CORE] MS-Kit Platform Kernel Context successfully booted.");

    runtime_state = KernelRuntimeState::Running;
    return true;
}

void KernelContext::ShutdownKernel() {
    if (runtime_state == KernelRuntimeState::Shutdown ||
        runtime_state == KernelRuntimeState::Stopping) return;

    runtime_state = KernelRuntimeState::Stopping;
    blog(LOG_INFO, "[MSK-CORE] Tearing down kernel platform context layers safely...");

    // 1. Halt the telemetry loop first to clear pending self-scheduling tasks
    auto rule_engine = service_registry.ResolveService<mskit::engine::PerformanceRuleEngine>();
    if (rule_engine) {
        rule_engine->Stop();
        blog(LOG_INFO, "[MSK-CORE] PerformanceRuleEngine background telemetry loop stopped.");
    }

    // 2. Purge active streaming output sessions via the controller
    auto output_controller = service_registry.ResolveService<mskit::engine::OutputController>();
    if (output_controller) {
        // Force-disconnect all active transport connections cleanly
        auto sessions = output_controller->GetAllActiveSessions();
        for (auto& session : sessions) {
            if (session->IsActive()) {
                session->StopPipeline();
            }
        }
    }

    // 3. Clear the ServiceRegistry map containers
    // This drops the RAII shared references. The Scheduler destructor will now safely
    // drain any remaining critical tasks, set its stop flag, and join the worker pool.
    service_registry.Clear();

    runtime_state = KernelRuntimeState::Shutdown;
    blog(LOG_INFO, "[MSK-CORE] MS-Kit Platform Kernel Context completely shutdown.");
}

} // namespace mskit::kernel
