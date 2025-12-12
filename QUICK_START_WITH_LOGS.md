# 🚀 **Quick Start with Log File Access**

## **All-in-One Test Setup:**

### **Terminal 1: Server**
```powershell
cd c:\dev\VoIP-System\server
.\run_with_logs.ps1
```
📝 **Logs → `server_log.txt`**

---

### **Terminal 2: Client 1**
```powershell
cd c:\dev\VoIP-System\client
.\run_with_logs.ps1
```
📝 **Logs → `client_log.txt`**

**Login as:** `dave` / `pass123`

---

### **Terminal 3: Client 2**
```powershell
cd c:\dev\VoIP-System\client
.\run_client2.ps1
```
📝 **Logs → `client2_log.txt`**

**Login as:** `bob` / `pass123`

---

## **Test PTT:**

1. **Dave (Client 1):** Press `F1` and speak
2. **Bob (Client 2):** Should hear audio! 🔊

---

## **If Something Goes Wrong:**

Just tell me:

> "Check server_log.txt"

or

> "Read client_log.txt and client2_log.txt"

I'll read the files directly - no more copy-pasting! 🎉

---

## **Log File Paths:**

- **Server:** `c:\dev\VoIP-System\server\server_log.txt`
- **Client 1:** `c:\dev\VoIP-System\client\client_log.txt`
- **Client 2:** `c:\dev\VoIP-System\client\client2_log.txt`

---

## **Expected Timeline:**

### **Client Logs:**
```
1. ✅ Voice session initialized (not started yet)
2. ✅ Login SUCCESS! User ID: X
3. 🔧 Updating VoiceSession user ID to X
4. ✅ Join channel 1 request sent
5. 🎤 Starting voice session now that channel is joined...
6. ✅ Voice session started successfully!
7. ⌨️ F1 pressed - Starting PTT for channel 1
8. 📡 Transmit targets: Channels: 1
```

### **Server Logs:**
```
1. New WebSocket connection
2. ✅ User dave (ID: 7) authenticated
3. ✅ User dave joined channel 1 (now has X users)
4. 📦 Voice packet: user=7, ch=1
5. 🔊 Routed voice from user 7 to Y recipients
```

---

**🎯 The key: Server must show "Routed to Y recipients" (not "alone")!**
