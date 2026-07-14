#pragma once
#include "../config/OutputProfile.hpp"
#include <vector>
#include <cstdint>

namespace mskit::engine {

class IEncoderInstance {
public:
    virtual ~IEncoderInstance() = default;

    // Control Plane: Dynamic encoder instantiation and parameter updates
    virtual bool Open(const OutputProfile& profile) = 0;
    virtual void Close() = 0;
    virtual bool UpdateBitrate(uint32_t new_bitrate_kbps) = 0;

    // Data Plane: Ingests raw scaled system memory rows from the Scaler
    virtual bool EncodeVideoFrame(const std::vector<uint8_t>& frame_bytes,
                                  uint32_t linesize,
                                  uint64_t timestamp_ns) = 0;

    // Observers
    virtual bool IsActive() const = 0;
    virtual const char* GetEncoderType() const = 0;
};

} // namespace mskit::engine
