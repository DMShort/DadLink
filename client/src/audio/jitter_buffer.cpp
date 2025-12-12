#include "audio/jitter_buffer.h"
#include <algorithm>
#include <cmath>

namespace voip::audio {

JitterBuffer::JitterBuffer(uint32_t buffer_frames, uint32_t frame_size)
    : max_packets_(buffer_frames * 2)  // Allow some headroom
    , frame_size_(frame_size)
    , target_buffer_size_(buffer_frames)
{
    // deque doesn't have reserve()
    recent_jitter_.reserve(100);  // Track last 100 samples for jitter calculation
}

bool JitterBuffer::push(AudioPacket packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats_.packets_received++;
    
    // First packet initializes sequence tracking
    if (!initialized_) {
        next_sequence_ = packet.sequence;
        initialized_ = true;
    }
    
    // Check for duplicate
    if (packet.sequence < next_sequence_) {
        stats_.packets_duplicate++;
        return false;
    }
    
    // Check for extremely late packet (more than buffer capacity behind)
    if (packet.sequence > next_sequence_ + max_packets_) {
        stats_.packets_late++;
        return false;
    }
    
    // Buffer is full, drop oldest if necessary
    if (buffer_.size() >= max_packets_) {
        stats_.packets_dropped++;
        buffer_.pop_front();
    }
    
    // Find insertion point to maintain sorted order
    size_t insert_pos = find_insertion_point(packet.sequence);
    
    // Check if this sequence already exists (duplicate)
    if (insert_pos < buffer_.size() && 
        buffer_[insert_pos].sequence == packet.sequence) {
        stats_.packets_duplicate++;
        return false;
    }
    
    // Insert packet
    BufferEntry entry{
        .sequence = packet.sequence,
        .timestamp = packet.timestamp,
        .samples = std::move(packet.samples)
    };
    
    buffer_.insert(buffer_.begin() + insert_pos, std::move(entry));
    
    // Update max buffer size stat
    stats_.max_buffer_size = std::max(stats_.max_buffer_size, 
                                     static_cast<uint32_t>(buffer_.size()));
    
    return true;
}

std::optional<AudioPacket> JitterBuffer::pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Wait until we have at least target_buffer_size packets (initial buffering)
    if (buffer_.size() < target_buffer_size_) {
        return std::nullopt;
    }
    
    // Check if we have the next expected packet
    if (buffer_.empty()) {
        if (initialized_) {
            stats_.underruns++;
        }
        return std::nullopt;
    }
    
    // Get next expected packet
    auto& front = buffer_.front();
    
    // Check if we have the next expected packet
    if (front.sequence == next_sequence_) {
        // Perfect - we have the next packet
        AudioPacket result{
            .sequence = front.sequence,
            .timestamp = front.timestamp,
            .samples = std::move(front.samples),
            .frame_size = frame_size_
        };
        
        buffer_.pop_front();
        next_sequence_++;
        
        // Update timing for jitter calculation
        auto now = Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());
        
        if (last_pop_time_.has_value()) {
            auto delta = now.count() - last_pop_time_.value().count();
            
            float jitter = std::abs(static_cast<float>(delta) - 20000.0f);  // 20ms expected
            recent_jitter_.push_back(jitter);
            if (recent_jitter_.size() > 100) {
                recent_jitter_.erase(recent_jitter_.begin());
            }
            
            // Calculate average jitter
            float sum = 0.0f;
            for (float j : recent_jitter_) {
                sum += j;
            }
            stats_.jitter_ms = sum / (recent_jitter_.size() * 1000.0f);
        }
        last_pop_time_ = now;
        
        return result;
    }
    
    // Missing packet(s) - we have a later packet but not the next one
    if (front.sequence > next_sequence_) {
        // Return empty packet for PLC (Packet Loss Concealment)
        AudioPacket plc_packet{
            .sequence = next_sequence_,
            .timestamp = Timestamp(0),  // Unknown timestamp
            .samples = {},  // Empty indicates loss
            .frame_size = frame_size_
        };
        
        next_sequence_++;
        stats_.packets_late++;
        
        return plc_packet;
    }
    
    // Should never reach here
    return std::nullopt;
}

bool JitterBuffer::is_ready() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size() >= target_buffer_size_;
}

uint32_t JitterBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(buffer_.size());
}

void JitterBuffer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
    next_sequence_ = 0;
    initialized_ = false;
    last_pop_time_.reset();
    recent_jitter_.clear();
    stats_ = JitterStats{};
}

JitterStats JitterBuffer::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.current_buffer_size = static_cast<uint32_t>(buffer_.size());
    return stats_;
}

size_t JitterBuffer::find_insertion_point(SequenceNumber seq) const {
    // Binary search for insertion point
    auto it = std::lower_bound(
        buffer_.begin(),
        buffer_.end(),
        seq,
        [](const BufferEntry& entry, SequenceNumber s) {
            return entry.sequence < s;
        }
    );
    
    return std::distance(buffer_.begin(), it);
}

bool JitterBuffer::is_sequence_valid(SequenceNumber seq) const {
    if (!initialized_) {
        return true;
    }
    
    // Check if sequence is within valid window
    return seq >= next_sequence_ && 
           seq < next_sequence_ + max_packets_;
}

} // namespace voip::audio
