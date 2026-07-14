#pragma once
#include "IServiceRegistry.hpp"
#include <cstdint>

namespace mskit::kernel {

enum class KernelState : uint8_t {
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

    // Lifecycle Control Contracts
    virtual bool BootKernel() = 0;
    virtual void ShutdownKernel() = 0;

    // Engine Core access handles
    virtual KernelState GetKernelState() const = 0;
    virtual const IServiceRegistry& GetRegistry() const = 0;
    virtual IServiceRegistry& GetWritableRegistry() = 0;
};

} // namespace mskit::kernel
