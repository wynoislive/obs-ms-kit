#pragma once
#include "IKernelContext.hpp"
#include <mutex>
#include <memory>

namespace mskit::kernel {

class KernelContext : public IKernelContext {
private:
    static std::shared_ptr<KernelContext> global_instance;
    static std::mutex instance_mutex;

    KernelRuntimeState runtime_state{KernelRuntimeState::Boot};
    ServiceRegistry service_registry;

    KernelContext() = default;

public:
    virtual ~KernelContext() override = default;

    // Singleton Thread-Safe Instantiation Accessor
    static std::shared_ptr<KernelContext> GetInstance();

    // IKernelContext Interface Overrides
    virtual bool BootKernel() override;
    virtual void ShutdownKernel() override;

    virtual KernelRuntimeState GetState() const override { return runtime_state; }
    virtual const ServiceRegistry& GetRegistry() const override { return service_registry; }
    virtual ServiceRegistry& GetWritableRegistry() override { return service_registry; }
};

} // namespace mskit::kernel
