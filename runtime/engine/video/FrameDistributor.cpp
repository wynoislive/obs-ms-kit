#include "FrameDistributor.hpp"
#include <obs-module.h>

namespace mskit::engine {

void FrameDistributor::Initialize(std::shared_ptr<OutputController> controller) {
    std::lock_guard<std::mutex> lock(distributor_mutex);
    output_controller = controller;
}

void FrameDistributor::CaptureRenderTick() {
    // Data Plane: Core OBS render pipeline callback hook.
    // TBD in the next stages.
}

bool FrameDistributor::AttachRenderHook(obs_source_t* target_canvas) {
    std::lock_guard<std::mutex> lock(distributor_mutex);
    if (!target_canvas) return false;
    current_main_canvas = target_canvas;
    return true;
}

void FrameDistributor::DetachRenderHook() {
    std::lock_guard<std::mutex> lock(distributor_mutex);
    current_main_canvas = nullptr;
}

} // namespace mskit::engine
