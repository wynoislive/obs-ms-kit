#include "ReconnectPolicy.hpp"
#include <obs-module.h>
#include <algorithm>

namespace mskit::engine {

ReconnectPolicy::ReconnectPolicy(const ReconnectConfig& cfg)
    : config(cfg), current_delay_ms(cfg.base_retry_delay_ms) {}

void ReconnectPolicy::RecordSuccess() {
    current_retry_count = 0;
    current_delay_ms = config.base_retry_delay_ms;
    using_fallback = false;
    blog(LOG_INFO,
         "[MSK-OUTPUT] Reconnect tracker successfully reset following a stable link handshake.");
}

bool ReconnectPolicy::RecordFailure(std::string& out_target_url,
                                     const std::string& primary_url) {
    current_retry_count++;

    // Strict control boundary: drop the output to terminal error state if capped
    if (current_retry_count > config.max_retries) {
        blog(LOG_ERROR,
             "[MSK-OUTPUT] Reconnect policy exhausted. Hard cap of %u retries reached.",
             config.max_retries);
        return false;
    }

    // Multiply delay metrics exponentially beyond the initial drop pass
    if (current_retry_count > 1) {
        current_delay_ms = (std::min)(current_delay_ms * 2, config.max_retry_delay_ms);
    }

    // Evaluate backup server routing thresholds
    // Shift targets if primary routing paths hit a hard midpoint failure limit
    if (config.enable_fallback &&
        !config.fallback_url.empty() &&
        current_retry_count > (config.max_retries / 2)) {

        if (!using_fallback) {
            blog(LOG_WARNING,
                 "[MSK-OUTPUT] Route degradation hit threshold. "
                 "Diverting traffic to backup server ingest.");
            using_fallback = true;
        }
        out_target_url = config.fallback_url;
    } else {
        using_fallback = false;
        out_target_url = primary_url;
    }

    blog(LOG_WARNING,
         "[MSK-OUTPUT] Connection dropping detected. Scheduling Retry #%u in %u ms via: %s",
         current_retry_count, current_delay_ms, out_target_url.c_str());

    return true;
}

} // namespace mskit::engine
