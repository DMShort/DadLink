# Audio Engineering Specification
## Gaming-Focused Multi-Channel VoIP System

---

## Audio Pipeline Overview

```
Microphone → [Capture] → [Encode] → [Encrypt] → [Network] → [Server Router]
                                                                     ↓
Speaker ← [Playback] ← [Mix] ← [Decode] ← [Decrypt] ← [Network] ← [Forward]
```

---

## Audio Configuration

### Sample Rate
**Recommended**: 48000 Hz

**Rationale:**
- Native Opus sample rate (no resampling)
- Standard for professional audio
- Supported by virtually all hardware

**Alternatives:**
- 44100 Hz (CD quality, requires resampling)
- 16000 Hz (narrowband, lower quality, saves bandwidth)

### Frame Size
**Recommended**: 20ms (960 samples @ 48kHz)

**Rationale:**
- Good balance between latency and efficiency
- Opus frame size sweet spot
- Typical VoIP standard

**Alternatives:**
- 10ms: Lower latency, higher overhead, more CPU
- 40ms: Higher latency, better efficiency, less CPU

### Bit Depth
**Recommended**: 32-bit float (internal processing)

**Rationale:**
- No clipping during mixing
- Headroom for audio ducking
- Opus operates on float internally

**Hardware I/O**: 16-bit or 24-bit (converted to float)

---

## Audio Capture

### Device Selection

```cpp
std::vector<AudioDevice> enumerate_input_devices() {
    std::vector<AudioDevice> devices;
    
    PaDeviceIndex count = Pa_GetDeviceCount();
    for (PaDeviceIndex i = 0; i < count; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info->maxInputChannels > 0) {
            devices.push_back({
                .id = i,
                .name = info->name,
                .max_channels = info->maxInputChannels,
                .default_sample_rate = info->defaultSampleRate
            });
        }
    }
    
    return devices;
}
```

### Capture Callback (Real-Time Safe)

```cpp
int capture_callback(
    const void* input_buffer,
    void* output_buffer,
    unsigned long frame_count,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data
) {
    AudioEngine* engine = static_cast<AudioEngine*>(user_data);
    const float* in = static_cast<const float*>(input_buffer);
    
    // Check for buffer underrun/overrun
    if (status_flags & paInputOverflow) {
        engine->stats.input_overflows++;
    }
    
    // Copy to lock-free queue (pre-allocated)
    if (!engine->capture_queue.try_push(in, frame_count)) {
        engine->stats.queue_full_errors++;
    }
    
    return paContinue;
}
```

**Key Points:**
- NO malloc/free
- NO mutexes
- NO blocking I/O
- Fast copy to lock-free queue
- Error tracking

---

## Opus Encoding

### Encoder Configuration

```cpp
OpusEncoder* create_voice_encoder() {
    int error;
    OpusEncoder* encoder = opus_encoder_create(
        48000,                      // Sample rate
        1,                          // Mono (reduce bandwidth)
        OPUS_APPLICATION_VOIP,      // Voice optimized
        &error
    );
    
    if (error != OPUS_OK) {
        return nullptr;
    }
    
    // Configure for low latency
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(32000));      // 32 kbps
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));      // Max quality
    opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(encoder, OPUS_SET_DTX(1));              // Enable DTX
    opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));       // Forward error correction
    opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(5)); // Expect 5% loss
    
    return encoder;
}
```

### Encoding Process

```cpp
EncodedPacket encode_audio(OpusEncoder* encoder, const float* pcm, size_t frames) {
    EncodedPacket packet;
    packet.data.resize(4000);  // Max Opus packet size
    
    int encoded_bytes = opus_encode_float(
        encoder,
        pcm,
        frames,
        packet.data.data(),
        packet.data.size()
    );
    
    if (encoded_bytes < 0) {
        // Error handling
        return {};
    }
    
    packet.data.resize(encoded_bytes);
    packet.frame_size = frames;
    
    // Detect if DTX (silence) packet
    packet.is_dtx = (encoded_bytes <= 3);
    
    return packet;
}
```

### Bitrate Adaptation

```cpp
void adapt_bitrate(OpusEncoder* encoder, NetworkStats stats) {
    // Increase bitrate if network is good
    if (stats.packet_loss < 0.01 && stats.jitter_ms < 10) {
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(64000));  // High quality
    }
    // Reduce bitrate if network is poor
    else if (stats.packet_loss > 0.05 || stats.jitter_ms > 50) {
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(24000));  // Low bitrate
    }
    // Default: balanced
    else {
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(32000));
    }
}
```

---

## Opus Decoding

### Decoder Configuration

```cpp
OpusDecoder* create_voice_decoder() {
    int error;
    OpusDecoder* decoder = opus_decoder_create(
        48000,  // Sample rate
        1,      // Mono
        &error
    );
    
    return (error == OPUS_OK) ? decoder : nullptr;
}
```

