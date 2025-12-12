# 🔧 **PTT Transmission Diagnostic Guide**

## **Fixes Applied:**

### **Fix 1: PTT Now OVERRIDES Hot Mic** ✅
**Previous Behavior:**
- Hot mic on Channel 1 → Always transmitting to Channel 1
- Press F2 → Transmitting to BOTH Channel 1 and Channel 2 (combined)

**New Behavior:**
- Hot mic on Channel 1 → Transmitting to Channel 1
- Press F2 → **ONLY** transmitting to Channel 2 (PTT overrides hot mic)
- Release F2 → Resume transmitting to Channel 1

**Rationale:** PTT should be explicit control. When you press a hotkey, you want to transmit ONLY to that channel.

---

### **Fix 2: Comprehensive Audio Transmission Logging** ✅
Added detailed logging to diagnose transmission issues:

1. **Every ~1 second during audio capture:**
   ```
   📡 Transmit targets: Channels: 1 | Hot mic: off | PTT: 1
   ```

2. **When PTT starts:**
   ```
   🎤 PTT started for channel 2 | Active PTT channels now: 2 | Hot mic: 1
   ```

3. **When PTT stops:**
   ```
   🔇 PTT stopped for channel 2 | Remaining PTT channels: (none) | Hot mic: 1
   ```

4. **If muted:**
   ```
   ⚠️ Audio muted - not transmitting (frame 101)
   ```

5. **If no targets:**
   ```
   📡 Transmit targets: (none - will drop audio)
   ```

---

## **How to Diagnose PTT Issues:**

### **Step 1: Run the Client**
```powershell
cd c:\dev\VoIP-System\client\build\Debug
.\voip-client.exe
```

### **Step 2: Watch Console Output**

#### **Expected on Startup:**
```
✅ Voice session started successfully!
📢 Auto-joined Channel 1 (General) for listening
🎤 Press F1-F4 to transmit to channels
```

#### **Expected During Idle (no PTT, no hot mic):**
```
🎤 Capturing audio: frame 101
📡 Transmit targets: (none - will drop audio)  ← Normal! Not transmitting
```

---

### **Step 3: Test Hot Mic**

**Action:** Click "🎤 Transmit" button on Channel 1

**Expected Console Output:**
```
🎤 Hot mic set to channel 1
```

**During audio capture (~1 second later):**
```
🎤 Capturing audio: frame 151
📡 Transmit targets: Channels: 1 | Hot mic: 1 | PTT: none
```

**✅ If you see this:** Hot mic is working, audio is being transmitted to Channel 1

**❌ If you see:**
```
📡 Transmit targets: (none - will drop audio)
```
**Problem:** Hot mic not set correctly. Check that `set_hot_mic_channel(1)` was called.

---

### **Step 4: Test PTT Without Hot Mic**

