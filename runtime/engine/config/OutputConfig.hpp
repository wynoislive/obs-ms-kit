#pragma once
#include <string>
#include <cstdint>

namespace mskit {

enum class NodeState : uint8_t {
    Stopped,
    Initializing,
    Ready,
    Encoding,
    Streaming,
    Congested,
    Reconnecting,
    Recovering,
    Error
};

enum class StreamProtocol : uint8_t {
    RTMP,
    RTMPS,
    SRT,
    WHIP
};

// ADR-009: Centralized Immutable Configuration Block
struct OutputNodeConfig {
    std::string destination_name;
    std::string endpoint_url;
    std::string credential_key;
    
    uint32_t target_width   = 1920;
    uint32_t target_height  = 1080;
    uint32_t video_bitrate  = 6000;
    uint32_t target_fps     = 60;
    
    uint32_t network_buffer_ms = 0; // Stream delay allocation
    uint32_t audio_track_idx   = 1;
    uint32_t priority_level    = 1; // ADR-007 Priority Engine mapping
    StreamProtocol protocol    = StreamProtocol::RTMP;
};

} // namespace mskit
