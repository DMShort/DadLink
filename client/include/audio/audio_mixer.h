#pragma once

#include "common/types.h"
#include <vector>
#include <map>
#include <cstdint>

namespace voip::audio {

/**
 * AudioMixer - Combines multiple audio streams with volume control and ducking
 * 
 * Thread Safety: mix() must be called from audio thread (real-time safe)
 * All other methods are thread-safe via atomic operations
 */
class AudioMixer {
public:
    /**
     * Input stream for mixing
     */
    struct ChannelStream {
        ChannelId id = 0;
        const float* samples = nullptr;  // PCM audio data
        size_t sample_count = 0;
        float volume = 1.0f;              // 0.0-2.0
        int priority = 0;                 // 0=lowest, 10=highest
        bool speaking = false;            // Is there voice activity?
    };
    
    /**
     * Mixing configuration
     */
    struct Config {
        // Audio ducking settings
        float ducking_amount = 0.5f;      // Reduce to 50% when ducked
        float ducking_threshold = 0.1f;   // RMS threshold for "speaking"
        bool enable_ducking = true;
        
        // Volume smoothing (prevents clicks/pops)
        float volume_smooth_factor = 0.95f;  // IIR filter (0.0-1.0, closer to 1 = smoother)
        
        // Output normalization
        bool enable_normalization = true;
        float normalization_headroom = 0.9f;  // Keep output below 0.9 to prevent clipping
    };
    
    AudioMixer();
    explicit AudioMixer(const Config& config);
    
    /**
     * Mix multiple channel streams into single output
     * 
     * REAL-TIME SAFE: Can be called from audio thread
     * - No heap allocation
     * - No locks
     * - Bounded execution time
     * 
     * @param inputs Vector of input streams to mix
     * @param output Output buffer (must be pre-allocated)
     * @param frame_count Number of frames to mix
     */
    void mix(
        const std::vector<ChannelStream>& inputs,
        float* output,
        size_t frame_count
    ) noexcept;
    
    /**
     * Update mixer configuration
     * Thread-safe
     */
    void set_config(const Config& config) noexcept;
    [[nodiscard]] Config get_config() const noexcept;
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t total_mixes = 0;
        uint64_t clipped_samples = 0;
        float peak_level = 0.0f;
        size_t active_channels = 0;
    };
    
    [[nodiscard]] Stats get_stats() const noexcept;
    void reset_stats() noexcept;
    
private:
    /**
     * Calculate RMS (Root Mean Square) of audio samples
     * Used for detecting speech and ducking
     */
    [[nodiscard]] float calculate_rms(const float* samples, size_t count) const noexcept;
    
    /**
     * Apply soft clipping to prevent harsh distortion
     * Uses tanh for smooth saturation
     */
    [[nodiscard]] float soft_clip(float sample) const noexcept;
    
    /**
     * Get smoothed volume for channel
     * Uses IIR filter to prevent clicks/pops
     */
    [[nodiscard]] float get_smoothed_volume(ChannelId id, float target_volume) noexcept;
    
    /**
     * Apply audio ducking based on priority
     * Returns adjusted volume for the channel
     */
    [[nodiscard]] float apply_ducking(
        const ChannelStream& stream,
        bool high_priority_speaking
    ) const noexcept;
    
    // Configuration
    Config config_;
    
    // Volume smoothing state (per channel)
    std::map<ChannelId, float> smoothed_volumes_;
    
    // Statistics
    mutable uint64_t total_mixes_ = 0;
    mutable uint64_t clipped_samples_ = 0;
    mutable float peak_level_ = 0.0f;
};

} // namespace voip::audio
