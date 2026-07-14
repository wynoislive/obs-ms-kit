#include "Scaler.hpp"
#include <obs-module.h>

namespace mskit::engine {

Scaler::Scaler(uint32_t width, uint32_t height, enum gs_color_format color_space)
    : target_width(width), target_height(height), format(color_space) {}

Scaler::~Scaler() {
    ReleaseGraphicsResources();
}

void Scaler::ReleaseGraphicsResources() {
    // TBD in actual graphics context implementation.
}

bool Scaler::Resize(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(memory_mutex);
    target_width = width;
    target_height = height;
    return true;
}

void Scaler::SubmitSourceFrame(obs_source_t* source) {
    // TBD in actual graphics context implementation.
    (void)source;
}

bool Scaler::ExtractFramePacked(std::vector<uint8_t>& out_bytes, uint32_t& out_linesize) {
    // TBD in actual graphics context implementation.
    (void)out_bytes;
    (void)out_linesize;
    return false;
}

} // namespace mskit::engine
