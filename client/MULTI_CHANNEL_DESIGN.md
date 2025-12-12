# Multi-Channel VoIP Architecture

## 🎯 Goal
Enable users to listen to 10+ voice channels simultaneously with independent control over each channel.

## 📋 Requirements

### Core Features
1. **Simultaneous Channels**: Listen to multiple channels at once
2. **Per-Channel Control**:
   - Individual volume (0-200%)
   - Individual mute toggle
   - PTT assignment (different keys per channel)
   - Priority level (for audio ducking)
3. **Audio Mixing**: Combine multiple audio streams intelligently
4. **Audio Ducking**: Lower volume of low-priority channels when high-priority speaks

### UI Requirements
1. Channel list shows all channels with:
   - Join/Leave toggle
   - Volume slider
   - Mute button
   - PTT key indicator
   - Speaking indicator
2. Status shows which channels are active
3. Visual priority indicators

---

## 🏗️ Architecture

### Current Single-Channel Flow
```
Microphone → AudioEngine → Opus Encode → UDP Send → Server
Server → UDP Receive → Jitter Buffer → Opus Decode → AudioEngine → Speakers
```

### New Multi-Channel Flow
```
Microphone → AudioEngine → Opus Encode → [Channel 1 UDP]
                                       → [Channel 2 UDP]  (if transmitting to multiple)
                                       → [Channel N UDP]

[Channel 1 UDP] → Jitter Buffer → Opus Decode → ]
[Channel 2 UDP] → Jitter Buffer → Opus Decode → ] → AudioMixer → AudioEngine → Speakers
[Channel N UDP] → Jitter Buffer → Opus Decode → ]
```

---

## 📦 New Components

### 1. MultiChannelSession
**Purpose**: Manage multiple VoiceSession instances

```cpp
class MultiChannelSession {
public:
    struct ChannelConfig {
        ChannelId channel_id;
        float volume = 1.0f;          // 0.0-2.0
        bool muted = false;
        int priority = 0;              // 0=lowest, 10=highest
        Qt::Key ptt_key = Qt::Key_unknown;
    };
    
    // Channel management
    Result<void> join_channel(const ChannelConfig& config);
    void leave_channel(ChannelId channel_id);
    
    // Per-channel controls
    void set_channel_volume(ChannelId id, float volume);
    void set_channel_muted(ChannelId id, bool muted);
    void set_channel_priority(ChannelId id, int priority);
    
    // PTT
    void set_ptt_active(ChannelId id, bool active);
    
private:
    // One session per channel
    std::map<ChannelId, std::shared_ptr<VoiceSession>> sessions_;
    
    // Channel states
    std::map<ChannelId, ChannelConfig> channel_configs_;
    
    // Audio mixer
    std::unique_ptr<AudioMixer> mixer_;
    
    // Shared audio engine (single capture/playback stream)
    std::shared_ptr<AudioEngine> audio_engine_;
};
```

### 2. AudioMixer
**Purpose**: Combine multiple audio streams with volume/ducking

```cpp
class AudioMixer {
public:
    struct ChannelStream {
        ChannelId id;
        const float* samples;
        size_t sample_count;
        float volume;        // 0.0-2.0
        int priority;        // 0-10
        bool speaking;       // Is someone speaking?
    };
    
    /**
     * Mix multiple channels into single output
     * Applies volume, ducking, and prevents clipping
     */
    void mix(
        const std::vector<ChannelStream>& inputs,
        float* output,
        size_t frame_count
    );
    
    /**
     * Apply audio ducking based on priority
     * Lower-priority channels reduced when higher-priority speaks
     */
    void apply_ducking(std::vector<ChannelStream>& streams);
    
private:
    // Ducking config
    float ducking_amount_ = 0.5f;  // Reduce to 50% when ducked
    float ducking_threshold_ = 0.1f; // RMS threshold for "speaking"
    
    // Smooth volume changes to avoid clicks
    std::map<ChannelId, float> smoothed_volumes_;
    float volume_smooth_factor_ = 0.95f;  // IIR filter coefficient
};
```

---

## 🔄 Modified Components

### VoiceSession Changes
```cpp
class VoiceSession {
public:
    // NEW: Set channel-specific callback for mixed audio
    void set_playback_mixer_callback(
        std::function<void(float*, size_t)> callback
    );
    
    // NEW: Get decoded audio samples (for mixer)
    bool get_decoded_audio(float* buffer, size_t frames);
    
private:
    // NEW: Queue of decoded audio for mixer
    LockFreeQueue<AudioBuffer> decoded_audio_queue_;
};
```

### UI Changes

