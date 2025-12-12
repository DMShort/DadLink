# Enhanced Multi-Channel UI - Rocker Switch Design

**Date:** November 24, 2025  
**Feature:** Per-Channel Listen/Transmit Toggles + Hotkey Support

---

## 🎯 **User Requirements**

1. **Rocker switch controls** for each channel (listen + transmit toggles)
2. **Easy toggle** - Click to enable/disable listening to a channel
3. **Transmit selection** - Select which channel to transmit to with hot mic
4. **Hotkey assignment** - Assign hotkeys for channel-specific PTT
5. **Visual clarity** - Immediately see which channels are active

---

## 🎨 **UI Design**

### **Channel List Widget (Enhanced)**

```
╔═══════════════════════════════════════════════╗
║  📢 CHANNELS                                  ║
╠═══════════════════════════════════════════════╣
║                                               ║
║  # General                        🎧 ON  🎤 1 ║
║  [Listen: ON] [Transmit: Channel 1] [F1]     ║
║  ──────────────────────────────────────────   ║
║                                               ║
║  # Gaming                         🎧 ON  🎤 - ║
║  [Listen: ON] [Transmit: OFF]     [F2]       ║
║  ──────────────────────────────────────────   ║
║                                               ║
║  # Music                          🎧 OFF 🎤 - ║
║  [Listen: OFF] [Transmit: OFF]    [F3]       ║
║  ──────────────────────────────────────────   ║
║                                               ║
║  # Dev                            🎧 -   🎤 - ║
║  [Not Joined]                     [--]       ║
║                                               ║
╚═══════════════════════════════════════════════╝
```

### **Rocker Switch Design**

Each channel has 3 states for listening:
- **🎧 ON** (Bright green) - Listening to this channel
- **🎧 MUTED** (Gray with slash) - Joined but muted
- **🎧 OFF** (Gray) - Not joined to this channel

Each channel has transmit indicator:
- **🎤 1** (Bright blue) - Hot mic transmits here
- **🎤 PTT** (Yellow) - PTT-only channel (via hotkey)
- **🎤 -** (Gray) - Not transmitting to this channel

---

## 🎮 **User Interactions**

### **Listening Control**

