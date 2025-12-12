#pragma once

#include "common/types.h"
#include "common/result.h"
#include <vector>
#include <deque>
#include <optional>
#include <chrono>
#include <mutex>

namespace voip::audio {

/**
 * Audio packet for jitter buffer
 */
struct AudioPacket {
    SequenceNumber sequence;
    Timestamp timestamp;
    std::vector<float> samples;
    size_t frame_size;
};

/**
 * Jitter buffer statistics
 */
struct JitterStats {
    uint64_t packets_received = 0;
    uint64_t packets_dropped = 0;
    uint64_t packets_late = 0;
    uint64_t packets_duplicate = 0;
    uint64_t underruns = 0;
    uint32_t current_buffer_size = 0;
    uint32_t max_buffer_size = 0;
    float jitter_ms = 0.0f;
};

/**
 * JitterBuffer - Reorders and buffers audio packets to handle network jitter
 * 
 * Responsibilities:
 * - Reorder out-of-sequence packets
 * - Compensate for network jitter
 * - Handle packet loss with silence/PLC
 * - Adaptive buffer sizing (future)
 * 
 * Thread Safety: Thread-safe. Can be called from multiple threads.
 */
class JitterBuffer {
public:
    /**
     * Create jitter buffer
     * 
     * @param buffer_frames Number of frames to buffer (affects latency)
     * @param frame_size Samples per frame
     */
    JitterBuffer(uint32_t buffer_frames, uint32_t frame_size);
    
    /**
     * Add packet to buffer
     * Returns false if packet is duplicate or too late
     */
    bool push(AudioPacket packet);
    
    /**
     * Get next packet for playback
     * Returns nullopt if buffer is empty (underrun)
     * Returns packet with empty samples if packet was lost (for PLC)
     */
    std::optional<AudioPacket> pop();
    
    /**
     * Check if buffer is ready to start playback
     * Should have enough packets buffered before starting
     */
    [[nodiscard]] bool is_ready() const;
    
    /**
     * Get current buffer size (number of packets)
     */
    [[nodiscard]] uint32_t size() const;
    
    /**
     * Get buffer capacity
     */
    [[nodiscard]] uint32_t capacity() const { return max_packets_; }
    
    /**
     * Reset buffer (clears all packets)
     */
    void reset();
    
    /**
     * Get statistics
     */
    [[nodiscard]] JitterStats get_stats() const;
    
private:
    struct BufferEntry {
        SequenceNumber sequence;
        Timestamp timestamp;
        std::vector<float> samples;
    };
    
    // Find insertion position for packet (maintains sorted order)
    size_t find_insertion_point(SequenceNumber seq) const;
    
    // Check if sequence number is in valid range
    bool is_sequence_valid(SequenceNumber seq) const;
    
    // Configuration
    const uint32_t max_packets_;
    const uint32_t frame_size_;
    const uint32_t target_buffer_size_;  // Packets to buffer before ready
    
    // Buffer storage (sorted by sequence number)
    std::deque<BufferEntry> buffer_;
    
    // State tracking
    SequenceNumber next_sequence_ = 0;
    bool initialized_ = false;
    
    // Timing
    std::optional<Timestamp> last_pop_time_;
    std::vector<float> recent_jitter_;  // For jitter calculation
    
    // Statistics
    mutable JitterStats stats_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace voip::audio
