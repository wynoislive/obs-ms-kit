#pragma once
#include <string>
#include <cstdint>

namespace mskit {

enum class StreamProtocol : uint8_t {
    RTMP,
    RTMPS,
    SRT,
    WHIP
};

// ADR-009: Immutable Output Destination Profile
struct OutputProfile {
    std::string destination_name;
    std::string endpoint_url;
    std::string credential_key;
    
    uint32_t target_width   = 1920;
    uint32_t target_height  = 1080;
    uint32_t video_bitrate  = 6000;
    uint32_t target_fps     = 60;
    
    uint32_t network_buffer_ms = 0; // Stream delay allocation
    uint32_t audio_track_idx   = 1;
    uint32_t priority_level    = 1;
    StreamProtocol protocol    = StreamProtocol::RTMP;
    std::string encoder_preset = "x264"; // NVENC | QSV | AMF | x264
};

} // namespace mskit
