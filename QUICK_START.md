# 🚀 Quick Start Guide - Multi-Channel VoIP System

## ✅ **Build Complete!**

Your multi-channel voice system is ready to test!

---

## 📋 **Prerequisites**

- ✅ Server built: `c:\dev\VoIP-System\server\target\debug\voip-server.exe`
- ✅ Client built: `c:\dev\VoIP-System\client\build\Debug\voip-client.exe`
- ✅ Qt DLLs deployed

---

## 🎯 **Step 1: Start the Server**

Open PowerShell and run:

```powershell
cd c:\dev\VoIP-System\server
cargo run
```

**Expected Output:**
```
🚀 Server listening on:
   - WebSocket (TLS): 127.0.0.1:9000
   - Voice (UDP): 127.0.0.1:9001
✅ Server ready!
```

**Leave this terminal open!**

---

## 🎯 **Step 2: Start the Client**

Open **another** PowerShell window and run:

```powershell
cd c:\dev\VoIP-System\client
.\build\Debug\voip-client.exe
```

**The client window should appear!**

---

## 🎯 **Step 3: Register & Login**

1. Click **"Register"** button
2. Enter:
   - Username: `testuser`
   - Password: `password123`
   - Email: `test@example.com`
3. Click **"Register"**
4. Click **"Login"** button
5. Enter same username/password
6. Click **"Login"**

---

## 🎯 **Step 4: Test Multi-Channel!**

You should now see the **Multi-Channel UI**:

```
╔═══════════════════════════════════════════════╗
║  📢 Channels (Multi-Channel)                  ║
╠═══════════════════════════════════════════════╣
║                                               ║
║  # General      🎧 OFF    🎤 OFF              ║
║  [Listen] [Transmit]      [F1]                ║
║                                               ║
║  # Gaming       🎧 OFF    🎤 OFF              ║
║  [Listen] [Transmit]      [F2]                ║
║                                               ║
║  # Music        🎧 OFF    🎤 OFF              ║
║  [Listen] [Transmit]      [F3]                ║
║                                               ║
║  # Dev          🎧 OFF    🎤 OFF              ║
║  [Listen] [Transmit]      [F4]                ║
║                                               ║
╚═══════════════════════════════════════════════╝
```

---

## 🎮 **Test Scenarios**

### **Test 1: Join Multiple Channels**

1. Click **"Listen"** on General → Button turns GREEN
2. Click **"Listen"** on Gaming → Now in BOTH channels!
3. Check Activity Log → Should show:
   ```
   ✅ Joined channel 1 for listening
   ✅ Joined channel 2 for listening
   ```

### **Test 2: Hot Mic**

1. Click **"Transmit"** on General → Button turns BLUE "HOT MIC"
2. Speak into microphone → Always transmitting to General
3. Check Activity Log:
   ```
   🎤 Hot mic set to channel 1
   ```

### **Test 3: PTT Hotkeys**

1. Make sure you're joined to General (Listen = ON)
2. Press **F1** key → Activity log shows:
   ```
   🎤 PTT started for channel 1
   ```
3. Release **F1** → Activity log shows:
   ```
   🔇 PTT stopped for channel 1
   ```

### **Test 4: Multi-PTT**

1. Join General, Gaming, Music (all Listen ON)
2. Press **F1** → Transmit to General
3. Press **F2** → Transmit to Gaming
4. Press **F1 + F2** together → Transmit to BOTH!
5. Press **F1 + F2 + F3** → Transmit to ALL THREE!

### **Test 5: Hot Mic + PTT Combo**

1. Set Hot Mic on General (Transmit button = BLUE)
2. Join Gaming (Listen = ON)
3. Speak → Transmits to General only (hot mic)
4. Hold **F2** → Transmits to General AND Gaming!
5. Release **F2** → Back to just General

### **Test 6: Per-Channel Muting**

1. Join General, Gaming, Music (all Listen = ON)
2. Click **"Listen"** on Music again → Button shows "🔇 MUTE"
3. Music is now muted but still joined
4. Click **"Listen"** again → Unmutes

---

## 🔧 **Troubleshooting**

### **Problem: Client closes immediately**

**Solution:** Make sure the server is running first!

```powershell
# Terminal 1: Start server
cd c:\dev\VoIP-System\server
cargo run

# Terminal 2: Start client (after server is running)
cd c:\dev\VoIP-System\client
.\build\Debug\voip-client.exe
```

### **Problem: "Connection failed"**

**Check:**
1. Server is running on port 9000
2. Firewall allows connections
3. Check server terminal for errors

### **Problem: No audio**

**Check:**
1. Microphone is plugged in
2. Microphone is not muted in Windows
3. Click Settings → Check audio devices
4. Join a channel before testing

### **Problem: Hotkeys don't work**

**Check:**
1. Client window has focus (click on it)
2. Press F1-F4 (not other keys)
3. Join a channel first (Listen = ON)
4. Check Activity Log for hotkey messages

---

## 📊 **Features to Test**

- ✅ Multi-channel listening (join 3+ channels)
- ✅ Audio mixing (hear all channels simultaneously)
- ✅ Per-channel muting (instant mute/unmute)
- ✅ Hot mic (always transmit to one channel)
- ✅ PTT hotkeys (F1-F4)
- ✅ Multi-PTT (F1+F2+F3 simultaneously)
- ✅ Hot mic + PTT combo
- ✅ Visual feedback (colors change on buttons)
- ✅ Activity log (shows all actions)

---

## 🎉 **Success Indicators**

✅ **Server Console:**
```
User 1 registered successfully
User authenticated: testuser (ID: 1)
User 1 joined channel 1
Voice packet received from user 1 on channel 1
```

✅ **Client Activity Log:**
```
[19:10:00] Welcome to VoIP Client!
[19:10:01] 🎮 Multi-channel support enabled!
[19:10:02] ⌨️ Hotkey system initialized
[19:10:03] 📢 Created 4 channels with hotkeys
[19:10:05] ✅ WebSocket Connected to server
[19:10:06] ✅ Joined channel 1 for listening
[19:10:07] 🎤 Hot mic set to channel 1
[19:10:08] 🎤 PTT started for channel 2
```

---

## 📝 **Notes**

- Default server: `127.0.0.1:9000` (WebSocket) and `127.0.0.1:9001` (Voice UDP)
- Default channels: General (ID 1), Gaming (ID 2), Music (ID 3), Dev (ID 4)
- Default hotkeys: F1=General, F2=Gaming, F3=Music, F4=Dev
- Audio codec: Opus 48kHz, 20ms frames, 32kbps
- Latency: ~150-200ms total (acceptable for voice)

---

## 🚀 **Ready to Test!**

1. Start server
2. Start client  
3. Register & login
4. Test multi-channel features!

**Enjoy your new multi-channel voice system!** 🎮🎤