**ChannelListWidget** (new):
```cpp
class ChannelListWidget : public QWidget {
    Q_OBJECT
    
public:
    void addChannel(const ChannelInfo& info);
    void updateChannelState(ChannelId id, bool joined, bool speaking);
    
signals:
    void channelJoinRequested(ChannelId id);
    void channelLeaveRequested(ChannelId id);
    void channelVolumeChanged(ChannelId id, float volume);
    void channelMuteToggled(ChannelId id, bool muted);
    
private:
    struct ChannelWidget {
        QLabel* nameLabel;
        QPushButton* joinButton;
        QSlider* volumeSlider;
        QPushButton* muteButton;
        QLabel* speakingIndicator;
    };
    
    std::map<ChannelId, ChannelWidget*> channels_;
};
```

---

## 🎨 Implementation Phases

### Phase 1: Foundation (Current)
- ✅ Single VoiceSession working
- ✅ Mute/Deafen controls
- ✅ PTT functionality
- ✅ Audio device selection

### Phase 2: Audio Mixer
1. Create `AudioMixer` class
2. Implement basic mixing (sum channels)
3. Add volume control per channel
4. Test with 2 channels

**Files to Create**:
- `client/include/audio/audio_mixer.h`
- `client/src/audio/audio_mixer.cpp`

### Phase 3: Multi-Channel Session
1. Create `MultiChannelSession` class
2. Manage multiple `VoiceSession` instances
3. Wire sessions to mixer
4. Test with 2-3 channels

**Files to Create**:
- `client/include/session/multi_channel_session.h`
- `client/src/session/multi_channel_session.cpp`

### Phase 4: UI Integration
1. Create `ChannelListWidget`
2. Update `MainWindow` to use `MultiChannelSession`
3. Add per-channel controls to UI
4. Test full flow

**Files to Modify**:
- `client/include/ui/main_window.h`
- `client/src/ui/main_window.cpp`
- `client/src/ui_main.cpp`

### Phase 5: Audio Ducking
1. Implement priority system
2. Add ducking algorithm
3. Smooth volume transitions
4. Test with varying priorities

### Phase 6: Polish
1. PTT per channel
2. Channel-specific notifications
3. Performance optimization
4. Stress test with 10+ channels

---

## 🧪 Testing Strategy

### Test 1: Two Channels
```
User joins Channel 1 and Channel 2
Channel 1: User A speaking
Channel 2: User B speaking
Expected: Hear both A and B simultaneously
```

### Test 2: Volume Control
```
Channel 1 volume: 100%
Channel 2 volume: 50%
Expected: Channel 1 louder than Channel 2
```

### Test 3: Per-Channel Mute
```
Mute Channel 1
Expected: Only hear Channel 2
```

### Test 4: Audio Ducking
```
Channel 1 priority: 10 (command)
Channel 2 priority: 5 (squad)
User speaks in Channel 1
Expected: Channel 2 volume reduced to 50%
```

---

## ⚠️ Challenges & Solutions

### Challenge 1: Audio Latency
**Problem**: Each channel adds latency  
**Solution**: 
- Share single AudioEngine (one capture/playback stream)
- Mix in real-time thread (no locks)
- Use lock-free queues

### Challenge 2: CPU Usage
**Problem**: 10 channels = 10x opus decode  
**Solution**:
- Decode in separate threads
- Only decode active channels
- Use PLC for silence periods

### Challenge 3: Clipping
**Problem**: Summing 10 channels can exceed [-1.0, 1.0]  
**Solution**:
- Normalize output (divide by channel count)
- Apply soft-clipping (tanh)
- Use limiter if needed

### Challenge 4: Thread Safety
**Problem**: Multiple sessions, one mixer, one audio thread  
**Solution**:
- Lock-free queues for audio data
- Atomic flags for state
- Mixer runs in audio callback (RT-safe)

---

## 📊 Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| **Channels** | 10+ | Simultaneously listening |
| **CPU Usage** | <20% | On modern CPU (4+ cores) |
| **Latency** | <200ms | Total end-to-end |
| **Memory** | <200MB | For 10 channels |
| **Audio Quality** | No clicks/pops | Smooth mixing |

---

## 🚀 Next Steps

**Immediate** (This session):
1. Create AudioMixer class
2. Implement basic mixing algorithm
3. Write unit tests for mixer
4. Integrate with single VoiceSession (proof of concept)

**Next Session**:
1. Create MultiChannelSession
2. Test with 2 channels
3. Add UI controls

**Future**:
1. Audio ducking
2. PTT per channel
3. Performance optimization

---

**Current Status**: ✅ Phase 1 Complete, Starting Phase 2 (Audio Mixer)
