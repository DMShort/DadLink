# 📝 **Log File Access for Cascade**

## **Log Files Created:**

### **Server Log:**
- **Path:** `c:\dev\VoIP-System\server\server_log.txt`
- **How to start:** `cd server; .\run_with_logs.ps1`
- **What it contains:** All server activity (WebSocket, UDP, routing, errors)

### **Client Log (Primary):**
- **Path:** `c:\dev\VoIP-System\client\client_log.txt`
- **How to start:** `cd client; .\run_with_logs.ps1`
- **What it contains:** Client 1 activity (login, PTT, audio, errors)

### **Client 2 Log (Multi-User Testing):**
- **Path:** `c:\dev\VoIP-System\client\client2_log.txt`
- **How to start:** `cd client; .\run_client2.ps1`
- **What it contains:** Client 2 activity (second user for testing)

---

## **Usage Instructions:**

### **1. Start Server with Logging**
```powershell
cd c:\dev\VoIP-System\server
.\run_with_logs.ps1
```

**Logs saved to:** `server\server_log.txt`

### **2. Start Client 1 with Logging**
```powershell
cd c:\dev\VoIP-System\client
.\run_with_logs.ps1
```

**Logs saved to:** `client\client_log.txt`

### **3. Start Client 2 with Logging (Multi-User Test)**
```powershell
# In a NEW terminal:
cd c:\dev\VoIP-System\client
.\run_client2.ps1
```

**Logs saved to:** `client\client2_log.txt`

---

## **For Cascade: How to Read Logs**

### **Read Server Log:**
```
Read file: c:\dev\VoIP-System\server\server_log.txt
```

### **Read Client 1 Log:**
```
Read file: c:\dev\VoIP-System\client\client_log.txt
```

### **Read Client 2 Log:**
```
Read file: c:\dev\VoIP-System\client\client2_log.txt
```

---

## **What Each Log Shows:**

### **Server Log (`server_log.txt`):**
```
✅ Shows when clients connect via WebSocket
✅ Shows login attempts (success/failure)
✅ Shows channel joins/leaves
✅ Shows UDP voice packets received
✅ Shows routing decisions ("Routed to X recipients")
✅ Shows errors (UDP receive errors, auth failures)
```

**Key things to look for:**
- `New WebSocket connection` - Client connected
- `User X authenticated` - Login succeeded
- `User X joined channel Y` - Channel join succeeded
- `📦 Voice packet: user=X, ch=Y` - UDP packet received
- `🔊 Routed voice from user X to Y recipients` - Routing success
- `👤 User X alone in channel Y` - No routing (problem!)

### **Client Log (`client_log.txt`):**
```
✅ Shows voice session initialization
✅ Shows WebSocket connection attempts
✅ Shows login attempts
✅ Shows channel join results
✅ Shows PTT activation (F1-F4 pressed)
✅ Shows audio transmission
✅ Shows errors
```

**Key things to look for:**
- `✅ Voice session initialized (not started yet)` - Good start
- `✅ Login SUCCESS! User ID: X` - Auth worked
- `🔧 Updating VoiceSession user ID to X` - User ID fix applied
- `🎤 Starting voice session now that channel is joined...` - Timing fix working
- `✅ Voice session started successfully!` - Ready to transmit
- `⌨️ F1 pressed - Starting PTT for channel 1` - PTT activated
- `📡 Transmit targets: Channels: X` - Sending to channel

---

## **Multi-User Test Scenario:**

### **Terminal 1: Server**
```powershell
cd c:\dev\VoIP-System\server
.\run_with_logs.ps1
```

### **Terminal 2: Client 1 (Dave)**
```powershell
cd c:\dev\VoIP-System\client
.\run_with_logs.ps1
# Login as: dave / pass123
```

### **Terminal 3: Client 2 (Bob)**
```powershell
cd c:\dev\VoIP-System\client
.\run_client2.ps1
# Login as: bob / pass123
```

### **Then:**
1. Press F1 on Client 1 (Dave) and speak
2. Check `server_log.txt` - Should see "Routed to 1 recipients"
3. Check `client2_log.txt` - Should see audio received
4. Check Client 2 speakers - Should HEAR audio!

---

## **Benefits:**

✅ **No more copy-pasting** - Cascade can read files directly  
✅ **Full history** - All logs captured, not just what fits in terminal  
✅ **Multi-client testing** - Separate logs for each client  
✅ **Timestamps preserved** - Can correlate events across logs  
✅ **Easier debugging** - Can search/analyze full log files  

---

## **Log File Locations (Quick Reference):**

```
c:\dev\VoIP-System\server\server_log.txt    ← Server logs
c:\dev\VoIP-System\client\client_log.txt    ← Client 1 logs
c:\dev\VoIP-System\client\client2_log.txt   ← Client 2 logs
```

**Cascade can read any of these files directly during debugging!**

---

## **Commands to Share with Cascade:**

When reporting issues, just say:

> "Check server_log.txt for errors"

or

> "See what client_log.txt shows after login"

or

> "Compare client_log.txt and client2_log.txt for the PTT test"

Cascade will read the files directly! 🎉