| Action | Result |
|--------|--------|
| **Click "Listen" toggle** | Join/leave channel (toggle listening) |
| **Click 🎧 icon** | Mute/unmute channel (stay joined, don't hear) |
| **Double-click channel name** | Join channel + set as transmit |

### **Transmit Control**

| Action | Result |
|--------|--------|
| **Click "Transmit" toggle** | Set as hot mic channel |
| **Click 🎤 icon** | Quick-set transmit channel |
| **Hold hotkey (F1-F12)** | PTT to that specific channel |
| **Right-click → Assign Hotkey** | Configure channel hotkey |

### **Hotkey System**

```
HOT MIC (Always Transmitting):
- User selects ONE channel as "hot mic"
- Microphone always transmits to this channel
- Shown as: 🎤 1

PTT CHANNELS (Push-to-Talk):
- User assigns hotkeys (F1-F12) to channels
- Hold hotkey → transmit to that channel ONLY
- Release → stop transmitting to that channel
- Shown as: 🎤 PTT (when hotkey held)

COMBINED MODE:
- Hot mic channel ALWAYS active
- Hold F2 → ALSO transmit to Gaming channel
- Release F2 → back to just hot mic
```

---

## 🏗️ **Architecture**

### **Client State Management**

```cpp
// MainWindow state
struct ChannelState {
    ChannelId channel_id;
    QString name;
    bool joined;           // User has joined this channel
    bool listening;        // User is hearing this channel (can mute while joined)
    bool hot_mic;          // Hot mic transmits to this channel
    QKeySequence hotkey;   // PTT hotkey for this channel
    bool ptt_active;       // Currently holding PTT for this channel
};

std::map<ChannelId, ChannelState> channels_;
ChannelId hot_mic_channel_ = 0;  // Which channel has hot mic
std::set<ChannelId> ptt_active_channels_;  // Channels with PTT currently held
```

### **VoiceSession Integration**

```cpp
// VoiceSession tracks:
std::set<ChannelId> listening_channels_;      // Channels to receive from
std::map<ChannelId, bool> channel_muted_;     // Per-channel mute state
ChannelId hot_mic_channel_;                   // Hot mic target
std::set<ChannelId> ptt_channels_;            // Active PTT channels

// Audio capture logic:
void on_audio_captured(const float* pcm, size_t frames) {
    // Encode once
    auto opus_packet = encode(pcm, frames);
    
    // Transmit to hot mic channel
    if (hot_mic_channel_ != 0) {
        send_voice_packet(hot_mic_channel_, opus_packet);
    }
    
    // Transmit to all active PTT channels
    for (auto channel_id : ptt_channels_) {
        send_voice_packet(channel_id, opus_packet);
    }
}

// Audio playback logic:
void on_audio_playback_needed(float* output, size_t frames) {
    memset(output, 0, frames * sizeof(float));
    
    for (auto channel_id : listening_channels_) {
        // Skip muted channels
        if (channel_muted_[channel_id]) continue;
        
        float temp[960];
        if (channel_buffers_[channel_id].get_frame(temp, frames)) {
            for (size_t i = 0; i < frames; i++) {
                output[i] += temp[i];
                output[i] = std::clamp(output[i], -1.0f, 1.0f);
            }
        }
    }
}
```

---

## 🎛️ **Widget Implementation**

### **ChannelWidget.h**

```cpp
class ChannelWidget : public QWidget {
    Q_OBJECT
    
public:
    ChannelWidget(ChannelId id, const QString& name, QWidget* parent = nullptr);
    
    void setJoined(bool joined);
    void setListening(bool listening);
    void setHotMic(bool active);
    void setPTTActive(bool active);
    void setHotkey(const QKeySequence& key);
    
signals:
    void listenToggled(ChannelId id, bool listen);
    void transmitSelected(ChannelId id);
    void hotkeyChangeRequested(ChannelId id);
    void muteToggled(ChannelId id);
    
private:
    ChannelId channel_id_;
    QString name_;
    
    // UI Elements
    QLabel* nameLabel_;
    QPushButton* listenButton_;      // 🎧 Listen toggle
    QPushButton* transmitButton_;    // 🎤 Transmit selector
    QLabel* hotkeyLabel_;            // [F1] hotkey display
    QLabel* statusLabel_;            // ON/OFF/MUTED
    
    bool joined_ = false;
    bool listening_ = false;
    bool hot_mic_ = false;
    bool ptt_active_ = false;
};
```

### **Visual States**

```cpp
// Listen button states
if (!joined_) {
    listenButton_->setStyleSheet("background: #2f3136; color: #72767d;");
    listenButton_->setText("🎧 --");
} else if (listening_) {
    listenButton_->setStyleSheet("background: #43b581; color: white;");
    listenButton_->setText("🎧 ON");
} else {
    listenButton_->setStyleSheet("background: #4f545c; color: #dcddde;");
    listenButton_->setText("🎧 MUTED");
}

// Transmit button states
if (hot_mic_) {
    transmitButton_->setStyleSheet("background: #5865f2; color: white;");
    transmitButton_->setText("🎤 HOT MIC");
} else if (ptt_active_) {
    transmitButton_->setStyleSheet("background: #faa61a; color: white;");
    transmitButton_->setText("🎤 PTT");
} else {
    transmitButton_->setStyleSheet("background: #2f3136; color: #72767d;");
    transmitButton_->setText("🎤 --");
}
```

---

## ⌨️ **Hotkey System**

### **HotkeyManager.h**

```cpp
class HotkeyManager : public QObject {
    Q_OBJECT
    
public:
    HotkeyManager(QWidget* parent = nullptr);
    
    bool registerHotkey(ChannelId channel_id, const QKeySequence& key);
    void unregisterHotkey(ChannelId channel_id);
    QKeySequence getHotkey(ChannelId channel_id) const;
    
signals:
    void hotkeyPressed(ChannelId channel_id);
    void hotkeyReleased(ChannelId channel_id);
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    
private:
    std::map<ChannelId, QKeySequence> channel_hotkeys_;
    std::map<Qt::Key, ChannelId> key_to_channel_;
    std::set<Qt::Key> pressed_keys_;
};
```

### **Global Hotkey Implementation**

```cpp
bool HotkeyManager::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        Qt::Key key = static_cast<Qt::Key>(keyEvent->key());
        
        // Check if this key is mapped to a channel
        if (key_to_channel_.contains(key)) {
            if (!pressed_keys_.contains(key)) {
                pressed_keys_.insert(key);
                emit hotkeyPressed(key_to_channel_[key]);
                return true;  // Consume event
            }
        }
    } else if (event->type() == QEvent::KeyRelease) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        Qt::Key key = static_cast<Qt::Key>(keyEvent->key());
        
        if (key_to_channel_.contains(key)) {
            pressed_keys_.erase(key);
            emit hotkeyReleased(key_to_channel_[key]);
            return true;
        }
    }
    
    return QObject::eventFilter(obj, event);
}
```

---

## 📊 **User Scenarios**

### **Scenario 1: Hot Mic with PTT Overlay**

```
Setup:
- User joins General, Gaming, Music
- User sets General as hot mic
- User assigns F2 to Gaming

Behavior:
1. User speaks → transmits to General (hot mic)
2. User holds F2 → transmits to BOTH General AND Gaming
3. User releases F2 → back to just General
4. User can toggle listening to Music without affecting transmit
```

### **Scenario 2: PTT-Only Mode**

```
Setup:
- User joins General, Gaming
- NO hot mic set
- User assigns F1 to General, F2 to Gaming

Behavior:
1. User speaks → no transmission (no hot mic)
2. User holds F1 → transmits to General only
3. User holds F2 → transmits to Gaming only
4. User holds F1+F2 → transmits to BOTH simultaneously
```

### **Scenario 3: Selective Listening**

```
Setup:
- User joins General, Gaming, Music, Dev
- User listening to all 4 channels (overwhelming)

Solution:
1. Click 🎧 on Music → mutes Music (stays joined, doesn't hear)
2. Click 🎧 on Dev → mutes Dev
3. Now only hearing General + Gaming
4. Can quickly unmute by clicking 🎧 again
```

---

## 🔧 **Implementation Plan**

### **Phase 1: Core Backend** (2 hours)

1. **VoiceSession Multi-Channel**
   - Add `listening_channels_` set
   - Add `channel_muted_` map
   - Add `hot_mic_channel_` and `ptt_channels_`
   - Implement audio mixing with mute support

2. **WebSocket Updates**
   - Add `set_transmit_channel()` method
   - Track joined channels locally
   - Handle channel join/leave responses

### **Phase 2: Hotkey System** (1.5 hours)

1. **HotkeyManager Class**
   - Global event filter for key press/release
   - Map hotkeys to channels
   - Emit signals for PTT start/stop

2. **MainWindow Integration**
   - Connect hotkey signals to VoiceSession
   - Update UI when PTT active
   - Persist hotkey settings

### **Phase 3: UI Widgets** (2 hours)

1. **ChannelWidget**
   - Rocker switches for listen/transmit
   - Visual state indicators
   - Hotkey display

2. **Hotkey Configuration Dialog**
   - Click to assign hotkey
   - Validate no conflicts
   - Show all assignments

### **Phase 4: Testing** (0.5 hours)

1. Multi-channel mixing
2. PTT with multiple channels
3. Hot mic + PTT overlay
4. Selective muting

---

## 🎯 **Success Criteria**

✅ User can toggle listening to each channel independently  
✅ User can set ONE hot mic channel  
✅ User can assign hotkeys to channels for PTT  
✅ User can hold multiple PTT keys simultaneously  
✅ Audio mixing works with muted channels excluded  
✅ Visual feedback clearly shows active channels  
✅ Hotkeys work globally (even when not focused)  

---

## 🚀 **Next Steps**

1. ✏️ Implement VoiceSession multi-channel backend
2. ✏️ Add audio mixing with per-channel mute
3. ✏️ Implement HotkeyManager
4. ✏️ Create ChannelWidget with rocker switches
5. ✏️ Add hotkey configuration UI
6. ✏️ Test all scenarios

**Estimated Time:** 6 hours total  
**Complexity:** Medium-High (hotkey system is tricky)

---

**Status:** 📝 Design Complete - Ready for Implementation  
**Priority:** High - Core user experience feature
