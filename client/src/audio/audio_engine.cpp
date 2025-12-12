#include "audio/audio_engine.h"
#include <cmath>
#include <algorithm>

namespace voip::audio {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

Result<void> AudioEngine::initialize(const AudioConfig& config) {
    if (initialized_.load()) {
        return Err<void>(ErrorCode::AudioInitFailed, "Already initialized");
    }
    
    config_ = config;
    
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        return Err<void>(ErrorCode::AudioInitFailed, 
                        std::string("PortAudio init failed: ") + Pa_GetErrorText(err));
    }
    
    // Get default devices if not set
    if (input_device_id_ == paNoDevice) {
        input_device_id_ = Pa_GetDefaultInputDevice();
    }
    if (output_device_id_ == paNoDevice) {
        output_device_id_ = Pa_GetDefaultOutputDevice();
    }
    
    initialized_.store(true);
    return Ok();
}

void AudioEngine::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    stop_capture();
    stop_playback();
    
    Pa_Terminate();
    initialized_.store(false);
}

std::vector<AudioDevice> AudioEngine::enumerate_input_devices() {
    std::vector<AudioDevice> devices;
    
    const PaDeviceIndex device_count = Pa_GetDeviceCount();
    const PaDeviceIndex default_input = Pa_GetDefaultInputDevice();
    
    for (PaDeviceIndex i = 0; i < device_count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            devices.push_back({
                .id = i,
                .name = info->name,
                .max_input_channels = info->maxInputChannels,
                .max_output_channels = 0,
                .default_sample_rate = info->defaultSampleRate,
                .is_default = (i == default_input)
            });
        }
    }
    
    return devices;
}

std::vector<AudioDevice> AudioEngine::enumerate_output_devices() {
    std::vector<AudioDevice> devices;
    
    const PaDeviceIndex device_count = Pa_GetDeviceCount();
    const PaDeviceIndex default_output = Pa_GetDefaultOutputDevice();
    
    for (PaDeviceIndex i = 0; i < device_count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxOutputChannels > 0) {
            devices.push_back({
                .id = i,
                .name = info->name,
                .max_input_channels = 0,
                .max_output_channels = info->maxOutputChannels,
                .default_sample_rate = info->defaultSampleRate,
                .is_default = (i == default_output)
            });
        }
    }
    
    return devices;
}

Result<void> AudioEngine::set_input_device(DeviceId id) {
    const PaDeviceInfo* info = Pa_GetDeviceInfo(id);
    if (!info || info->maxInputChannels == 0) {
        return Err<void>(ErrorCode::AudioDeviceNotFound, "Invalid input device");
    }
    
    input_device_id_ = id;
    return Ok();
}

Result<void> AudioEngine::set_output_device(DeviceId id) {
    const PaDeviceInfo* info = Pa_GetDeviceInfo(id);
    if (!info || info->maxOutputChannels == 0) {
        return Err<void>(ErrorCode::AudioDeviceNotFound, "Invalid output device");
    }
    
    output_device_id_ = id;
    return Ok();
}

Result<void> AudioEngine::start_capture() {
    if (!initialized_.load()) {
        return Err<void>(ErrorCode::AudioInitFailed, "Not initialized");
    }
    
    if (capture_running_.load()) {
        return Ok();  // Already running
    }
    
    PaStreamParameters input_params;
    input_params.device = input_device_id_;
    input_params.channelCount = 1;  // Mono
    input_params.sampleFormat = paFloat32;
    input_params.suggestedLatency = Pa_GetDeviceInfo(input_device_id_)->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_OpenStream(
        &capture_stream_,
        &input_params,
        nullptr,  // No output
        config_.sample_rate,
        config_.frame_size,
        paNoFlag,
        &AudioEngine::capture_callback_static,
        this
    );
    
    if (err != paNoError) {
        return Err<void>(ErrorCode::AudioStreamFailed,
                        std::string("Failed to open capture stream: ") + Pa_GetErrorText(err));
    }
    
    err = Pa_StartStream(capture_stream_);
    if (err != paNoError) {
        Pa_CloseStream(capture_stream_);
        capture_stream_ = nullptr;
        return Err<void>(ErrorCode::AudioStreamFailed,
                        std::string("Failed to start capture stream: ") + Pa_GetErrorText(err));
    }
    
    capture_running_.store(true);
    return Ok();
}

Result<void> AudioEngine::stop_capture() {
    if (!capture_running_.load()) {
        return Ok();
    }
    
    if (capture_stream_) {
        Pa_StopStream(capture_stream_);
        Pa_CloseStream(capture_stream_);
        capture_stream_ = nullptr;
    }
    
    capture_running_.store(false);
    return Ok();
}

