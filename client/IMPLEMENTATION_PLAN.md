# Implementation Plan: Mute/Deafen + PTT

## 📋 Analysis Summary

### Current State
✅ **UI Already Working**:
- MainWindow has `isMuted_` and `isDeafened_` flags
- Mute/Deafen/PTT buttons update UI state correctly
- PTT key events (V key) already trigger `setPushToTalkActive()`
- `updateMicrophoneState()` is called in all the right places

❌ **Missing Audio Control**:
- VoiceSession has NO mute/deafen methods
- Audio callbacks don't check mute/deafen state
- `updateMicrophoneState()` has TODO - doesn't actually control audio

### Audio Callbacks (Real-Time Threads!)
1. **`on_audio_captured()`** (line 214, voice_session.cpp)
   - Runs in audio thread
   - Currently: Always captures, encodes, and sends
   - Needs: Early return if muted

2. **`on_audio_playback_needed()`** (line 295, voice_session.cpp)
   - Runs in audio thread
   - Currently: Always plays audio from jitter buffer
   - Needs: Fill with silence if deafened

## 🎯 Implementation Plan

### Step 1: Add Mute/Deafen to VoiceSession Header
**File**: `client/include/session/voice_session.h`

**Changes**:
```cpp
public:
    // Add after get_output_level() around line 89
    /**
     * Control microphone mute state
     * @param muted true to mute microphone (stop transmitting)
     */
    void set_muted(bool muted) noexcept;
    
    /**
     * Control speaker deafen state
     * @param deafened true to deafen (stop receiving)
     */
    void set_deafened(bool deafened) noexcept;
    
    /**
     * Get current mute/deafen state
     */
    [[nodiscard]] bool is_muted() const noexcept;
    [[nodiscard]] bool is_deafened() const noexcept;

private:
    // Add after active_ around line 144
    std::atomic<bool> is_muted_{false};
    std::atomic<bool> is_deafened_{false};
```

### Step 2: Implement Mute/Deafen in VoiceSession
**File**: `client/src/session/voice_session.cpp`

**Changes**:

A. Add methods after `get_output_level()` (around line 180):
```cpp
void VoiceSession::set_muted(bool muted) noexcept {
    is_muted_ = muted;
}

void VoiceSession::set_deafened(bool deafened) noexcept {
    is_deafened_ = deafened;
}

bool VoiceSession::is_muted() const noexcept {
    return is_muted_;
}

bool VoiceSession::is_deafened() const noexcept {
    return is_deafened_;
}
```

B. Modify `on_audio_captured()` (line 214):
```cpp
void VoiceSession::on_audio_captured(const float* pcm, size_t frames) {
    // Early return if muted - don't transmit
    if (is_muted_) {
        return;
    }
    
    // ... rest of existing code ...
```

C. Modify `on_audio_playback_needed()` (line 295):
```cpp
void VoiceSession::on_audio_playback_needed(float* pcm, size_t frames) {
    // If deafened, output silence
    if (is_deafened_) {
        std::fill(pcm, pcm + frames, 0.0f);
        return;
    }
    
    // ... rest of existing code ...
```

### Step 3: Wire MainWindow to VoiceSession
**File**: `client/src/ui/main_window.cpp`

**Changes**: Implement `updateMicrophoneState()` (line 495):
```cpp
void MainWindow::updateMicrophoneState() {
    if (!voiceSession_) {
        return;  // No session yet
    }
    
    // Update voice session with current mute/deafen state
    voiceSession_->set_muted(isMuted_);
    voiceSession_->set_deafened(isDeafened_);
    
    // Log state changes for debugging
    if (isMuted_ && isDeafened_) {
        statusBar()->showMessage("Voice: Muted & Deafened");
    } else if (isDeafened_) {
        statusBar()->showMessage("Voice: Deafened");
    } else if (isMuted_) {
        statusBar()->showMessage("Voice: Muted");
    } else {
        statusBar()->showMessage("Voice: Connected");
    }
}
```

## ✅ PTT Already Works!

The PTT implementation is already complete:
- ✅ Key press/release handlers exist
- ✅ `setPushToTalkActive()` updates `isMuted_` flag
- ✅ Calls `updateMicrophoneState()` which will now work

**No additional changes needed for PTT!**

## 🧪 Testing Plan

### Test 1: Mute Button
1. Start client, join channel
2. Click "Mute" button
3. Expected: Microphone stops transmitting, input meter shows "Muted"
4. Speak into microphone
5. Expected: No audio transmitted, other users don't hear you

### Test 2: Deafen Button  
1. Start client, join channel
2. Have another user speak
3. Click "Deafen" button
4. Expected: Output meter goes to 0, no audio playback, mute button auto-checked

### Test 3: Push-to-Talk
1. Start client, join channel
2. Enable PTT mode
3. Expected: Microphone muted, indicator shows "Press V to Talk"
4. Hold V key
5. Expected: Indicator shows "TRANSMITTING" in red, microphone unmuted
6. Release V key
7. Expected: Back to "Press V to Talk", microphone muted

### Test 4: PTT with Deafen
1. Enable PTT mode
2. Enable Deafen
3. Hold V key
4. Expected: Can transmit but can't hear responses

## 🚀 No Conflicts!

This implementation:
- ✅ Adds NEW methods only (no modifications to existing APIs)
- ✅ Uses atomic flags (thread-safe)
- ✅ Follows real-time audio safety rules (no allocations, no locks)
- ✅ Leverages existing UI infrastructure
- ✅ No breaking changes

**Ready to implement!**