**Action:** 
1. Disable hot mic (click "🎤 Transmit" again if it's on)
2. Press and hold F1

**Expected Console Output:**
```
⌨️ F1 pressed - Starting PTT for channel 1
  ✅ Widget updated - transmit button should turn orange
  📡 Channel 1 joined for listening: YES
  🎤 Active PTT channels: 1
🎤 PTT started for channel 1 | Active PTT channels now: 1 | Hot mic: off
```

**During audio capture (~1 second later):**
```
🎤 Capturing audio: frame 201
📡 Transmit targets: Channels: 1 | Hot mic: off | PTT: 1
```

**✅ If you see this:** PTT is working correctly!

**❌ If you see:**
```
📡 Transmit targets: (none - will drop audio)
```
**Possible causes:**
1. **PTT not added to ptt_channels_** → Check if `start_ptt()` was called
2. **HotkeyManager not triggering** → Check if hotkey is registered
3. **Signal not connected** → Check MainWindow::onHotkeyPressed connection

---

### **Step 5: Test PTT Overriding Hot Mic**

**Action:**
1. Enable hot mic on Channel 1 (click "🎤 Transmit" button)
2. Press and hold F2

**Expected Console Output:**
```
🎤 Hot mic set to channel 1
⌨️ F2 pressed - Starting PTT for channel 2
🎤 PTT started for channel 2 | Active PTT channels now: 2 | Hot mic: 1
```

**During audio capture (~1 second later):**
```
📡 Transmit targets: Channels: 2 | Hot mic: 1 | PTT: 2
```
**Key:** Transmit target is Channel **2 ONLY** (not both 1 and 2)

**Release F2:**
```
⌨️ F2 released - Stopping PTT for channel 2
🔇 PTT stopped for channel 2 | Remaining PTT channels: (none) | Hot mic: 1
```

**After release:**
```
📡 Transmit targets: Channels: 1 | Hot mic: 1 | PTT: none
```
**Key:** Now back to hot mic Channel 1

**✅ If you see this:** PTT override is working correctly!

**❌ If you see both channels in targets:**
```
📡 Transmit targets: Channels: 1 2 | ...
```
**Problem:** Code is still combining instead of overriding. Check audio_captured() logic.

---

### **Step 6: Test Multi-PTT**

**Action:**
1. Disable hot mic
2. Press and hold F1
3. While holding F1, also press F2

**Expected Console Output:**
```
⌨️ F1 pressed - Starting PTT for channel 1
🎤 PTT started for channel 1 | Active PTT channels now: 1 | Hot mic: off

⌨️ F2 pressed - Starting PTT for channel 2
🎤 PTT started for channel 2 | Active PTT channels now: 1 2 | Hot mic: off
```

**During audio capture:**
```
📡 Transmit targets: Channels: 1 2 | Hot mic: off | PTT: 1 2
```

**Release F1 (keep holding F2):**
```
⌨️ F1 released - Stopping PTT for channel 1
🔇 PTT stopped for channel 1 | Remaining PTT channels: 2 | Hot mic: off
```

**After F1 release:**
```
📡 Transmit targets: Channels: 2 | Hot mic: off | PTT: 2
```

**✅ If you see this:** Multi-PTT is working correctly!

---

## **Common Issues:**

### **Issue 1: PTT Not Working at All**

**Symptoms:**
- Press F1-F4, nothing happens
- No "⌨️ F1 pressed" in console

**Diagnosis:**
1. Check if HotkeyManager is initialized:
   ```
   ✅ HotkeyManager initialized
   ⌨️ Registered hotkey F1 for channel 1
   ```

2. If missing, check that `setupMultiChannelUI()` was called

3. If present but not responding:
   - Check if event filter is installed
   - Check if another widget is consuming key events
   - Try clicking on the main window to give it focus

---

### **Issue 2: PTT Starts But No Audio Transmitted**

**Symptoms:**
```
⌨️ F1 pressed - Starting PTT for channel 1
🎤 PTT started for channel 1 | Active PTT channels now: 1 | Hot mic: off
📡 Transmit targets: Channels: 1 | Hot mic: off | PTT: 1
```
**BUT** server doesn't receive packets

**Possible Causes:**

**A. Microphone Not Capturing:**
Check for:
```
🎤 Capturing audio: frame 101
```
If NOT appearing → Audio engine not started or microphone not working

**B. Muted:**
Check for:
```
⚠️ Audio muted - not transmitting (frame 101)
```
If appearing → Click "Mute" button to unmute

**C. Encoding Failure:**
Would see encode errors increase in stats

**D. Network Failure:**
Check UDP socket is connected:
```
🔌 UDP socket connected to 127.0.0.1:9001
```

---

### **Issue 3: Audio Transmitted to Wrong Channel**

**Symptoms:**
- Press F2, but server receives packets for Channel 1

**Diagnosis:**
Look at the "Transmit targets" line:
```
📡 Transmit targets: Channels: X | ...
```

The `Channels: X` should match the PTT channel you pressed.

**If it doesn't match:**
- Check that the correct channel ID is passed to `start_ptt()`
- Check that hotkey → channel mapping is correct
- Verify `target_channels` logic in audio callback

---

### **Issue 4: PTT and Hot Mic Both Transmitting**

**Symptoms:**
- Hot mic on Channel 1
- Press F2
- Server receives packets for BOTH Channel 1 and Channel 2

**Diagnosis:**
```
📡 Transmit targets: Channels: 1 2 | Hot mic: 1 | PTT: 2
```

**If you see both channels:** The old "combine" logic is still running.

**Fix:** Verify the audio callback logic:
```cpp
if (!ptt_targets.empty()) {
    // PTT active → use ONLY PTT channels
    target_channels = ptt_targets;
} else if (hot_mic != 0) {
    // No PTT active → use hot mic channel
    target_channels.insert(hot_mic);
}
```

---

## **Server-Side Verification:**

### **What to Check:**

**When PTT to Channel 2:**
```
📦 Voice packet: seq=XXX, ch=2, user=42, payload=XXB
🔊 Routed voice from user 42 to N recipients in channel 2
```

**Key points:**
- `ch=2` should match the PTT channel
- Sequence numbers should be incrementing
- Payload size should be ~40-80 bytes (Opus encoded)

**❌ Bad signs:**
- `ch=1` when you pressed F2 → Wrong channel in packet
- No packets arriving → Network issue
- Packets from multiple UDP ports → Multiple client instances running

---

## **Success Criteria:**

✅ **Hot mic works:** Audio continuously transmitted to hot mic channel  
✅ **PTT works:** Audio transmitted when hotkey pressed  
✅ **PTT overrides hot mic:** Only PTT channel receives audio when active  
✅ **Multi-PTT works:** Multiple simultaneous hotkeys transmit to multiple channels  
✅ **Clean switching:** Release PTT → resume hot mic transmission  
✅ **Console logging:** All state changes visible in logs  
✅ **Server receives correct packets:** Channel ID matches PTT target  

---

## **Quick Reference:**

### **Good Log Patterns:**

**Idle (no transmission):**
```
📡 Transmit targets: (none - will drop audio)
```

**Hot mic only:**
```
📡 Transmit targets: Channels: 1 | Hot mic: 1 | PTT: none
```

**PTT only:**
```
📡 Transmit targets: Channels: 2 | Hot mic: off | PTT: 2
```

**PTT overriding hot mic:**
```
📡 Transmit targets: Channels: 2 | Hot mic: 1 | PTT: 2
```

**Multi-PTT:**
```
📡 Transmit targets: Channels: 1 2 3 | Hot mic: off | PTT: 1 2 3
```

---

## **Next Steps:**

1. **Run the client** with these new fixes
2. **Copy-paste console output** showing the issue
3. **Share server logs** showing what packets are received
4. I'll analyze and pinpoint the exact problem! 🔍

