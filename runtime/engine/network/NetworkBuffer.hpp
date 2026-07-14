#pragma once

#include <obs-module.h>
#include <obs-encoder.h>
#include <mutex>
#include <deque>
#include <atomic>
#include <vector>

namespace mskit::engine {

struct BufferedPacket {
    encoder_packet packet;
    uint64_t arrival_time_ms;
};

class NetworkBuffer {
private:
    uint32_t target_delay_ms;
    size_t max_memory_budget_bytes;
    
    mutable std::mutex buffer_mutex;
    std::deque<BufferedPacket> packet_queue;
    
    std::atomic<size_t> current_memory_usage{0};
    std::atomic<uint64_t> dropped_frames_counter{0};

    void ClearQueueInternal();

public:
    NetworkBuffer(uint32_t delay_ms, size_t max_mem_budget_bytes = 32 * 1024 * 1024); // Default 32MB budget
    ~NetworkBuffer();

    // Data Plane: High-performance inter-thread boundary insertions
    bool PushPacket(encoder_packet* packet);
    
    // Data Plane: Pulls packets that have cleared the latency/delay threshold
    bool PopReadyPacket(encoder_packet* out_packet);

    // Control Plane: Hot-swappable sizing allocations (ADR-009)
    void UpdateDelayTarget(uint32_t new_delay_ms);
    void ResetPipeline();

    // Metrics Expositions
    size_t GetMemoryUsage() const { return current_memory_usage.load(); }
    uint64_t GetDroppedFrames() const { return dropped_frames_counter.load(); }
    size_t GetQueueSize() const { 
        std::lock_guard<std::mutex> lock(buffer_mutex);
        return packet_queue.size(); 
    }
};

} // namespace mskit::engine
