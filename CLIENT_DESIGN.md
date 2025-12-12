# Client Design Specification
## Gaming-Focused Multi-Channel VoIP System

---

## Module Overview

### Client Architecture Layers

```
┌─────────────────────────────────────────┐
│         UI LAYER (Qt)                   │
│  Radio Stack | Settings | Overlay       │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────┴───────────────────────┐
│      APPLICATION LOGIC LAYER            │
│  Channel Router | Hotkey | Config       │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────┴───────────────────────┐
│       AUDIO ENGINE LAYER                │
│  Capture | Mixer | Playback             │
└─────────────────┬───────────────────────┘
                  ↓
┌─────────────────┴───────────────────────┐
│      CODEC & NETWORK LAYER              │
│  Opus | UDP | TLS | Jitter Buffer       │
└─────────────────────────────────────────┘
```

---

## Audio Engine Module

### Responsibilities
- Enumerate audio devices
- Low-latency capture from microphone
- Low-latency playback to speakers
- Sample rate conversion if needed
- Real-time thread management

### API Design

```cpp
class AudioEngine {
public:
    Result<void> initialize(const AudioConfig& config);
    std::vector<AudioDevice> enumerate_input_devices();
    std::vector<AudioDevice> enumerate_output_devices();
    Result<void> set_input_device(DeviceId id);
    Result<void> set_output_device(DeviceId id);
    
    void start_capture();
    void stop_capture();
    void start_playback();
    void stop_playback();
    
    void set_capture_callback(CaptureCallback cb);
    void set_playback_callback(PlaybackCallback cb);
    
    float get_input_level();
    float get_output_level();
    AudioStats get_stats();
};
```

### Configuration

```cpp
struct AudioConfig {
    uint32_t sample_rate = 48000;      // Hz
    uint32_t frame_size = 480;         // samples (10ms @ 48kHz)
    uint32_t buffer_frames = 3;        // 30ms total buffering
    float input_volume = 1.0;          // 0.0-2.0
    float output_volume = 0.8;         // 0.0-2.0
};
```

### Real-Time Safety Rules
1. **NO** heap allocation in audio callbacks
2. **NO** blocking operations (mutex, I/O, sleep)
3. **NO** unbounded loops
4. Use lock-free queues for cross-thread communication
5. Pre-allocate all buffers during initialization

---

## Codec Layer Module

### Opus Encoder/Decoder

```cpp
class OpusCodec {
public:
    static Result<OpusEncoder*> create_encoder(
        uint32_t sample_rate,
        uint32_t channels,
        uint32_t bitrate
    );
    
    Result<EncodedPacket> encode(const float* pcm, size_t frames);
    void set_bitrate(uint32_t bitrate);
    void enable_dtx(bool enable);
    
    static Result<OpusDecoder*> create_decoder(
        uint32_t sample_rate,
        uint32_t channels
    );
    
    Result<size_t> decode(
        const uint8_t* opus_data,
        size_t opus_size,
        float* pcm_out
    );
    
    Result<size_t> decode_plc(float* pcm_out);  // Packet loss concealment
};
```

### Recommended Settings

| Setting | Value | Rationale |
|---------|-------|-----------|
| Sample Rate | 48000 Hz | Native Opus, best quality |
| Frame Size | 20ms (960 samples) | Balance latency/efficiency |
| Bitrate | 24-64 kbps | Voice optimized |
| Complexity | 8-10 | Best quality |
| Application | VOIP | Voice optimized |
| DTX | Enabled | Save bandwidth on silence |
| FEC | Enabled | Packet loss resilience |

---

## Network Layer Module

### Voice Packet Structure

