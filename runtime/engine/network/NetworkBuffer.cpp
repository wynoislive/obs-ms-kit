#include "NetworkBuffer.hpp"
#include <obs-module.h>
#include <util/platform.h>

namespace mskit::engine {

NetworkBuffer::NetworkBuffer(uint32_t delay_ms, size_t max_mem_budget_bytes)
    : target_delay_ms(delay_ms), max_memory_budget_bytes(max_mem_budget_bytes) {}

NetworkBuffer::~NetworkBuffer() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    ClearQueueInternal();
}

void NetworkBuffer::ClearQueueInternal() {
    for (auto& slot : packet_queue) {
        obs_encoder_packet_release(&slot.packet);
    }
    packet_queue.clear();
    current_memory_usage.store(0);
}

bool NetworkBuffer::PushPacket(encoder_packet* packet) {
    if (!packet) return false;

    // Hard boundary check against memory allocation targets
    if (current_memory_usage.load() + packet->size > max_memory_budget_bytes) {
        dropped_frames_counter.fetch_add(1);
        blog(LOG_WARNING, "[MSK-NETWORK] Pipeline Buffer overflow hit. Dropping oldest allocation blocks.");
        return false;
    }

    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    // Deep copy the encoder payload into our isolated queue context
    BufferedPacket buffered_slot;
    obs_encoder_packet_ref(&buffered_slot.packet, packet);
    buffered_slot.arrival_time_ms = os_gettime_ns() / 1000000;

    packet_queue.push_back(buffered_slot);
    current_memory_usage.fetch_add(packet->size);

    return true;
}

bool NetworkBuffer::PopReadyPacket(encoder_packet* out_packet) {
    if (!out_packet) return false;

    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (packet_queue.empty()) return false;

    uint64_t current_time_ms = os_gettime_ns() / 1000000;
    const BufferedPacket& oldest_entry = packet_queue.front();

    // Verify if the packet has cleared the requested pipeline delay configuration
    uint64_t elapsed_time = current_time_ms - oldest_entry.arrival_time_ms;
    if (elapsed_time < target_delay_ms) {
        return false; // Packet is still aging inside the pipeline buffer
    }

    // Move packet ownership out to the transport client layer
    *out_packet = oldest_entry.packet;
    current_memory_usage.fetch_sub(oldest_entry.packet.size);
    packet_queue.pop_front();

    return true;
}

void NetworkBuffer::UpdateDelayTarget(uint32_t new_delay_ms) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    target_delay_ms = new_delay_ms;
    blog(LOG_INFO, "[MSK-CONFIG] Network Pipeline Buffer target adjusted dynamically to %u ms.", new_delay_ms);
}

void NetworkBuffer::ResetPipeline() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    ClearQueueInternal();
    blog(LOG_INFO, "[MSK-NETWORK] Network Pipeline Buffer flushed cleanly.");
}

} // namespace mskit::engine
