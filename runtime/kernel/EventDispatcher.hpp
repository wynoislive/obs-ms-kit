#pragma once
#include "IEventDispatcher.hpp"
#include <unordered_map>
#include <vector>
#include <shared_mutex>

namespace mskit::kernel {

class EventDispatcher : public IEventDispatcher {
private:
    mutable std::shared_mutex bus_mutex;
    std::unordered_map<ControlEvent, std::vector<EventCallback>> subscribers;

public:
    EventDispatcher() = default;
    virtual ~EventDispatcher() override = default;

    // IService Overrides
    virtual const char* GetServiceName() const override { return "EventDispatcher"; }

    // IEventDispatcher Overrides
    virtual void Subscribe(ControlEvent system_event, EventCallback callback) override;
    virtual void Broadcast(ControlEvent system_event,
                           const std::string& node_id,
                           const std::string& metadata) override;
};

} // namespace mskit::kernel