### Decoding Process

```cpp
DecodedAudio decode_audio(OpusDecoder* decoder, const uint8_t* opus_data, size_t opus_len) {
    DecodedAudio audio;
    audio.pcm.resize(5760);  // Max frame size (120ms @ 48kHz)
    
    int decoded_samples = opus_decode_float(
        decoder,
        opus_data,
        opus_len,
        audio.pcm.data(),
        audio.pcm.size(),
        0  // Decode normally (not FEC)
    );
    
    if (decoded_samples < 0) {
        // Error - use PLC
        return decode_plc(decoder);
    }
    
    audio.pcm.resize(decoded_samples);
    return audio;
}
```

### Packet Loss Concealment (PLC)

```cpp
DecodedAudio decode_plc(OpusDecoder* decoder) {
    DecodedAudio audio;
    audio.pcm.resize(960);  // 20ms frame
    
    // Decode without packet (Opus synthesizes audio)
    int decoded_samples = opus_decode_float(
        decoder,
        nullptr,  // No packet data
        0,        // Zero length
        audio.pcm.data(),
        audio.pcm.size(),
        0
    );
    
    audio.pcm.resize(decoded_samples);
    audio.is_plc = true;
    
    return audio;
}
```

**PLC Quality:**
- First lost packet: Nearly transparent
- 2-3 consecutive: Slight degradation
- 5+ consecutive: Noticeable but intelligible
- 10+ consecutive: Severe degradation

---

## Jitter Buffer

### Purpose
Smooth out network jitter by buffering and reordering packets.

### Implementation

```cpp
class JitterBuffer {
private:
    std::map<uint64_t, VoicePacket> buffer_;  // Ordered by sequence
    uint64_t next_seq_ = 0;
    Duration target_delay_ = Duration::from_ms(30);
    std::chrono::steady_clock::time_point first_packet_time_;
    
public:
    void push_packet(VoicePacket packet) {
        // Discard duplicates
        if (buffer_.count(packet.sequence) > 0) {
            stats_.duplicates++;
            return;
        }
        
        // Discard very late packets
        if (packet.sequence < next_seq_) {
            stats_.late_packets++;
            return;
        }
        
        // Store packet
        buffer_[packet.sequence] = packet;
        
        // Record first packet time
        if (buffer_.size() == 1) {
            first_packet_time_ = std::chrono::steady_clock::now();
        }
    }
    
    Option<VoicePacket> pop_packet() {
        // Wait for target delay before starting playout
        if (buffer_.empty()) {
            return None;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - first_packet_time_;
        
        if (elapsed < target_delay_) {
            return None;  // Not ready yet
        }
        
        // Pop next expected packet
        auto it = buffer_.find(next_seq_);
        if (it != buffer_.end()) {
            VoicePacket packet = it->second;
            buffer_.erase(it);
            next_seq_++;
            return Some(packet);
        }
        
        // Gap detected - use PLC for missing packet
        stats_.gaps++;
        next_seq_++;
        return None;
    }
    
    void set_target_delay(Duration delay) {
        target_delay_ = delay;
    }
};
```

### Adaptive Jitter Buffer

```cpp
class AdaptiveJitterBuffer : public JitterBuffer {
private:
    std::deque<Duration> jitter_history_;
    
    void update_target_delay() {
        // Calculate 95th percentile jitter
        std::vector<Duration> sorted = jitter_history_;
        std::sort(sorted.begin(), sorted.end());
        
        size_t idx = sorted.size() * 0.95;
        Duration p95_jitter = sorted[idx];
        
        // Set target delay to 2x p95 jitter (with min/max bounds)
        Duration new_target = p95_jitter * 2;
        new_target = std::clamp(new_target, 
            Duration::from_ms(20),   // Min 20ms
            Duration::from_ms(100)   // Max 100ms
        );
        
        set_target_delay(new_target);
    }
};
```

---

## Multi-Channel Mixing

### Mixer Architecture

