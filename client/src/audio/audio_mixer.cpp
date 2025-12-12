#include "audio/audio_mixer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace voip::audio {

AudioMixer::AudioMixer()
    : config_{}
{
}

AudioMixer::AudioMixer(const Config& config)
    : config_(config)
{
}

void AudioMixer::mix(
    const std::vector<ChannelStream>& inputs,
    float* output,
    size_t frame_count
) noexcept {
    if (!output || frame_count == 0) {
        return;
    }
    
    total_mixes_++;
    
    // Initialize output to silence
    std::memset(output, 0, frame_count * sizeof(float));
    
    if (inputs.empty()) {
        return;
    }
    
    // Check if any high-priority channel is speaking (for ducking)
    bool high_priority_speaking = false;
    int max_priority = 0;
    
    if (config_.enable_ducking) {
        for (const auto& stream : inputs) {
            if (stream.speaking && stream.priority > max_priority) {
                max_priority = stream.priority;
            }
        }
        
        // Consider "high priority" if priority >= 7
        high_priority_speaking = (max_priority >= 7);
    }
    
    // Mix all channels
    size_t active_channels = 0;
    
    for (const auto& stream : inputs) {
        if (!stream.samples || stream.sample_count == 0) {
            continue;
        }
        
        // Get smoothed volume to prevent clicks
        float target_volume = stream.volume;
        
        // Apply ducking if this is a lower-priority channel
        if (config_.enable_ducking && high_priority_speaking && stream.priority < max_priority) {
            target_volume *= config_.ducking_amount;
        }
        
        float smooth_volume = get_smoothed_volume(stream.id, target_volume);
        
        // Mix this channel into output
        size_t samples_to_mix = std::min(stream.sample_count, frame_count);
        
        for (size_t i = 0; i < samples_to_mix; ++i) {
            output[i] += stream.samples[i] * smooth_volume;
        }
        
        active_channels++;
    }
    
    // Normalize output to prevent clipping
    if (config_.enable_normalization && active_channels > 0) {
        // Simple normalization: divide by number of active channels
        // This prevents summing from exceeding [-1.0, 1.0]
        float normalization_factor = 1.0f / std::sqrt(static_cast<float>(active_channels));
        normalization_factor *= config_.normalization_headroom;
        
        for (size_t i = 0; i < frame_count; ++i) {
            output[i] *= normalization_factor;
            
            // Apply soft clipping for any remaining peaks
            if (std::abs(output[i]) > 1.0f) {
                output[i] = soft_clip(output[i]);
                clipped_samples_++;
            }
            
            // Track peak level
            float abs_sample = std::abs(output[i]);
            if (abs_sample > peak_level_) {
                peak_level_ = abs_sample;
            }
        }
    } else {
        // No normalization - just clip if needed
        for (size_t i = 0; i < frame_count; ++i) {
            if (std::abs(output[i]) > 1.0f) {
                output[i] = soft_clip(output[i]);
                clipped_samples_++;
            }
            
            float abs_sample = std::abs(output[i]);
            if (abs_sample > peak_level_) {
                peak_level_ = abs_sample;
            }
        }
    }
}

void AudioMixer::set_config(const Config& config) noexcept {
    config_ = config;
}

AudioMixer::Config AudioMixer::get_config() const noexcept {
    return config_;
}

AudioMixer::Stats AudioMixer::get_stats() const noexcept {
    Stats stats;
    stats.total_mixes = total_mixes_;
    stats.clipped_samples = clipped_samples_;
    stats.peak_level = peak_level_;
    stats.active_channels = smoothed_volumes_.size();
    return stats;
}

void AudioMixer::reset_stats() noexcept {
    total_mixes_ = 0;
    clipped_samples_ = 0;
    peak_level_ = 0.0f;
}

float AudioMixer::calculate_rms(const float* samples, size_t count) const noexcept {
    if (!samples || count == 0) {
        return 0.0f;
    }
    
    float sum_squares = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum_squares += samples[i] * samples[i];
    }
    
    return std::sqrt(sum_squares / static_cast<float>(count));
}

float AudioMixer::soft_clip(float sample) const noexcept {
    // Use tanh for smooth saturation
    // tanh(x) maps (-inf, +inf) to (-1, +1) smoothly
    return std::tanh(sample);
}

float AudioMixer::get_smoothed_volume(ChannelId id, float target_volume) noexcept {
    // Get current smoothed volume (or initialize to target)
    auto it = smoothed_volumes_.find(id);
    if (it == smoothed_volumes_.end()) {
        smoothed_volumes_[id] = target_volume;
        return target_volume;
    }
    
    // Apply IIR filter: y[n] = alpha * y[n-1] + (1 - alpha) * x[n]
    float alpha = config_.volume_smooth_factor;
    float current = it->second;
    float smoothed = alpha * current + (1.0f - alpha) * target_volume;
    
    smoothed_volumes_[id] = smoothed;
    return smoothed;
}

float AudioMixer::apply_ducking(
    const ChannelStream& stream,
    bool high_priority_speaking
) const noexcept {
    if (!config_.enable_ducking || !high_priority_speaking) {
        return stream.volume;
    }
    
    // If this stream is high priority, don't duck it
    if (stream.priority >= 7) {
        return stream.volume;
    }
    
    // Duck lower priority channels
    return stream.volume * config_.ducking_amount;
}

} // namespace voip::audio
