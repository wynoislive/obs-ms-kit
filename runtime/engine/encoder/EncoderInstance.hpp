#pragma once
#include "IEncoderInstance.hpp"
#include <obs-module.h>
#include <string>
#include <mutex>

namespace mskit::engine {

class EncoderInstance : public IEncoderInstance {
private:
    std::string instance_id;
    mutable std::mutex encoder_mutex;

    // Low-level raw libobs encoder reference handle
    obs_encoder_t* obs_video_encoder = nullptr;
    bool is_initialized = false;

    bool BindObsEncoder(const std::string& encoder_id, const OutputProfile& profile);

public:
    explicit EncoderInstance(const std::string& id);
    virtual ~EncoderInstance() override;

    // IEncoderInstance Interface Implementations
    virtual bool Open(const OutputProfile& profile) override;
    virtual void Close() override;
    virtual bool UpdateBitrate(uint32_t new_bitrate_kbps) override;
    virtual bool EncodeVideoFrame(const std::vector<uint8_t>& frame_bytes,
                                  uint32_t linesize,
                                  uint64_t timestamp_ns) override;

    virtual bool IsActive() const override { return is_initialized; }
    virtual const char* GetEncoderType() const override;
};

} // namespace mskit::engine
