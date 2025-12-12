#pragma once

#include <atomic>
#include <memory>
#include <cstring>

namespace voip {

/**
 * Lock-Free Single-Producer Single-Consumer (SPSC) Queue
 * 
 * Thread-safe for one producer and one consumer thread.
 * Ideal for real-time audio callbacks.
 * 
 * IMPORTANT: This is a wait-free implementation suitable for real-time threads.
 * No dynamic allocation after construction.
 */
template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity)
        : capacity_(capacity + 1)  // +1 for full/empty distinction
        , buffer_(std::make_unique<T[]>(capacity_))
        , head_(0)
        , tail_(0)
    {
    }
    
    // Disable copy
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    /**
     * Push element into queue (producer side)
     * Returns false if queue is full
     * 
     * RT-SAFE: No allocation, no blocking
     */
    bool try_push(const T& value) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = next(current_tail);
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;  // Queue full
        }
        
        buffer_[current_tail] = value;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    /**
     * Pop element from queue (consumer side)
     * Returns false if queue is empty
     * 
     * RT-SAFE: No allocation, no blocking
     */
    bool try_pop(T& value) noexcept {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false;  // Queue empty
        }
        
        value = buffer_[current_head];
        head_.store(next(current_head), std::memory_order_release);
        return true;
    }
    
    /**
     * Check if queue is empty
     * RT-SAFE
     */
    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }
    
    /**
     * Check if queue is full
     * RT-SAFE
     */
    [[nodiscard]] bool full() const noexcept {
        const size_t next_tail = next(tail_.load(std::memory_order_acquire));
        return next_tail == head_.load(std::memory_order_acquire);
    }
    
    /**
     * Get approximate size (may be stale)
     * RT-SAFE
     */
    [[nodiscard]] size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        
        if (tail >= head) {
            return tail - head;
        } else {
            return capacity_ - head + tail;
        }
    }
    
    /**
     * Get capacity
     * RT-SAFE
     */
    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_ - 1;  // -1 because we reserve one slot
    }
    
private:
    [[nodiscard]] size_t next(size_t current) const noexcept {
        return (current + 1) % capacity_;
    }
    
    const size_t capacity_;
    std::unique_ptr<T[]> buffer_;
    
    // Cache line padding to prevent false sharing
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

/**
 * Audio Buffer Queue - Specialized for audio PCM data
 * 
 * Stores fixed-size audio frames for real-time processing
 */
class AudioBufferQueue {
public:
    AudioBufferQueue(size_t capacity, size_t frame_size)
        : frame_size_(frame_size)
        , capacity_(capacity)
        , data_(std::make_unique<float[]>(capacity * frame_size))
        , head_(0)
        , tail_(0)
    {
    }
    
    /**
     * Push audio frame (RT-SAFE)
     * Returns false if full
     */
    bool try_push(const float* frame, size_t count) noexcept {
        if (count != frame_size_) {
            return false;  // Size mismatch
        }
        
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) % capacity_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;  // Queue full
        }
        
        // Copy frame data
        std::memcpy(&data_[current_tail * frame_size_], frame, 
                   frame_size_ * sizeof(float));
        
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    /**
     * Pop audio frame (RT-SAFE)
     * Returns false if empty
     */
    bool try_pop(float* frame, size_t count) noexcept {
        if (count != frame_size_) {
            return false;  // Size mismatch
        }
        
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false;  // Queue empty
        }
        
        // Copy frame data
        std::memcpy(frame, &data_[current_head * frame_size_], 
                   frame_size_ * sizeof(float));
        
        head_.store((current_head + 1) % capacity_, std::memory_order_release);
        return true;
    }
    
    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }
    
    [[nodiscard]] size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        
        if (tail >= head) {
            return tail - head;
        } else {
            return capacity_ - head + tail;
        }
    }
    
private:
    const size_t frame_size_;
    const size_t capacity_;
    std::unique_ptr<float[]> data_;
    
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

} // namespace voip
