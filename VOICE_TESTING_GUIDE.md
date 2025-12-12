# 🎤 Voice Testing Guide - Two Clients

## 🚀 Quick Start (5 minutes)

### Step 1: Launch Second Client
```powershell
# From the client directory:
cd C:\dev\VoIP-System\client
Start-Process ".\build\Debug\voip-client.exe" -WorkingDirectory ".\build\Debug"
```

**Or simply:** Double-click `voip-client.exe` in File Explorer to open a second window!

---

### Step 2: Login Both Clients

**Client 1 (First Window):**
```
Username: demo
Password: demo123
```

**Client 2 (Second Window):**

**Option A - Quick Test (Same User):**
```
Username: demo
Password: demo123
```
⚠️ **Note:** This will disconnect Client 1 (single session per user), but it's fine for quick testing!

**Option B - Proper Multi-User Test:**
1. In **Client 1**, go to **Admin tab** (🔧)
2. Click **Users** sub-tab
3. Click **➕ Create User**
4. Fill in:
   ```
   Username: testuser
   Password: test123
   Email: (optional)
   Organization: Demo Organization
   ```
5. Click **Create**
6. In **Client 2**, login as:
   ```
   Username: testuser
   Password: test123
   ```

---

## 🎙️ Voice Testing Steps

### 1. Join Same Channel

**In Both Clients:**
1. After login, you'll see the channel list on the left
2. Double-click **"General"** channel (or any channel)
3. Both clients should now be in the same channel

**Expected:**
- ✅ Channel name appears at top
- ✅ User list shows both users
- ✅ Voice indicator appears

---

### 2. Test Push-to-Talk (PTT)

**Client 1:**
1. Press and hold **Space bar** (default PTT key)
2. Speak into your microphone
3. You should see the input level meter move (green bar)
4. Release Space when done

**Client 2:**
1. Should hear Client 1's voice through speakers
2. Should see output level meter moving (green bar)
3. Should see Client 1's user icon highlighted when speaking

**Then switch:**
- Client 2 presses Space and speaks
- Client 1 should hear it

---

### 3. Test Features

**Mute Test:**
- Click the 🔇 **Mute** button
- Try PTT - should not transmit
- Other client shouldn't hear you

**Deafen Test:**
- Click the 🔊 **Deafen** button
- Other client speaks - you shouldn't hear anything
- Your output meter shouldn't move

**Volume Control:**
- Adjust the output slider
- Verify volume changes while other person speaks

---

## 🧪 Expected Behavior

### Audio Path
```
Client 1 Mic → Opus Encode → UDP Packet → Server 
    → Server Route to Channel → UDP Packet → Client 2
    → Jitter Buffer → Opus Decode → Speakers
```

### Performance Metrics
- **Latency:** 150-200ms (shown in stats)
- **Packet Loss:** <1% on good network
- **Audio Quality:** Should be clear at 32kbps
- **CPU Usage:** <5% per client

### Visual Indicators

**When Transmitting (PTT pressed):**
- ✅ Input meter shows green (40-80% for normal speech)
- ✅ Your user icon should highlight
- ✅ Packet count increases in stats

**When Receiving:**
- ✅ Output meter shows green
- ✅ Other user's icon highlights
- ✅ Jitter buffer shows frames queued

---

## 🐛 Troubleshooting

### Issue: Can't hear other client

**Check:**
1. Both clients in same channel? (Look at channel name at top)
2. Deafen disabled? (🔊 button should not be pressed)
3. Output volume up? (Slider on right side)
4. Windows volume up? (System tray)
5. Server running? (Check server console for voice packets)

**Debug:**
```powershell
# Check server is receiving voice packets
# In server console, you should see:
📨 Voice packet from user X in channel Y
```

---

### Issue: Other client can't hear me

**Check:**
1. Microphone working? (Check Windows sound settings)
2. Mute disabled? (🔇 button should not be pressed)
3. PTT key pressed? (Hold Space while talking)
4. Input meter moving when you speak? (Green bar should move)

**Test microphone:**
- Open Windows Sound Settings
- Speak - the input level should move
- If not, select correct microphone device

---

### Issue: Audio is choppy/robotic

**Possible causes:**
- Network packet loss >5%
- High CPU usage (check Task Manager)
- Jitter buffer underruns (check stats)

**Fix:**
- Close other applications
- Check network connection
- Increase jitter buffer size (requires code change)

---

### Issue: High latency (>500ms)

**Check:**
- Network RTT (ping server)
- Server location (should be low-latency)
- Both clients on same local network? (should be <50ms)

---

## 📊 Voice Statistics

**To view stats:**
1. In client, look at bottom status bar
2. Should show:
   ```
   🎤 Latency: 180ms | Quality: 95% | PLC: 2 | Jitter: 5 frames
   ```

**What they mean:**
- **Latency:** Round-trip audio delay
- **Quality:** Percentage of packets received
- **PLC:** Packet Loss Concealment events (should be low)
- **Jitter:** Frames in buffer (5 = ~100ms buffer)

---

## 🎯 Test Scenarios

### Scenario 1: Basic Voice Chat
1. Join channel
2. Press PTT and greet
3. Other client responds
4. **Expected:** Clear bidirectional audio

### Scenario 2: Multiple Users
1. Join 3+ clients to same channel
2. Each speaks in turn
3. **Expected:** All users can hear all others

### Scenario 3: Channel Switching
1. Start in channel 1
2. Both switch to channel 2
3. Test voice again
4. **Expected:** Voice still works after channel change

### Scenario 4: Reconnection
1. Disconnect one client (close window)
2. Reopen and rejoin channel
3. Test voice
4. **Expected:** Voice works after reconnect

### Scenario 5: Stress Test
1. Press PTT and speak continuously for 30 seconds
2. Check for audio artifacts
3. Check CPU and network usage
4. **Expected:** Smooth audio, <5% CPU

---

## 🎊 Success Criteria

**Voice System Working if:**
- ✅ Both clients can hear each other clearly
- ✅ Latency <250ms consistently
- ✅ No audio dropouts or glitches
- ✅ PTT responds instantly
- ✅ Mute/deafen work correctly
- ✅ Audio level meters work
- ✅ Can switch channels without issues

---

## 🚀 Advanced Testing

### Test Packet Loss Recovery
```powershell
# On Windows, simulate packet loss:
# Settings → Network → Advanced → Properties
# Or use clumsy.exe to inject packet loss
```

**Expected:** FEC + PLC should handle 2-5% packet loss smoothly

### Test Jitter Handling
- Network should handle variable latency
- Jitter buffer adapts automatically
- Audio should stay smooth with ±50ms jitter

### Test Bandwidth
**At 32kbps per stream:**
- 1 user speaking = ~4 KB/s upload
- 3 users in channel = ~12 KB/s download
- Total: ~16 KB/s for 3-user channel

---

## 📝 Quick Commands

**Launch Client 1:**
```powershell
.\build\Debug\voip-client.exe
```

**Launch Client 2 (new window):**
```powershell
Start-Process ".\build\Debug\voip-client.exe"
```

**Check Running Clients:**
```powershell
Get-Process voip-client | Format-Table Id, MainWindowTitle
```

**Stop All Clients:**
```powershell
Stop-Process -Name voip-client -Force
```

---

## 🎉 You're Ready!

Now you have:
- ✅ Two client instances running
- ✅ User accounts for both
- ✅ Testing guide for voice
- ✅ Troubleshooting steps
- ✅ Success criteria

**Go ahead and test! Press Space to talk, and enjoy your fully-functional VoIP system! 🎤**