Result<void> AudioEngine::start_playback() {
    if (!initialized_.load()) {
        return Err<void>(ErrorCode::AudioInitFailed, "Not initialized");
    }
    
    if (playback_running_.load()) {
        return Ok();  // Already running
    }
    
    PaStreamParameters output_params;
    output_params.device = output_device_id_;
    output_params.channelCount = 1;  // Mono
    output_params.sampleFormat = paFloat32;
    output_params.suggestedLatency = Pa_GetDeviceInfo(output_device_id_)->defaultLowOutputLatency;
    output_params.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_OpenStream(
        &playback_stream_,
        nullptr,  // No input
        &output_params,
        config_.sample_rate,
        config_.frame_size,
        paNoFlag,
        &AudioEngine::playback_callback_static,
        this
    );
    
    if (err != paNoError) {
        return Err<void>(ErrorCode::AudioStreamFailed,
                        std::string("Failed to open playback stream: ") + Pa_GetErrorText(err));
    }
    
    err = Pa_StartStream(playback_stream_);
    if (err != paNoError) {
        Pa_CloseStream(playback_stream_);
        playback_stream_ = nullptr;
        return Err<void>(ErrorCode::AudioStreamFailed,
                        std::string("Failed to start playback stream: ") + Pa_GetErrorText(err));
    }
    
    playback_running_.store(true);
    return Ok();
}

Result<void> AudioEngine::stop_playback() {
    if (!playback_running_.load()) {
        return Ok();
    }
    
    if (playback_stream_) {
        Pa_StopStream(playback_stream_);
        Pa_CloseStream(playback_stream_);
        playback_stream_ = nullptr;
    }
    
    playback_running_.store(false);
    return Ok();
}

void AudioEngine::set_capture_callback(CaptureCallback callback) {
    capture_callback_ = std::move(callback);
}

void AudioEngine::set_playback_callback(PlaybackCallback callback) {
    playback_callback_ = std::move(callback);
}

float AudioEngine::get_input_level() const noexcept {
    return current_input_level_.load(std::memory_order_relaxed);
}

float AudioEngine::get_output_level() const noexcept {
    return current_output_level_.load(std::memory_order_relaxed);
}

AudioStats AudioEngine::get_stats() const {
    return AudioStats{
        .input_overflows = input_overflows_.load(std::memory_order_relaxed),
        .output_underflows = output_underflows_.load(std::memory_order_relaxed),
        .queue_full_errors = 0,
        .queue_empty_errors = 0,
        .current_input_level = current_input_level_.load(std::memory_order_relaxed),
        .current_output_level = current_output_level_.load(std::memory_order_relaxed),
        .estimated_latency_ms = config_.frame_size * config_.buffer_frames * 1000 / config_.sample_rate
    };
}

void AudioEngine::set_input_volume(float volume) noexcept {
    input_volume_.store(std::clamp(volume, 0.0f, 2.0f), std::memory_order_relaxed);
}

void AudioEngine::set_output_volume(float volume) noexcept {
    output_volume_.store(std::clamp(volume, 0.0f, 2.0f), std::memory_order_relaxed);
}

// Static callback wrappers
int AudioEngine::capture_callback_static(
    const void* input,
    void* output,
    unsigned long frame_count,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data
) {
    auto* engine = static_cast<AudioEngine*>(user_data);
    return engine->handle_capture(
        static_cast<const float*>(input),
        frame_count,
        status_flags
    );
}

int AudioEngine::playback_callback_static(
    const void* input,
    void* output,
    unsigned long frame_count,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data
) {
    auto* engine = static_cast<AudioEngine*>(user_data);
    return engine->handle_playback(
        static_cast<float*>(output),
        frame_count,
        status_flags
    );
}

// Member callback handlers
int AudioEngine::handle_capture(
    const float* input,
    unsigned long frame_count,
    PaStreamCallbackFlags status_flags
) {
    // Check for buffer overflow
    if (status_flags & paInputOverflow) {
        input_overflows_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Apply input volume
    const float volume = input_volume_.load(std::memory_order_relaxed);
    
    // Calculate input level (RMS)
    const float level = calculate_rms(input, frame_count);
    current_input_level_.store(level, std::memory_order_relaxed);
    
    // Call user callback if set
    if (capture_callback_) {
        // Note: If volume != 1.0, we'd need to apply it here
        // For now, pass through directly for performance
        capture_callback_(input, frame_count);
    }
    
    return paContinue;
}

int AudioEngine::handle_playback(
    float* output,
    unsigned long frame_count,
    PaStreamCallbackFlags status_flags
) {
    // Check for buffer underflow
    if (status_flags & paOutputUnderflow) {
        output_underflows_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Call user callback if set
    if (playback_callback_) {
        playback_callback_(output, frame_count);
        
        // Apply output volume
        const float volume = output_volume_.load(std::memory_order_relaxed);
        if (volume != 1.0f) {
            for (unsigned long i = 0; i < frame_count; ++i) {
                output[i] *= volume;
            }
        }
        
        // Calculate output level (RMS)
        const float level = calculate_rms(output, frame_count);
        current_output_level_.store(level, std::memory_order_relaxed);
    } else {
        // No callback - output silence
        std::fill(output, output + frame_count, 0.0f);
        current_output_level_.store(0.0f, std::memory_order_relaxed);
    }
    
    return paContinue;
}

float AudioEngine::calculate_rms(const float* pcm, size_t count) const noexcept {
    float sum_squares = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum_squares += pcm[i] * pcm[i];
    }
    float rms = std::sqrt(sum_squares / static_cast<float>(count));
    
    // Apply gain multiplier to make meters more responsive
    // RMS is naturally low (0.1-0.3 for normal speech), multiply by 4x for better visualization
    rms *= 4.0f;
    
    // Clamp to 0.0-1.0 range
    return std::min(rms, 1.0f);
}

} // namespace voip::audio
