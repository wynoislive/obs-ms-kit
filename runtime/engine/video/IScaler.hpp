#pragma once
#include <obs-module.h>
#include <graphics/graphics.h>
#include <vector>
#include <cstdint>

namespace mskit::engine {

class IScaler {
public:
    virtual ~IScaler() = default;

    // Control Plane: Dynamic resizing hook
    virtual bool Resize(uint32_t width, uint32_t height) = 0;

    // Data Plane: Called by FrameDistributor within OBS graphics thread context
    virtual void SubmitSourceFrame(obs_source_t* source) = 0;

    // Data Plane: Called by asynchronous worker to pull scaled texture bytes
    virtual bool ExtractFramePacked(std::vector<uint8_t>& out_bytes, uint32_t& out_linesize) = 0;
};

} // namespace mskit::engine
