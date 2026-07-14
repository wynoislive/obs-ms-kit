#pragma once
#include "IService.hpp"
#include <string>
#include <functional>
#include <cstdint>


namespace mskit::kernel {

enum class ControlEvent : uint8_t {
    OutputStarted,
    OutputStopped,
    OutputWarning,
    OutputError,
    HealthChanged,
    ConfigurationChanged,
    PerformanceWarning
};

using EventCallback = std::function<void(const std::string& node_id, const std::string& metadata)>;

class IEventDispatcher : public IService {
public:
    virtual ~IEventDispatcher() = default;
    virtual void Subscribe(ControlEvent system_event, EventCallback callback) = 0;
    virtual void Broadcast(ControlEvent system_event, const std::string& node_id, const std::string& metadata) = 0;
};

} // namespace mskit::kernel
