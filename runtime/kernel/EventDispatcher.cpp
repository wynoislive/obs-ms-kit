#include "EventDispatcher.hpp"
#include <obs-module.h>

namespace mskit::kernel {

void EventDispatcher::Subscribe(ControlEvent system_event, EventCallback callback) {
    if (!callback) return;

    std::unique_lock<std::shared_mutex> lock(bus_mutex);
    subscribers[system_event].push_back(std::move(callback));
}

void EventDispatcher::Broadcast(ControlEvent system_event,
                                 const std::string& node_id,
                                 const std::string& metadata) {
    std::vector<EventCallback> targets;

    // 1. Snapshot execution targets quickly under a shared read-lock
    {
        std::shared_lock<std::shared_mutex> lock(bus_mutex);
        auto it = subscribers.find(system_event);
        if (it == subscribers.end() || it->second.empty()) {
            return;
        }
        targets = it->second;
    }

    // 2. Execute callbacks cleanly OUTSIDE the lock window.
    //    This isolates the event bus entirely from listener logic side effects,
    //    preventing callback deadlock when listeners re-subscribe or touch shared state.
    for (const auto& callback : targets) {
        try {
            callback(node_id, metadata);
        } catch (const std::exception& e) {
            blog(LOG_ERROR,
                 "[MSK-CORE] Exception caught in event listener for Event ID %u: %s",
                 static_cast<unsigned>(system_event), e.what());
        } catch (...) {
            blog(LOG_ERROR,
                 "[MSK-CORE] Unknown exception caught in event listener for Event ID %u",
                 static_cast<unsigned>(system_event));
        }
    }
}

} // namespace mskit::kernel
