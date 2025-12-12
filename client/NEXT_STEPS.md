# Next Development Steps - VoIP Client

## 🎯 **Immediate Priorities (This Week)**

### 1. Fix WebSocket User List ⭐⭐⭐ (HIGH)
**Issue**: User list only shows yourself  
**Root Cause**: "Unknown message type: 0" - parser issue or timing problem

**Options**:
```cpp
// Option A: Move WebSocket init after event loop
mainWindow->show();
QTimer::singleShot(100, [&]() {
    // Initialize WebSocket here
    auto wsClient = std::make_shared<network::WebSocketClient>();
    wsClient->connect(...);
    mainWindow->setWebSocketClient(wsClient);
});

// Option B: Fix message parser in websocket_client.cpp
// Check handle_message() for type 0 handling

// Option C: Auto-populate from voice packets (bypass WebSocket entirely)
void VoiceSession::on_packet_received(const VoicePacket& packet) {
    if (packet.user_id != my_user_id && !user_list.contains(packet.user_id)) {
        emit userDiscovered(packet.user_id);  // Add to UI
    }
}
```

**Files to Check**:
- `client/src/network/websocket_client.cpp` - Message parser
- `client/src/ui_main.cpp` - WebSocket initialization
- `client/include/protocol/control_messages.h` - Message types

---

### 2. Implement Mute/Deafen Control ⭐⭐ (MEDIUM)
**Status**: UI exists but not wired to audio

**Implementation**:
```cpp
// In VoiceSession class
class VoiceSession {
public:
    void set_muted(bool muted) { is_muted_ = muted; }
    void set_deafened(bool deafened) { is_deafened_ = deafened; }
    
private:
    std::atomic<bool> is_muted_{false};
    std::atomic<bool> is_deafened_{false};
    
    void on_audio_captured(const float* pcm, size_t frames) {
        if (is_muted_) return;  // Don't encode/send when muted
        // ... existing code ...
    }
    
    void on_audio_playback_needed(float* pcm, size_t frames) {
        if (is_deafened_) {
            std::fill_n(pcm, frames, 0.0f);  // Silence output
            return;
        }
        // ... existing code ...
    }
};

// In MainWindow::updateMicrophoneState()
void MainWindow::updateMicrophoneState() {
    if (voiceSession_) {
        voiceSession_->set_muted(isMuted_);
        voiceSession_->set_deafened(isDeafened_);
    }
}
```

**Files to Edit**:
- `client/include/session/voice_session.h` - Add mute/deafen methods
- `client/src/session/voice_session.cpp` - Implement mute/deafen logic
- `client/src/ui/main_window.cpp` - Wire UI to session

---

### 3. Add Push-to-Talk (PTT) ⭐⭐ (MEDIUM)
**Status**: UI shows PTT indicator but doesn't actually work

**Implementation**:
```cpp
// Already have in MainWindow:
// - isPushToTalkMode_ flag
// - isPushToTalkActive_ flag
// - keyPressEvent/keyReleaseEvent handlers
// - pttKey_ (default Qt::Key_V)

// Just need to wire to mute control:
void MainWindow::setPushToTalkActive(bool active) {
    isPushToTalkActive_ = active;
    
    if (isPushToTalkMode_) {
        isMuted_ = !active;  // Unmute when PTT pressed, mute when released
        updateMicrophoneState();  // This will call voiceSession_->set_muted()
    }
}
```

**Files to Edit**:
- `client/src/ui/main_window.cpp` - Wire PTT to mute

**Testing**:
1. Enable PTT mode
2. Hold V key → should unmute + "TRANSMITTING" indicator
3. Release V → should mute again

---

## 🚀 **Next Features (This Month)**

### 4. Audio Device Selection ⭐⭐ (MEDIUM)
**Current**: Uses default audio devices  
**Needed**: Let user choose input/output devices

**UI Design**:
```
Settings Dialog:
┌─────────────────────────────────┐
│ Audio Settings                  │
├─────────────────────────────────┤
│ Input Device:  [Dropdown ▼]    │
│   Microphone (Realtek)          │
│   Steam Streaming Microphone    │
│                                 │
│ Output Device: [Dropdown ▼]    │
│   Speakers (Realtek)            │
│   Steam Streaming Speakers      │
│                                 │
│ [Test Input] [Test Output]      │
│                                 │
│         [Apply] [Cancel]        │
└─────────────────────────────────┘
```

**Implementation**:
```cpp
// AudioEngine already has:
std::vector<AudioDevice> enumerate_input_devices();
std::vector<AudioDevice> enumerate_output_devices();
Result<void> set_input_device(DeviceId id);
Result<void> set_output_device(DeviceId id);

// Just need UI:
class SettingsDialog : public QDialog {
    QComboBox* inputDeviceCombo_;
    QComboBox* outputDeviceCombo_;
    
    void populateDevices() {
        auto inputDevices = audioEngine_->enumerate_input_devices();
        for (const auto& dev : inputDevices) {
            inputDeviceCombo_->addItem(dev.name, dev.id);
        }
    }
};
```

---

