#include "Scaler.hpp"
#include <obs-module.h>
#include <cstring>

namespace mskit::engine {

// ─────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────

Scaler::Scaler(uint32_t width, uint32_t height, enum gs_color_format color_space)
    : target_width(width), target_height(height), format(color_space) {

    obs_enter_graphics();

    // Allocate the offscreen texture render target (no depth/stencil needed for 2D scaling)
    tex_render = gs_texrender_create(format, GS_ZS_NONE);

    // Allocate double-buffered staging surfaces for async GPU→CPU readback
    for (size_t i = 0; i < 2; ++i) {
        stage_slots[i].surface = gs_stagesurface_create(target_width, target_height, format);
        stage_slots[i].is_dirty.store(false, std::memory_order_relaxed);
    }

    obs_leave_graphics();

    blog(LOG_INFO,
         "[MSK-VIDEO] Scaler initialized: %ux%u, format=%d, double-buffered staging active.",
         target_width, target_height, static_cast<int>(format));
}

Scaler::~Scaler() {
    ReleaseGraphicsResources();
}

void Scaler::ReleaseGraphicsResources() {
    obs_enter_graphics();

    for (size_t i = 0; i < 2; ++i) {
        if (stage_slots[i].surface) {
            gs_stagesurface_destroy(stage_slots[i].surface);
            stage_slots[i].surface = nullptr;
        }
        stage_slots[i].is_dirty.store(false, std::memory_order_relaxed);
    }

    if (tex_render) {
        gs_texrender_destroy(tex_render);
        tex_render = nullptr;
    }

    obs_leave_graphics();

    blog(LOG_INFO, "[MSK-VIDEO] Scaler graphics resources released.");
}

// ─────────────────────────────────────────────────────────────
// Control Plane: Dynamic Resize
// ─────────────────────────────────────────────────────────────

bool Scaler::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        blog(LOG_WARNING, "[MSK-VIDEO] Scaler resize rejected: invalid dimensions %ux%u.", width, height);
        return false;
    }

    if (width == target_width && height == target_height) {
        return true; // No-op: dimensions unchanged
    }

    // Tear down existing GPU resources under the graphics context
    ReleaseGraphicsResources();

    // Update target dimensions
    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        target_width = width;
        target_height = height;
        internal_pixel_cache.clear();
    }

    // Reallocate with new dimensions
    obs_enter_graphics();

    tex_render = gs_texrender_create(format, GS_ZS_NONE);
    for (size_t i = 0; i < 2; ++i) {
        stage_slots[i].surface = gs_stagesurface_create(target_width, target_height, format);
        stage_slots[i].is_dirty.store(false, std::memory_order_relaxed);
    }

    obs_leave_graphics();

    blog(LOG_INFO, "[MSK-VIDEO] Scaler resized to %ux%u.", target_width, target_height);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Data Plane: GPU-side Source Rendering + Staging
// Called on the OBS graphics thread by FrameDistributor
// ─────────────────────────────────────────────────────────────

void Scaler::SubmitSourceFrame(obs_source_t* source) {
    if (!source || !tex_render) return;

    // Reset the texture render target for a fresh pass
    gs_texrender_reset(tex_render);

    // Begin rendering into our offscreen target at the scaled resolution
    if (!gs_texrender_begin(tex_render, target_width, target_height)) {
        blog(LOG_WARNING, "[MSK-VIDEO] gs_texrender_begin failed for %ux%u.", target_width, target_height);
        return;
    }

    // Set up a clean orthographic projection matching our target dimensions
    gs_ortho(0.0f, static_cast<float>(obs_source_get_width(source)),
             0.0f, static_cast<float>(obs_source_get_height(source)),
             -1.0f, 1.0f);

    // Render the source into our offscreen texture at the target resolution
    obs_source_video_render(source);

    gs_texrender_end(tex_render);

    // Retrieve the rendered texture handle
    gs_texture_t* rendered_tex = gs_texrender_get_texture(tex_render);
    if (!rendered_tex) return;

    // Stage the GPU texture into the current write-side staging surface
    // This is an async GPU copy — it does NOT block the render thread
    StagingSlot& write_slot = stage_slots[gpu_write_idx];
    if (write_slot.surface) {
        gs_stage_texture(write_slot.surface, rendered_tex);
        write_slot.is_dirty.store(true, std::memory_order_release);
    }

    // Swap the double-buffer indices so the next frame writes to the other slot
    // while the CPU can safely read from the slot we just wrote
    gpu_write_idx ^= 1;
    cpu_read_idx  ^= 1;
}

// ─────────────────────────────────────────────────────────────
// Data Plane: CPU-side Pixel Extraction
// Called from an encoder worker thread (NOT the graphics thread)
// ─────────────────────────────────────────────────────────────

bool Scaler::ExtractFramePacked(std::vector<uint8_t>& out_bytes, uint32_t& out_linesize) {
    StagingSlot& read_slot = stage_slots[cpu_read_idx];

    // Only proceed if the GPU has written new data to this slot
    if (!read_slot.is_dirty.load(std::memory_order_acquire)) {
        return false;
    }

    if (!read_slot.surface) return false;

    uint8_t* mapped_data = nullptr;
    uint32_t linesize = 0;

    obs_enter_graphics();

    bool mapped = gs_stagesurface_map(read_slot.surface, &mapped_data, &linesize);

    if (mapped && mapped_data && linesize > 0) {
        const size_t total_bytes = static_cast<size_t>(linesize) * target_height;

        {
            std::lock_guard<std::mutex> lock(memory_mutex);
            internal_pixel_cache.resize(total_bytes);
            std::memcpy(internal_pixel_cache.data(), mapped_data, total_bytes);
        }

        gs_stagesurface_unmap(read_slot.surface);
        obs_leave_graphics();

        // Mark the slot as consumed so we don't re-read stale data
        read_slot.is_dirty.store(false, std::memory_order_release);

        // Copy to caller's output buffer outside the graphics context
        std::lock_guard<std::mutex> lock(memory_mutex);
        out_bytes = internal_pixel_cache;
        out_linesize = linesize;
        return true;
    }

    // Map failed — clean up
    if (mapped) {
        gs_stagesurface_unmap(read_slot.surface);
    }
    obs_leave_graphics();

    return false;
}

} // namespace mskit::engine
