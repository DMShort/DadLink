# Mute/Deafen/PTT Testing Guide

## ✅ What Was Implemented

### VoiceSession (Audio Control)
- ✅ `set_muted(bool)` - Controls microphone transmission
- ✅ `set_deafened(bool)` - Controls audio playback
- ✅ `is_muted()` / `is_deafened()` - Query current state
- ✅ Audio capture checks mute flag before encoding/transmitting
- ✅ Audio playback outputs silence when deafened

### MainWindow (UI Integration)
- ✅ `updateMicrophoneState()` now wires to VoiceSession
- ✅ Status bar shows current voice state
- ✅ Called automatically when buttons clicked or PTT toggled

### Thread Safety
- ✅ Uses `std::atomic<bool>` for flags (no locks needed)
- ✅ Audio callbacks remain real-time safe
- ✅ No heap allocations or blocking operations

---

## 🧪 Test Procedures

### Test 1: Mute Button
**Setup**: Start 2 clients, both in same channel

**Steps**:
1. Client A: Speak into microphone
2. Client B: Should hear Client A's voice
3. Client A: Click "🔇 Mute" button
4. Client A: Continue speaking
5. Client B: Should NOT hear Client A anymore

**Expected Results**:
- ✅ Client A button changes to "🔊 Unmute"
- ✅ Client A input meter shows "Muted"
- ✅ Client A status bar: "Voice: Muted"
- ✅ Client B receives no audio from Client A
- ✅ Client B's output meter for Client A goes to 0

**Verification**:
- Console should still show "🎤 Capturing audio" (capture continues)
- Console should NOT show encoding/sending when muted

---

### Test 2: Deafen Button
**Setup**: Start 2 clients, both in same channel

**Steps**:
1. Client A: Speak into microphone
2. Client B: Should hear Client A
3. Client B: Click "🔇 Deafen" button
4. Client A: Continue speaking
5. Client B: Should NOT hear anything

**Expected Results**:
- ✅ Client B button changes to "🔔 Undeafen"
- ✅ Client B output meter goes to 0
- ✅ Client B mute button is auto-checked and disabled
- ✅ Client B status bar: "Voice: Muted & Deafened"
- ✅ Client B hears nothing

**Verification**:
- Console should still show "📥 Received packet" (reception continues)
- Jitter buffer still fills, but playback is silenced

---

### Test 3: Push-to-Talk (PTT)
**Setup**: Start 2 clients, both in same channel

**Steps**:
1. Client A: Click "PTT Mode" toggle
2. Client A: Speak normally
3. Client B: Should NOT hear Client A
4. Client A: Hold V key
5. Client A: Speak while holding V
6. Client B: Should hear Client A
7. Client A: Release V key
8. Client A: Speak normally again
9. Client B: Should NOT hear Client A anymore

**Expected Results**:
- ✅ Step 1: Indicator shows "🎤 Voice: Press V to Talk" (yellow)
- ✅ Step 1: Status bar: "Voice: PTT Ready"
- ✅ Step 1: Mute button disabled
- ✅ Step 4: Indicator shows "🔴 TRANSMITTING" (red background)
- ✅ Step 4: Console: "🔴 Push-to-Talk: ACTIVE"
- ✅ Step 7: Indicator back to "Press V to Talk"
- ✅ Step 7: Console: "⚪ Push-to-Talk: Released"

---

### Test 4: Mute + Deafen Interaction
**Setup**: Start 1 client

**Steps**:
1. Click "Mute" button
2. Click "Deafen" button
3. Observe state
4. Click "Undeafen"
5. Observe state

**Expected Results**:
- ✅ Step 1: Status bar "Voice: Muted"
- ✅ Step 2: Status bar "Voice: Muted & Deafened"
- ✅ Step 2: Mute button disabled
- ✅ Step 4: Status bar back to "Voice: Muted"
- ✅ Step 4: Mute button enabled again
- ✅ Mute state preserved through deafen/undeafen

---

### Test 5: PTT + Deafen Combo
**Setup**: Start 2 clients

**Steps**:
1. Client A: Enable PTT mode
2. Client A: Enable Deafen
3. Client A: Hold V key and speak
4. Client B: Speak continuously
5. Verify Client A can transmit but can't hear responses

**Expected Results**:
- ✅ Client A transmits when holding V
- ✅ Client B hears Client A
- ✅ Client A does NOT hear Client B
- ✅ Client A's output meter stays at 0

---

### Test 6: Rapid Toggling
**Setup**: Start 1 client

**Steps**:
1. Rapidly click Mute button 10 times
2. Rapidly toggle PTT mode 5 times
3. Hold/release V key rapidly 10 times
4. Verify no crashes or audio glitches

**Expected Results**:
- ✅ No crashes
- ✅ No audio dropouts
- ✅ State remains consistent
- ✅ Status bar updates correctly

---

### Test 7: State Persistence Through Mode Changes
**Setup**: Start 1 client

**Steps**:
1. Click Mute
2. Enable PTT mode
3. Disable PTT mode
4. Check if still muted

**Expected Results**:
- ✅ When leaving PTT mode, mute state should be restored to unmuted
- ✅ (Current behavior: PTT mode unmutes when disabled)

---

## 🔍 Debug Checks

### Console Output to Watch For

**When Muted**:
```
🎤 Capturing audio: frame 100
(NO encoding/sending messages)
```

**When Unmuted**:
```
🎤 Capturing audio: frame 100
(Encoding and sending should occur)
```

**When Deafened**:
```
📥 Received packet: seq=123 ch=1 user=42
(NO playback, but packets still received)
```

**PTT Active**:
```
🔴 Push-to-Talk: ACTIVE
🎤 Capturing audio: frame 100
(Encoding/sending resumes)
```

**PTT Released**:
```
⚪ Push-to-Talk: Released
🎤 Capturing audio: frame 100
(NO encoding/sending)
```

---

## 📊 Success Criteria

### Mute ✅
- [ ] Stops audio transmission when enabled
- [ ] Other users can't hear you
- [ ] Input meter shows "Muted"
- [ ] Can be toggled on/off smoothly

### Deafen ✅
- [ ] Stops audio playback when enabled
- [ ] You can't hear others
- [ ] Output meter shows 0
- [ ] Auto-enables mute

### PTT ✅
- [ ] V key hold transmits audio
- [ ] V key release stops transmission
- [ ] Visual indicator changes state
- [ ] Works consistently

### Integration ✅
- [ ] All features work together
- [ ] No race conditions
- [ ] Status bar accurate
- [ ] No memory leaks or crashes

---

## 🐛 Known Limitations

1. **Mute still captures audio**: Capture continues but transmission stops (by design - reduces latency when unmuting)

2. **Deafen still receives packets**: Network reception continues but playback silenced (by design - jitter buffer stays primed)

3. **PTT overrides mute state**: When leaving PTT mode, mute is cleared (could be improved to remember pre-PTT state)

---

## 🚀 Ready to Test!

Start the server and run the client:
```powershell
# Terminal 1: Start server
cd c:\dev\VoIP-System\server
cargo run

# Terminal 2: Build and run first client
cd c:\dev\VoIP-System\client
.\build_and_deploy.bat
.\build\Debug\voip-client.exe

# Terminal 3: Run second client
cd c:\dev\VoIP-System\client
.\start_second_client.bat
```

**Test each scenario systematically!**