```
┌─────────────────────────────────────────┐
│ Header (28 bytes)                       │
├─────────────────────────────────────────┤
│ Magic (4)      │ 0xVOIP0001            │
│ Sequence (8)   │ Monotonic counter     │
│ Timestamp (8)  │ Microseconds          │
│ ChannelID (4)  │ Target channel        │
│ UserID (4)     │ Sender                │
├─────────────────────────────────────────┤
│ Encrypted Payload (variable)            │
├─────────────────────────────────────────┤
│ - Opus Frame (40-80 bytes)              │
│ - Energy Level (1 byte)                 │
└─────────────────────────────────────────┘
```

### Jitter Buffer

```cpp
class JitterBuffer {
public:
    void push_packet(VoicePacket packet);
    Option<VoicePacket> pop_packet();
    void set_target_delay(Duration delay);  // 20-40ms typical
    BufferStats get_stats();
};

struct BufferStats {
    uint32_t late_packets;
    uint32_t duplicates;
    uint32_t gaps;
    uint32_t current_delay_ms;
};
```

### Control Messages (WebSocket/JSON)

```json
{
  "type": "join_channel",
  "channel_id": 12345,
  "password": null
}

{
  "type": "channel_state",
  "channel_id": 12345,
  "users": [
    {"id": 1, "name": "User1", "speaking": false},
    {"id": 2, "name": "User2", "speaking": true}
  ]
}
```

---

## Channel Router & Mixer Module

### Multi-Channel Mixer

```cpp
class AudioMixer {
public:
    void add_stream(ChannelId channel, UserId user, 
                   const float* pcm, size_t samples);
    void mix_to_output(float* output, size_t samples);
    void set_channel_volume(ChannelId channel, float volume);
    void set_ducking_rule(ChannelId priority_channel, float duck_db);
    void clear_streams();
};
```

### Ducking Example
- Command channel active → reduce other channels by -12dB
- Flight leader speaking → reduce squadron by -6dB
- User-configurable per operation

---

## Hotkey Manager Module

```cpp
class HotkeyManager {
public:
    Result<HotkeyId> register_hotkey(KeyCombination keys, 
                                     HotkeyCallback cb);
    void unregister_hotkey(HotkeyId id);
    void set_channel_ptt(ChannelId channel, KeyCombination keys);
    void set_simulcast_ptt(std::vector<ChannelId> channels, 
                           KeyCombination keys);
};
```

### Example Bindings
- `Ctrl+1` → PTT on Command
- `Ctrl+2` → PTT on Squadron
- `Ctrl+Shift+1` → Simulcast to Command + Squadron
- `Ctrl+M` → Mute all
- `Ctrl+D` → Deafen

---

## UI Components

### Radio Stack View (Main Window)
- Vertical channel list
- Per-channel: name, user count, active speakers
- PTT indicator (current TX channel)
- Audio level meters
- Quick join/leave buttons

### Settings Dialog
- Audio device selection
- Volume sliders
- Hotkey configuration
- Server management
- Operation preset loader

### In-Game Overlay
- Minimal display: TX channel + speakers
- Draggable, resizable
- Configurable opacity

---

## Threading Model

| Thread | Priority | Purpose | RT Safe? |
|--------|----------|---------|----------|
| Main/UI | Normal | Qt event loop | No |
| Audio Capture | Real-Time | Mic input | YES |
| Audio Playback | Real-Time | Speaker output | YES |
| Encoding | High | Opus encode, UDP send | No |
| Decoding | High | UDP receive, Opus decode | No |
| Network I/O | Normal | TLS control | No |
| Mixer | High | Multi-channel mix | Mostly |

---

## Configuration File

```json
{
  "version": "1.0",
  "audio": {
    "input_device": "Microphone (USB)",
    "output_device": "Speakers (Realtek)",
    "input_volume": 1.0,
    "output_volume": 0.8
  },
  "servers": [{
    "name": "My Org",
    "address": "voip.org.com:9000",
    "username": "pilot_1"
  }],
  "hotkeys": {
    "command_ptt": "Ctrl+1",
    "flight_ptt": "Ctrl+2"
  }
}
```

See `ARCHITECTURE.md` for system overview.
