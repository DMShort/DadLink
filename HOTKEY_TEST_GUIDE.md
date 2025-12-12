# 🎮 **Multi-Channel Hotkey System - Test Guide**

## **What Was Fixed:**

### **Fix 1: Auto-Join Channel 1** ✅
- **Problem:** Channels were created with hotkeys, but user wasn't joined to any channel
- **Impact:** PTT hotkeys didn't work because no channels were active
- **Solution:** Automatically join Channel 1 (General) when voice session starts

### **Fix 2: Enhanced Logging** ✅
- Added comprehensive debug output for every hotkey press/release
- Shows joined channels, active PTT channels, and widget states
- Makes it easy to diagnose what's happening

---

## **How Multi-Channel PTT Should Work:**

### **Concept:**
- **Listening:** Join a channel to HEAR others (click "🎧 Listen" button)
- **Transmitting:** Press hotkey to SPEAK to a channel (works even if not listening)
- **Multi-PTT:** Hold multiple hotkeys simultaneously to transmit to multiple channels

### **Visual Feedback:**
1. **Transmit Button Colors:**
   - **Gray (🎤 OFF):** Not transmitting
   - **Orange (🎤 PTT):** Actively transmitting via hotkey press
   - **Blue (🎤 HOT MIC):** Always transmitting (hot mic mode)

2. **Listen Button Colors:**
   - **Gray (🎧 OFF):** Not joined to channel
   - **Green (🎧 ON):** Joined and listening
   - **Dark Gray (🔇 MUTE):** Joined but muted

---

## **Test Procedures:**

### **Test 1: Basic PTT (Single Channel)**

**Expected Behavior:**
1. **Start client** → Should see:
   ```
   ✅ Voice session started successfully!
   📢 Auto-joined Channel 1 (General) for listening
   🎤 Press F1-F4 to transmit to channels
   ```

2. **Check Channel 1 widget:**
   - Listen button should be **GREEN** (🎧 ON)
   - Transmit button should be **GRAY** (🎤 OFF)
   - Hotkey should show **[F1]**

3. **Press and hold F1:**
   - Console should show:
     ```
     ⌨️ F1 pressed - Starting PTT for channel 1
       ✅ Widget updated - transmit button should turn orange
       📡 Channel 1 joined for listening: YES
       🎤 Active PTT channels: 1
     🎤 PTT started for channel 1
     ```
   - **Transmit button turns ORANGE** (🎤 PTT)
   - **Speak into microphone** → Audio should be encoded and sent

4. **Release F1:**
   - Console should show:
     ```
     ⌨️ F1 released - Stopping PTT for channel 1
       ✅ Widget updated - transmit button should return to gray
       🎤 Remaining PTT channels: (none)
     🔇 PTT stopped for channel 1
     ```
   - **Transmit button returns to GRAY** (🎤 OFF)

---

### **Test 2: Cross-Channel PTT (Not Listening)**

**Expected Behavior:**
1. **Currently listening to Channel 1** (from Test 1)

2. **Press and hold F4** (Dev channel - NOT listening):
   - Console should show:
     ```
     ⌨️ F4 pressed - Starting PTT for channel 4
       ✅ Widget updated - transmit button should turn orange
       📡 Channel 4 joined for listening: NO  ← Important!
       🎤 Active PTT channels: 4
     🎤 PTT started for channel 4
     ```
   - **Channel 4 transmit button turns ORANGE**
   - **Channel 4 listen button stays GRAY** (not listening)
   - **Audio is sent to Channel 4** even though you're not listening to it!

3. **Verify packet routing:**
   - Server logs should show:
     ```
     📦 Voice packet: seq=XXX, ch=4, user=42, payload=XXB
     🔊 Routed voice from user 42 to N recipients in channel 4
     ```
   - **Only Channel 4**, not Channel 1!

---

### **Test 3: Multi-PTT (Simultaneous Channels)**

**Expected Behavior:**
1. **Press and hold F1**:
   - Channel 1 transmit button → **ORANGE**
   - Console: `🎤 Active PTT channels: 1`

2. **While holding F1, also press F2**:
   - Channel 1 transmit button → **ORANGE** (still)
   - Channel 2 transmit button → **ORANGE** (new!)
   - Console: `🎤 Active PTT channels: 1 2`
   - **Audio is transmitted to BOTH channels simultaneously!**

3. **Release F1 (keep holding F2)**:
   - Channel 1 transmit button → **GRAY**
   - Channel 2 transmit button → **ORANGE** (still)
   - Console: `🎤 Remaining PTT channels: 2`
   - **Still transmitting to Channel 2 only**

4. **Release F2**:
   - Channel 2 transmit button → **GRAY**
   - Console: `🎤 Remaining PTT channels: (none)`
   - **No longer transmitting**

---

### **Test 4: Listen to Multiple Channels**

