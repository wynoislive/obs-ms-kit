#pragma once
#include "IService.hpp"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <shared_mutex>
#include <mutex>


namespace mskit::kernel {

class ServiceRegistry {
private:
    mutable std::shared_mutex registry_mutex;
    std::unordered_map<std::type_index, std::shared_ptr<IService>> services;

public:
    template <typename InterfaceType>
    void RegisterService(std::shared_ptr<InterfaceType> service_instance) {
        std::unique_lock<std::shared_mutex> lock(registry_mutex);
        services[typeid(InterfaceType)] = std::static_pointer_cast<IService>(service_instance);
    }

    template <typename InterfaceType>
    std::shared_ptr<InterfaceType> ResolveService() const {
        std::shared_lock<std::shared_mutex> lock(registry_mutex);
        auto it = services.find(typeid(InterfaceType));
        if (it == services.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<InterfaceType>(it->second);
    }

    void Clear() {
        std::unique_lock<std::shared_mutex> lock(registry_mutex);
        services.clear();
    }
};

} // namespace mskit::kernel
