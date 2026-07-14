#pragma once

#include "../output/OutputController.hpp"
#include <obs-module.h>
#include <mutex>
#include <vector>
#include <memory>

namespace mskit::engine {

class FrameDistributor {
private:
    // Weak reference hook to the control plane coordinator
    std::weak_ptr<OutputController> output_controller;
    
    // Low-overhead mutex protecting the active consumer array
    mutable std::mutex distributor_mutex;
    obs_source_t* current_main_canvas = nullptr;

public:
    FrameDistributor() = default;
    ~FrameDistributor() { DetachRenderHook(); }

    // Control Plane: Bind the controller registry to the video path
    void Initialize(std::shared_ptr<OutputController> controller);

    // Data Plane: Core OBS render pipeline callback hook
    // This executes directly inside the host OBS graphics loop thread
    void CaptureRenderTick();

    // Pipeline Registration Hooks
    bool AttachRenderHook(obs_source_t* target_canvas);
    void DetachRenderHook();

    // Status Observers
    bool IsHookActive() const { return current_main_canvas != nullptr; }
};

} // namespace mskit::engine
