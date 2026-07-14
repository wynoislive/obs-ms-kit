#include "KernelContext.hpp"
#include "Scheduler.hpp"
#include "EventDispatcher.hpp"
#include "ConfigManager.hpp"
#include <obs-module.h>

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

    // Register async task execution framework
    auto task_scheduler = std::make_shared<Scheduler>(2);
    service_registry.RegisterService<IScheduler>(task_scheduler);

    // Register type-safe control event bus
    auto event_bus = std::make_shared<EventDispatcher>();
    service_registry.RegisterService<IEventDispatcher>(event_bus);

    // Register configuration manager
    auto config_mgr = std::make_shared<ConfigManager>();
    service_registry.RegisterService<IConfigManager>(config_mgr);

    runtime_state = KernelRuntimeState::Ready;
    blog(LOG_INFO, "[MSK-CORE] MS-Kit Platform Kernel Context successfully booted.");

    runtime_state = KernelRuntimeState::Running;
    return true;
}

void KernelContext::ShutdownKernel() {
    runtime_state = KernelRuntimeState::Stopping;
    blog(LOG_INFO, "[MSK-CORE] Tearing down kernel platform context layers safely...");

    // Services drop sequentially via shared_ptr RAII scoping when registry clears
    runtime_state = KernelRuntimeState::Shutdown;
    blog(LOG_INFO, "[MSK-CORE] Kernel shutdown complete.");
}

} // namespace mskit::kernel
