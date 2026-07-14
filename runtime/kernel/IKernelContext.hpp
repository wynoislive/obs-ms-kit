#pragma once
#include "ServiceRegistry.hpp"
#include <cstdint>

namespace mskit::kernel {

enum class KernelRuntimeState : uint8_t {
    Boot,
    Initialize,
    Ready,
    Running,
    Stopping,
    Shutdown
};

class IKernelContext {
public:
    virtual ~IKernelContext() = default;
    
    virtual bool BootKernel() = 0;
    virtual void ShutdownKernel() = 0;
    
    virtual KernelRuntimeState GetState() const = 0;
    virtual const ServiceRegistry& GetRegistry() const = 0;
    virtual ServiceRegistry& GetWritableRegistry() = 0;
};

} // namespace mskit::kernel