**Expected Behavior:**
1. **Click "🎧 Listen" on Channel 2**:
   - Button turns **GREEN** (🎧 ON)
   - Console: `✅ Joined channel 2 for listening`
   - Now listening to **both Channel 1 and Channel 2**

2. **Press F3 to transmit to Channel 3**:
   - Console shows: `📡 Channel 3 joined for listening: NO`
   - Still works! You can transmit without listening

3. **Verify audio reception:**
   - If another user speaks in Channel 1 → You hear it
   - If another user speaks in Channel 2 → You hear it
   - If another user speaks in Channel 3 → You DON'T hear it (not listening)

---

### **Test 5: Hot Mic Mode**

**Expected Behavior:**
1. **Click "🎤 Transmit" button on Channel 1**:
   - Button turns **BLUE** (🎤 HOT MIC)
   - Console: `🎤 Hot mic set to channel 1`
   - **Always transmitting to Channel 1** (no hotkey needed)

2. **Press F2**:
   - Channel 2 transmit button → **ORANGE** (PTT active)
   - Channel 1 transmit button → **BLUE** (still hot mic)
   - **Transmitting to BOTH Channel 1 (hot mic) and Channel 2 (PTT)**

3. **Release F2**:
   - Channel 2 transmit button → **GRAY**
   - Channel 1 transmit button → **BLUE** (still hot mic)
   - **Still transmitting to Channel 1 only**

4. **Click "🎤 Transmit" button again**:
   - Button returns to **GRAY** (🎤 OFF)
   - Console: `🎤 Hot mic disabled`
   - **No longer transmitting**

---

## **Server-Side Verification:**

### **What to Check in Server Logs:**

1. **User joins only once:**
   ```
   👤 User 42 joined channel 1 from 127.0.0.1:XXXXX
   ```
   - **SAME port** for all channels from one client ✅
   - **NO "User switching from channel X to Y" spam** ✅

2. **Packet routing shows correct channel:**
   ```
   Press F1 → 📦 Voice packet: ch=1
   Press F2 → 📦 Voice packet: ch=2
   Press F1+F2 → 📦 Voice packet: ch=1, then ch=2 (two packets)
   ```

3. **Clean disconnect:**
   ```
   🔌 User dave (ID: 7) disconnected
   🔌 Unregistering user dave
   ```

---

## **Common Issues and Solutions:**

### **Issue 1: Hotkeys Don't Work**
**Symptoms:**
- Press F1-F4, nothing happens
- No console output

**Diagnosis:**
```
Check console for:
❌ Hotkey pressed but no voice session!  ← Voice session not set
⚠️ No widget found for channel X         ← Widget creation failed
```

**Solution:**
- Ensure voice session starts successfully
- Check that `setupMultiChannelUI()` is called

---

### **Issue 2: Audio Not Transmitted**
**Symptoms:**
- PTT button turns orange
- Console shows PTT active
- But server doesn't receive packets

**Diagnosis:**
```
Check audio capture:
🎤 Capturing audio: frame 101  ← Should see this every 2 seconds
```

**If NOT seeing capture output:**
- Microphone not working
- Audio engine not started
- Check `voiceSession->start()` was called

**If seeing capture but no packets:**
```
Check in on_audio_captured():
target_channels.empty()  ← Would cause silent drop
```

---

### **Issue 3: Transmit Button Doesn't Change Color**
**Symptoms:**
- Console shows PTT active
- But button stays gray

**Diagnosis:**
```
⚠️ No widget found for channel X
```

**Solution:**
- Widget not created or not in `channelWidgets_` map
- Check `createDefaultChannels()` execution

---

### **Issue 4: Can't Hear Other Users**
**Symptoms:**
- Can transmit (button turns orange)
- But can't hear anyone else

**Diagnosis:**
1. **Are you listening to the channel?**
   - Listen button should be **GREEN**
   - Console: `📡 Channel X joined for listening: YES`

2. **Are there other users in the channel?**
   - Check user list
   - Server should show multiple users

3. **Is audio decoding working?**
   ```
   Check console:
   📥 Received packet: seq=XXX ch=1 user=YY
   ```

---

## **Success Criteria:**

✅ **Channel 1 auto-joins on startup**  
✅ **F1-F4 hotkeys work immediately**  
✅ **Transmit button turns orange when hotkey pressed**  
✅ **Button returns to gray when hotkey released**  
✅ **Can transmit to channels without listening to them**  
✅ **Multiple hotkeys work simultaneously (multi-PTT)**  
✅ **Server shows single UDP port per client**  
✅ **No "switching channels" spam in server logs**  
✅ **Visual feedback matches actual state**  

---

## **Next Steps After Testing:**

1. **If basic PTT works:** Test with multiple real clients
2. **If multi-PTT works:** Test hot mic + PTT combination
3. **If server routing works:** Test channel muting
4. **If all pass:** System is production-ready! 🎉

