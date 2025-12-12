# 🚀 **Start Everything With Logging**

This guide will start the server and both clients with full logging to files for debugging.

## **📋 Prerequisites**

Make sure you have 3 PowerShell terminals open.

---

## **🎬 Step-by-Step Instructions**

### **Terminal 1 - Server**

```powershell
cd c:\dev\VoIP-System\server
.\run_with_logs.ps1
```

**Wait for:**
```
✅ Channel manager initialized
✅ Voice router initialized
✅ UDP voice server started
✅ WebSocket control server started
```

**Logs saving to:** `c:\dev\VoIP-System\server\server_log.txt`

---

### **Terminal 2 - Client 1 (dave)**

```powershell
cd c:\dev\VoIP-System\client
.\run_with_logs.ps1
```

**Login as:**
- Username: `dave`
- Password: `pass123`

**Logs saving to:** `c:\dev\VoIP-System\client\client_log.txt`

---

### **Terminal 3 - Client 2 (bob)**

```powershell
cd c:\dev\VoIP-System\client
.\run_client2.ps1
```

**Login as:**
- Username: `bob`
- Password: `pass123`

**Logs saving to:** `c:\dev\VoIP-System\client\client2_log.txt`

---

## **🧪 Testing the Issue**

### **Reproduce the Intermittent Problem:**

1. **Both clients login and join channel 1** (automatic)
2. **Toggle channel listening** (click channels to join/leave)
3. **Watch user list** - Do users appear/disappear?
4. **Test PTT audio:**
   - Dave presses F1, speaks
   - Does Bob hear? Check Bob's client
   - Bob presses F1, speaks
   - Does Dave hear? Check Dave's client
5. **Repeat several times** to catch intermittent behavior

---

## **📊 Log Files Created**

After testing, you'll have 3 log files:

1. **`server\server_log.txt`** - Server activity (routing, joins, leaves)
2. **`client\client_log.txt`** - Client 1 (dave) activity
3. **`client\client2_log.txt`** - Client 2 (bob) activity

---

## **🔍 Tell Cascade to Analyze**

After reproducing the issue, say:

```
"Analyze all three log files - users are appearing/disappearing when toggling channels"
```

or

```
"Check logs - audio is intermittent, sometimes one-way, sometimes two-way failure"
```

Cascade will read all three files and correlate the events to find the root cause.

---

## **⚡ Quick Stop All**

To stop everything:

```powershell
# Kill all VoIP processes
Stop-Process -Name "voip-server" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "voip-client" -Force -ErrorAction SilentlyContinue
```

---

## **🐛 What to Watch For**

### **In Server Logs:**
- ✅ `User X joined channel Y (now has Z users)` - Should see both users
- ✅ `🔊 Routed voice from user X to Y recipients` - Audio routing working
- ❌ `User X alone in channel Y` - No other users found (BAD!)
- ❌ `User X disconnected` - Unexpected disconnect

### **In Client Logs:**
- ✅ `Login SUCCESS! User ID: X`
- ✅ `Voice session started successfully!`
- ✅ `User X joined channel Y` - Other user appeared
- ❌ `User X left channel Y` - Other user disappeared (investigate why!)
- ❌ `WebSocket connection closed` - Connection dropped

---

## **💡 Common Issues**

### **"Users disappearing from list"**
- Likely: WebSocket disconnect/reconnect
- Look for: "User X disconnected" in server logs
- Timing: When does it happen? During channel toggle?

### **"Audio works sometimes, not others"**
- Likely: UDP address mapping issue
- Look for: "alone in channel" vs "Routed to X recipients"
- Pattern: Does it fail after channel change?

### **"One-way audio"**
- Likely: Only one client's UDP address registered
- Look for: Which direction fails consistently?
- Check: Both users sending voice packets?

---

## **🎯 Next Steps**

1. Start all three processes with logging
2. Reproduce the intermittent issue
3. Tell Cascade to analyze the logs
4. Cascade will identify the exact sequence causing the problem

**Good luck! 🍀**
