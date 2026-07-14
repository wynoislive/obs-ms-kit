#pragma once

#include <string>
#include <cstdint>

namespace mskit::engine {

struct ReconnectConfig {
    uint32_t max_retries = 10;
    uint32_t base_retry_delay_ms = 5000;  // Initial 5-second sleep boundary
    uint32_t max_retry_delay_ms = 30000;  // Ceil cap at 30 seconds
    bool enable_fallback = true;
    std::string fallback_url;
};

class ReconnectPolicy {
private:
    ReconnectConfig config;
    uint32_t current_retry_count = 0;
    uint32_t current_delay_ms = 0;
    bool using_fallback = false;

public:
    explicit ReconnectPolicy(const ReconnectConfig& cfg);
    ~ReconnectPolicy() = default;

    // Reset tracking metrics upon successful streaming handshake
    void RecordSuccess();

    // Computes the next sleep interval using exponential backoff metrics
    // Returns false if max_retries has been breached (fatal domain fail)
    bool RecordFailure(std::string& out_target_url, const std::string& primary_url);

    // Read-only status inspectors for the Health Monitor
    uint32_t GetRetryCount() const { return current_retry_count; }
    uint32_t GetNextDelayMs() const { return current_delay_ms; }
    bool IsUsingFallback() const { return using_fallback; }
};

} // namespace mskit::engine