### 5. Multi-Channel Support ⭐⭐⭐ (HIGH)
**Current**: Single channel only  
**Goal**: Listen to 10+ channels simultaneously

**Architecture Changes Needed**:
```cpp
class MultiChannelSession {
    // One VoiceSession per channel
    std::map<ChannelId, std::shared_ptr<VoiceSession>> sessions_;
    
    // Audio mixer for playback
    AudioMixer mixer_;
    
    // Per-channel mute/volume
    struct ChannelState {
        bool muted;
        float volume;      // 0.0-1.0
        int priority;      // For audio ducking
    };
    std::map<ChannelId, ChannelState> channel_states_;
};

class AudioMixer {
    // Combine multiple audio streams
    void mix(const std::vector<float*>& inputs, float* output, size_t frames);
    
    // Apply ducking: lower volume of lower-priority channels
    void apply_ducking(/* ... */);
};
```

**UI Changes**:
```
Channel List (with checkboxes):
☑ Command (Priority: 1) [80%] [PTT: F1]
☑ Squad    (Priority: 2) [60%] [PTT: F2]
☑ Proximity(Priority: 3) [40%] [PTT: F3]
☐ Logistics(Priority: 4) [--]  [PTT: --]
```

---

### 6. Configuration File ⭐ (LOW)
**Current**: Hardcoded settings  
**Needed**: Save/load user preferences

**Format** (`config.json`):
```json
{
  "audio": {
    "input_device_id": 2,
    "output_device_id": 1,
    "sample_rate": 48000,
    "frame_size": 960
  },
  "network": {
    "server_address": "127.0.0.1",
    "server_port": 9000,
    "udp_port": 9001
  },
  "voice": {
    "bitrate": 32000,
    "enable_fec": true,
    "jitter_buffer_frames": 5
  },
  "hotkeys": {
    "ptt_global": "V",
    "mute_toggle": "M",
    "deafen_toggle": "D"
  },
  "ui": {
    "theme": "dark",
    "remember_credentials": false
  }
}
```

---

## 🔧 **Technical Debt & Improvements**

### 7. Error Handling Improvements
**Issues**:
- Some errors not logged with enough context
- No user-visible error messages for network failures
- Audio errors silently increment counters

**Fixes**:
```cpp
// Better logging
LOG_ERROR("Failed to encode audio", 
    "frame", frame_count,
    "error", encode_result.error().message());

// User notifications
void MainWindow::showError(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
    addLogMessage("❌ " + message);
}
```

---

### 8. Unit Tests
**Current**: Minimal test coverage  
**Needed**: Tests for all core components

**Priority Tests**:
```cpp
// Audio engine tests
TEST(AudioEngine, InitializeAndShutdown)
TEST(AudioEngine, EnumerateDevices)
TEST(AudioEngine, CapturePlaybackLoopback)

// Opus codec tests
TEST(OpusCodec, EncodeDecodeRoundtrip)
TEST(OpusCodec, PacketLossConcealment)

// Jitter buffer tests
TEST(JitterBuffer, OrderingAndResequencing)
TEST(JitterBuffer, UnderrunRecovery)

// Network tests
TEST(UdpSocket, SendReceive)
TEST(UdpSocket, PacketLoss)
```

---

### 9. Performance Profiling
**Measure**:
- Audio callback execution time (target: <1ms)
- Encoding/decoding latency
- Network round-trip time
- Jitter buffer depth over time

**Tools**:
- Windows Performance Analyzer (WPA)
- Visual Studio Profiler
- Custom timing logs

---

## 📋 **Long-Term Features**

### 10. Audio Ducking/Priority System
- Lower volume of low-priority channels when high-priority speaks
- Configurable ducking curves
- Smooth volume transitions

### 11. Operation Presets
- Save/load channel configurations
- Quick setup for complex scenarios
- Share presets with team

### 12. Voice Activity Detection (VAD)
- Auto-mute when not speaking (optional)
- Reduce bandwidth when silent
- Visual indicator for who's talking

### 13. Noise Suppression
- RNNoise integration
- Configurable aggressiveness
- Toggle on/off per channel

### 14. Echo Cancellation
- For users without headphones
- Critical for large operations

---

## 🎯 **Success Criteria**

### Week 1
- ✅ User list shows all connected users
- ✅ Mute/deafen actually controls audio
- ✅ PTT works with keyboard hotkey

### Week 2
- ✅ Audio device selection working
- ✅ Settings dialog implemented
- ✅ Configuration file save/load

### Week 3
- ✅ Multi-channel foundation
- ✅ Channel mixer basic implementation
- ✅ Per-channel volume control

### Month 1
- ✅ 2-3 channels working simultaneously
- ✅ Unit test coverage >60%
- ✅ Basic audio ducking

---

## 🚀 **Let's Start!**

**Recommended Order**:
1. Fix WebSocket user list (unblocks testing)
2. Wire mute/deafen (critical UX)
3. Enable PTT (most requested feature)
4. Add device selection (common user need)
5. Start multi-channel work (core differentiator)

**Which would you like to tackle first?**
