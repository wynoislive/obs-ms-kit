#include "FrameDistributor.hpp"
#include <obs-module.h>

namespace mskit::engine {

// Static proxy bridge to route libobs global C-style function pointers back into class scope
static void OBS_MainRenderCallbackProxy(void* param, uint32_t cx, uint32_t cy) {
    auto distributor = static_cast<FrameDistributor*>(param);
    if (distributor) {
        distributor->CaptureRenderTick();
    }
    
    // Suppress unused compiler warnings for sizing values
    (void)cx;
    (void)cy;
}

void FrameDistributor::Initialize(std::shared_ptr<OutputController> controller) {
    std::lock_guard<std::mutex> lock(distributor_mutex);
    output_controller = controller;
    blog(LOG_INFO, "[MSK-CORE] FrameDistributor linked to master control plane registry.");
}

bool FrameDistributor::AttachRenderHook(obs_source_t* target_canvas) {
    std::lock_guard<std::mutex> lock(distributor_mutex);
    
    if (current_main_canvas) {
        blog(LOG_WARNING, "[MSK-CORE] Interception hook already bound to a rendering context.");
        return false;
    }

    if (!target_canvas) {
        blog(LOG_ERROR, "[MSK-CORE] Cannot attach hook: target canvas reference is null.");
        return false;
    }

    current_main_canvas = target_canvas;
    
    // Inject our proxy directly into OBS Studio's primary graphics loop thread
    obs_add_main_render_callback(OBS_MainRenderCallbackProxy, this);
    
    blog(LOG_INFO, "[MSK-CORE] FrameDistributor cleanly attached to host rendering pipeline.");
    return true;
}

void FrameDistributor::DetachRenderHook() {
    std::lock_guard<std::mutex> lock(distributor_mutex);
    
    if (!current_main_canvas) return;

    obs_remove_main_render_callback(OBS_MainRenderCallbackProxy, this);
    current_main_canvas = nullptr;
    
    blog(LOG_INFO, "[MSK-CORE] FrameDistributor cleanly detached from host rendering pipeline.");
}

void FrameDistributor::CaptureRenderTick() {
    // This method executes at Tier 1 priority directly inside the OBS render thread
    obs_source_t* canvas = nullptr;
    std::shared_ptr<OutputController> controller;

    {
        // Low-overhead stack snapshot allocation to ensure minimal lock window sizing
        std::lock_guard<std::mutex> lock(distributor_mutex);
        canvas = current_main_canvas;
        controller = output_controller.lock();
    }

    if (!canvas || !controller) return;

    // ADR-003/ADR-005: Query all active stream destinations via the shared read lock
    std::vector<std::shared_ptr<IOutputSession>> active_sessions = controller->GetAllActiveSessions();
    if (active_sessions.empty()) return;

    // Reference Fanning: Pass the single raw GPU pointer down to each session's pipeline safely
    for (const auto& session : active_sessions) {
        if (session->GetState() == mskit::SessionState::Streaming || 
            session->GetState() == mskit::SessionState::Encoding) {
            
            // The canvas pointer is distributed directly without VRAM texture duplication
            // Downstream nodes will capture their own localized GPU scaling passes asynchronously
            session->ProcessVideoFrame(canvas); 
        }
    }
}

} // namespace mskit::engine
