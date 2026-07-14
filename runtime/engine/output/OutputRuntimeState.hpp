#pragma once
#include <cstdint>

namespace mskit {

enum class SessionState : uint8_t {
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

enum class OutputHealth : uint8_t {
    Healthy,
    Warning,
    Congested,
    Reconnecting,
    Recovering,
    Stopped,
    Error
};

// ADR-009: Mutable Telemetry & Execution State Block
struct OutputRuntimeState {
    SessionState state      = SessionState::Stopped;
    OutputHealth health    = OutputHealth::Stopped;
    uint32_t current_bitrate = 0;
    uint64_t dropped_frames  = 0;
    uint32_t reconnect_count = 0;
    uint32_t rtt_ms          = 0;
    uint32_t queue_depth     = 0;
};

} // namespace mskit
