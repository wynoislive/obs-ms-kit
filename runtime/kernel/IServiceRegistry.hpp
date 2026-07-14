#pragma once
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <shared_mutex>

namespace mskit::kernel {

class IServiceRegistry {
private:
    mutable std::shared_mutex registry_mutex;
    std::unordered_map<std::type_index, std::shared_ptr<void>> services;

public:
    virtual ~IServiceRegistry() = default;

    // Registers a service instance bound cleanly to its interface contract type
    template <typename InterfaceType>
    void RegisterService(std::shared_ptr<InterfaceType> service_instance) {
        std::unique_lock<std::shared_mutex> lock(registry_mutex);
        services[typeid(InterfaceType)] = std::static_pointer_cast<void>(service_instance);
    }

    // Resolves a service interface contract safely across decoupled layers
    template <typename InterfaceType>
    std::shared_ptr<InterfaceType> ResolveService() const {
        std::shared_lock<std::shared_mutex> lock(registry_mutex);
        auto it = services.find(typeid(InterfaceType));
        if (it == services.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<InterfaceType>(it->second);
    }
};

} // namespace mskit::kernel
