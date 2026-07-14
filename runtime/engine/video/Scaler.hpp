#pragma once
#include "IScaler.hpp"
#include <mutex>
#include <atomic>

namespace mskit::engine {

class Scaler : public IScaler {
private:
    uint32_t target_width;
    uint32_t target_height;
    enum gs_color_format format;

    // Libobs internal graphic rendering contexts
    gs_texrender_t* tex_render = nullptr;
    
    // Double-buffered structure to isolate GPU writes from CPU reads
    struct StagingSlot {
        gs_stagesurf_t* surface = nullptr;
        std::atomic<bool> is_dirty{false};
    };
    StagingSlot stage_slots[2];
    
    size_t gpu_write_idx = 0;
    size_t cpu_read_idx  = 1;

    mutable std::mutex memory_mutex;
    std::vector<uint8_t> internal_pixel_cache;

    void ReleaseGraphicsResources();

public:
    Scaler(uint32_t width, uint32_t height, enum gs_color_format color_space = GS_RGBA);
    virtual ~Scaler() override;

    // IScaler Interface Implementation overrides
    virtual bool Resize(uint32_t width, uint32_t height) override;
    virtual void SubmitSourceFrame(obs_source_t* source) override;
    virtual bool ExtractFramePacked(std::vector<uint8_t>& out_bytes, uint32_t& out_linesize) override;
};

} // namespace mskit::engine
