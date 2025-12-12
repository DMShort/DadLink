# 🧪 **READY TO TEST - Complete Instructions**

## **✅ Client Rebuilt with All Fixes!**

The following fixes are now active:
1. ✅ Server auto-join disabled (PTT fix)
2. ✅ Client user ID uses authenticated ID (not hardcoded 42)
3. ✅ Voice session starts AFTER WebSocket channel join (timing fix)

---

## **🚀 Test Procedure:**

### **Terminal 1: Server (Already Running)**
Your server is already running from:
```powershell
cd c:\dev\VoIP-System\server
.\run_with_logs.ps1
```

**Server log:** `c:\dev\VoIP-System\server\server_log.txt`

✅ Server is ready and listening!

---

### **Terminal 2: Client 1 (Dave)**

```powershell
cd c:\dev\VoIP-System\client
.\run_with_logs.ps1
```

**What to watch for:**
```
✅ Voice session initialized (not started yet)  ← NEW! Voice waits
WebSocket connect() initiated (async)
✅ Login SUCCESS! User ID: 7
🔧 Updating VoiceSession user ID to 7           ← NEW! Uses real ID
✅ Join channel 1 request sent
🎤 Starting voice session now that channel is joined...  ← NEW! Delayed start
✅ Voice session started successfully!
✅ Voice session ready - you can now use PTT!
```

**Login as:** `dave` / `pass123`

**Client log:** `c:\dev\VoIP-System\client\client_log.txt`

---

### **Terminal 3: Client 2 (Bob) - OPTIONAL FOR MULTI-USER TEST**

```powershell
cd c:\dev\VoIP-System\client
.\run_client2.ps1
```

**Same messages as Client 1, but with User ID: 3**

**Login as:** `bob` / `pass123`

**Client log:** `c:\dev\VoIP-System\client\client2_log.txt`

---

## **🎮 Single Client Test (Dave Only):**

1. **Start Client 1** (see Terminal 2 above)
2. **Login as dave**
3. **Watch for all 4 success messages** (especially "Starting voice session now that channel is joined")
4. **Press F1** and speak
5. **Check server logs** for:
   ```
   📦 Voice packet: user=7, ch=1  ← Should be user=7 (not 42!)
   ```

---

## **👥 Multi-Client Test (Dave + Bob):**

1. **Start Client 1** (dave) - see Terminal 2
2. **Start Client 2** (bob) - see Terminal 3
3. **Both should see 2 users in the user list**
4. **Dave: Press F1** and speak
5. **Bob: Should HEAR audio** from speakers! 🔊
6. **Bob: Press F1** and speak
7. **Dave: Should HEAR audio** from speakers! 🔊

---

## **📊 How to Check Logs (Tell Cascade):**

Instead of copy-pasting, just say:

> **"Check server_log.txt"**

or

> **"Read client_log.txt after login"**

or

> **"Compare client_log.txt and client2_log.txt"**

Cascade will read the files directly!

---

## **✅ Success Indicators:**

### **Client Logs Should Show:**
```
✅ Voice session initialized (not started yet)  ← Voice waits for WebSocket
✅ Login SUCCESS! User ID: 7                    ← Real user ID
🔧 Updating VoiceSession user ID to 7           ← User ID fix applied
✅ Join channel 1 request sent                  ← WebSocket join
🎤 Starting voice session now that channel is joined... ← Timing fix!
✅ Voice session started successfully!          ← NOW voice starts
⌨️ F1 pressed - Starting PTT for channel 1    ← PTT works
📡 Transmit targets: Channels: 1               ← Sending to Ch1
```

### **Server Logs Should Show:**
```
New WebSocket connection                        ← Client connected
✅ User dave (ID: 7) authenticated              ← Login worked
✅ User dave joined channel 1 (now has X users) ← Channel join
📦 Voice packet: user=7, ch=1                   ← Correct user ID!
🔊 Routed voice from user 7 to Y recipients     ← Routing works!
```

**NOT:**
```
❌ 📦 Voice packet: user=42  ← Old hardcoded ID
❌ 👤 User 7 alone in channel 1  ← Routing failed
```

---

## **❌ Failure Indicators:**

### **If you see:**
```
❌ Hot mic: 1 | PTT: none (continuously spamming)
❌ Capturing audio: frame 33701 (before login)
❌ No "Login SUCCESS" message
❌ No "Starting voice session now that channel is joined"
```

**→ This means the OLD client is running! Rebuild again.**

---

## **🔧 If Something Goes Wrong:**

### **Client won't start:**
```powershell
cd c:\dev\VoIP-System\client
Stop-Process -Name "voip-client" -Force -ErrorAction SilentlyContinue
cmake --build build --config Debug
.\run_with_logs.ps1
```

### **Server not responding:**
```powershell
# Restart server:
cd c:\dev\VoIP-System\server
# Ctrl+C in server terminal
.\run_with_logs.ps1
```

### **No WebSocket connection:**
- Check server is running on port 9000
- Check client login dialog shows correct server address (127.0.0.1:9000)
- Check firewall isn't blocking

### **No audio between clients:**
- First verify **server logs show "Routed to X recipients"** (not "alone")
- Then verify **both clients show "Voice session started successfully"**
- Then check **audio devices** (microphone and speakers working?)

---

## **📁 Log File Locations:**

```
c:\dev\VoIP-System\server\server_log.txt    ← Server
c:\dev\VoIP-System\client\client_log.txt    ← Client 1 (dave)
c:\dev\VoIP-System\client\client2_log.txt   ← Client 2 (bob)
```

---

## **🎯 Expected Timeline:**

### **Before PTT (should take ~2 seconds total):**
1. Client starts (0.1s)
2. Voice session initialized, NOT started (0.1s)
3. WebSocket connects (0.5s)
4. Login succeeds (0.2s)
5. Join channel (0.2s)
6. **THEN voice session starts** (0.5s)
7. Ready for PTT! ✅

### **After F1 Pressed:**
1. PTT activates (instant)
2. Audio captured (continuous)
3. Packets sent via UDP (continuous)
4. Server routes packets (instant per packet)
5. Other clients receive and play audio (<100ms latency)

---

## **🚀 Quick Commands Summary:**

```powershell
# Terminal 1: Server (already running)
cd c:\dev\VoIP-System\server
.\run_with_logs.ps1

# Terminal 2: Client 1
cd c:\dev\VoIP-System\client
.\run_with_logs.ps1
# Login: dave / pass123

# Terminal 3: Client 2 (optional)
cd c:\dev\VoIP-System\client
.\run_client2.ps1
# Login: bob / pass123
```

---

## **💬 What to Tell Cascade:**

### **If it works:**
> "It works! Both clients can hear each other!"

### **If login fails:**
> "Check client_log.txt - login failed"

### **If no audio:**
> "No audio between clients. Check server_log.txt for routing"

### **If WebSocket won't connect:**
> "No WebSocket connection. Check server_log.txt"

---

**🎉 You're ready to test! Server is already running, just start the client(s)!**
