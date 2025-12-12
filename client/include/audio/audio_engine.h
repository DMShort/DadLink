#pragma once

#include "common/types.h"
#include "common/result.h"
#include "common/lock_free_queue.h"
#include <portaudio.h>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

namespace voip::audio {

// Callback types
using CaptureCallback = std::function<void(const float* pcm, size_t frame_count)>;
using PlaybackCallback = std::function<void(float* pcm, size_t frame_count)>;

/**
 * AudioEngine - Manages audio capture and playback via PortAudio
 * 
 * Thread Safety: Public methods are thread-safe.
 * Audio callbacks run on real-time threads - must follow RT safety rules!
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    
    // Disable copy
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    
    /**
     * Initialize audio system with configuration
     * Must be called before any other methods
     */
    Result<void> initialize(const AudioConfig& config);
    
    /**
     * Shutdown audio system and release resources
     */
    void shutdown();
    
    /**
     * Enumerate available audio devices
     */
    std::vector<AudioDevice> enumerate_input_devices();
    std::vector<AudioDevice> enumerate_output_devices();
    
    /**
     * Set active audio devices
     * Returns error if device not found or incompatible
     */
    Result<void> set_input_device(DeviceId id);
    Result<void> set_output_device(DeviceId id);
    
    /**
     * Start/stop audio capture from microphone
     */
    Result<void> start_capture();
    Result<void> stop_capture();
    
    /**
     * Start/stop audio playback to speakers
     */
    Result<void> start_playback();
    Result<void> stop_playback();
    
    /**
     * Set capture callback (called from real-time thread!)
     * Callback receives PCM audio data from microphone
     */
    void set_capture_callback(CaptureCallback callback);
    
    /**
     * Set playback callback (called from real-time thread!)
     * Callback must fill PCM buffer for output
     */
    void set_playback_callback(PlaybackCallback callback);
    
    /**
     * Get current audio levels (0.0 - 1.0)
     * Thread-safe, can be called from any thread
     */
    [[nodiscard]] float get_input_level() const noexcept;
    [[nodiscard]] float get_output_level() const noexcept;
    
    /**
     * Get audio statistics
     * Thread-safe
     */
    [[nodiscard]] AudioStats get_stats() const;
    
    /**
     * Set volume (0.0 - 2.0)
     * Thread-safe
     */
    void set_input_volume(float volume) noexcept;
    void set_output_volume(float volume) noexcept;
    
private:
    // PortAudio callback functions (static, call member functions)
    static int capture_callback_static(
        const void* input,
        void* output,
        unsigned long frame_count,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags,
        void* user_data
    );
    
    static int playback_callback_static(
        const void* input,
        void* output,
        unsigned long frame_count,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags,
        void* user_data
    );
    
    // Member callback handlers
    int handle_capture(
        const float* input,
        unsigned long frame_count,
        PaStreamCallbackFlags status_flags
    );
    
    int handle_playback(
        float* output,
        unsigned long frame_count,
        PaStreamCallbackFlags status_flags
    );
    
    // Helper methods
    float calculate_rms(const float* pcm, size_t count) const noexcept;
    
    // Configuration
    AudioConfig config_;
    
    // PortAudio streams
    PaStream* capture_stream_ = nullptr;
    PaStream* playback_stream_ = nullptr;
    
    // Selected devices
    DeviceId input_device_id_ = paNoDevice;
    DeviceId output_device_id_ = paNoDevice;
    
    // Callbacks
    CaptureCallback capture_callback_;
    PlaybackCallback playback_callback_;
    
    // Statistics (atomic for thread safety)
    mutable std::atomic<uint64_t> input_overflows_{0};
    mutable std::atomic<uint64_t> output_underflows_{0};
    mutable std::atomic<float> current_input_level_{0.0f};
    mutable std::atomic<float> current_output_level_{0.0f};
    
    // Volume controls (atomic)
    std::atomic<float> input_volume_{1.0f};
    std::atomic<float> output_volume_{0.8f};
    
    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> capture_running_{false};
    std::atomic<bool> playback_running_{false};
};

} // namespace voip::audio