```cpp
class AudioMixer {
private:
    struct ChannelStream {
        ChannelId channel;
        UserId user;
        std::vector<float> pcm;
        float volume = 1.0;
        bool ducked = false;
    };
    
    std::vector<ChannelStream> streams_;
    std::map<ChannelId, float> channel_volumes_;
    std::map<ChannelId, float> ducking_amounts_;  // dB reduction
    
public:
    void add_stream(ChannelId channel, UserId user, const float* pcm, size_t samples) {
        streams_.push_back({
            .channel = channel,
            .user = user,
            .pcm = std::vector<float>(pcm, pcm + samples),
            .volume = channel_volumes_[channel]
        });
    }
    
    void mix_to_output(float* output, size_t samples) {
        // Zero output buffer
        std::fill(output, output + samples, 0.0f);
        
        // Determine if any priority channel is active
        bool priority_active = false;
        for (const auto& stream : streams_) {
            if (is_priority_channel(stream.channel)) {
                priority_active = true;
                break;
            }
        }
        
        // Mix all streams
        for (auto& stream : streams_) {
            // Apply ducking if needed
            float gain = stream.volume;
            if (priority_active && !is_priority_channel(stream.channel)) {
                float duck_db = ducking_amounts_[stream.channel];
                gain *= std::pow(10.0f, duck_db / 20.0f);  // dB to linear
            }
            
            // Mix with gain
            for (size_t i = 0; i < samples && i < stream.pcm.size(); i++) {
                output[i] += stream.pcm[i] * gain;
            }
        }
        
        // Soft clipping (prevent harsh distortion)
        for (size_t i = 0; i < samples; i++) {
            output[i] = soft_clip(output[i]);
        }
        
        // Clear streams for next mix
        streams_.clear();
    }
    
private:
    float soft_clip(float x) {
        // Tanh soft clipping
        if (x > 1.0f || x < -1.0f) {
            return std::tanh(x);
        }
        return x;
    }
};
```

### Audio Ducking Rules

**Example Configuration:**
```json
{
  "ducking_rules": [
    {
      "priority_channel": "Command",
      "affected_channels": ["Flight", "Ground", "Logistics"],
      "reduction_db": -12
    },
    {
      "priority_channel": "Flight",
      "affected_channels": ["AlphaWing", "BetaWing"],
      "reduction_db": -6
    }
  ]
}
```

---

## Playback

### Playback Callback (Real-Time Safe)

```cpp
int playback_callback(
    const void* input_buffer,
    void* output_buffer,
    unsigned long frame_count,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data
) {
    AudioEngine* engine = static_cast<AudioEngine*>(user_data);
    float* out = static_cast<float*>(output_buffer);
    
    // Check for buffer underrun
    if (status_flags & paOutputUnderflow) {
        engine->stats.output_underflows++;
    }
    
    // Get mixed audio from lock-free queue
    if (engine->playback_queue.try_pop(out, frame_count)) {
        // Apply output volume
        for (size_t i = 0; i < frame_count; i++) {
            out[i] *= engine->output_volume;
        }
    } else {
        // No audio available - output silence
        std::fill(out, out + frame_count, 0.0f);
        engine->stats.queue_empty_errors++;
    }
    
    return paContinue;
}
```

---

## Audio Quality Optimization

### Latency Budget

| Component | Target | Acceptable | Notes |
|-----------|--------|------------|-------|
| Capture Buffer | 10ms | 20ms | 480 samples @ 48kHz |
| Encoding | 2ms | 5ms | Opus @ complexity 10 |
| Network (one-way) | 30ms | 80ms | Depends on geography |
| Jitter Buffer | 30ms | 60ms | Adaptive |
| Decoding | 2ms | 5ms | Opus decode |
| Playback Buffer | 10ms | 20ms | 480 samples |
| **Total** | **84ms** | **190ms** | Target < 150ms |

### Echo Cancellation

**Options:**
1. **WebRTC AEC** - Integrate WebRTC's acoustic echo cancellation
2. **Speex AEC** - Lightweight alternative
3. **Encourage headsets** - Hardware solution (preferred)

**Implementation (WebRTC AEC):**
```cpp
#include <webrtc/modules/audio_processing/include/audio_processing.h>

webrtc::AudioProcessing* apm = webrtc::AudioProcessing::Create();
apm->echo_cancellation()->Enable(true);
apm->echo_cancellation()->enable_drift_compensation(true);

// In capture callback
apm->ProcessStream(&audio_frame);
```

### Noise Suppression

**RNNoise Integration:**
```cpp
#include <rnnoise.h>

DenoiseState* denoiser = rnnoise_create(nullptr);

void process_audio_with_noise_suppression(float* audio, size_t samples) {
    // RNNoise expects 480 samples (10ms @ 48kHz)
    for (size_t offset = 0; offset < samples; offset += 480) {
        rnnoise_process_frame(denoiser, audio + offset, audio + offset);
    }
}
```

---

## Performance Considerations

### CPU Usage Targets
- **Capture Thread**: <5% CPU
- **Encoding**: <2% per stream
- **Decoding**: <2% per stream  
- **Mixing**: <3% for 10 channels
- **Playback Thread**: <5% CPU
- **Total Target**: <25% single core for typical use (3-5 channels)

### Memory Usage
- **Audio Buffers**: ~100 KB (pre-allocated)
- **Per-Stream Decoders**: ~50 KB each
- **Jitter Buffers**: ~10 KB per source
- **Total**: <5 MB for typical use

---

See `ARCHITECTURE.md` for system overview and `CLIENT_DESIGN.md` for integration details.
