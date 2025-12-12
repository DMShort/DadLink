# 🚀 Quick VoIP System Test Guide

## ✅ Current Status:
- **UI:** ✅ Perfect! All buttons work
- **Struct Packing:** ✅ Already fixed (#pragma pack)
- **Server:** ✅ Running on ports 9000 (WebSocket) + 9001 (UDP)
- **Client:** ✅ Starting now

---

## 🎯 Quick Test Steps:

### 1. Login to Client
**In the login dialog that just opened:**
```
Username: TestUser
Password: (anything)
Address: 127.0.0.1
Port: 9000
```
**Click "Connect"**

---

### 2. Main Window Opens
**You should see:**
- Username: "User: TestUser"
- Channel list (left)
- User list (middle)
- Activity log (right): "Welcome to VoIP Client!"
- Voice controls (bottom):
  - 🔇 Mute button
  - 🔇 Deafen button
  - ⌨️ Push-to-Talk: OFF
  - Input/Output meters

---

### 3. Test Your Voice! 🎤

**Click the Mute button to UNMUTE** (it starts muted)
- Button changes to show you're transmitting
- Speak into your microphone
- **You should hear yourself with ~20-50ms delay!**

**Watch the meters:**
- **Input meter** (left) - bounces when you speak
- **Output meter** (right) - bounces when you receive

**Check statistics:**
```
Latency: 20-50ms
Quality: Good ✅
Cap: XX, Enc: XX, Sent: XX
Recv: YY, Dec: YY, Play: YY
PLC: Should be LOW (< 10)
Underruns: Should be LOW (< 5)
```

---

### 4. Test Push-to-Talk (PTT) 🎮

**Click "⌨️ Push-to-Talk: OFF"** button
- Changes to "⌨️ Push-to-Talk: ON"
- Indicator shows: "🎤 Voice: Press Space to Talk"
- You start muted

**Press and HOLD the SPACE key:**
- Indicator turns red: "🎤 Voice: TRANSMITTING"
- Microphone activates
- Speak while holding space
- **You should hear yourself!**

**Release SPACE:**
- Back to muted
- Indicator: "🎤 Voice: Press Space to Talk"

---

### 5. Multi-User Test (Optional) 👥

**Open a second terminal:**
```powershell
Start-Process ".\build\Debug\voip-client.exe"
```

**In second client:**
- Login as: User2
- Connect to same server

**Test:**
- User1 speaks → User2 hears
- User2 speaks → User1 hears
- Both can talk simultaneously!

**User List:**
- Both clients show both users
- Activity log: "✅ User joined channel: User2"

---

## 📊 What Good Performance Looks Like:

### Excellent ✅
- **PLC:** < 10 frames (< 1%)
- **Underruns:** 0-2
- **Latency:** 20-30ms
- **Quality:** Good (green)
- **Audio:** Clear, no robotic sound

### Acceptable ⚠️
- **PLC:** 10-50 frames (1-5%)
- **Underruns:** 2-10
- **Latency:** 30-50ms
- **Quality:** Fair (yellow)
- **Audio:** Slight choppiness OK

### Needs Fixing ❌
- **PLC:** > 100 frames (> 10%)
- **Underruns:** > 20
- **Latency:** > 100ms
- **Quality:** Poor (red)
- **Audio:** Robotic, choppy

---

## 🐛 Quick Troubleshooting:

### Can't hear anything?
1. Check microphone working (Windows Sound Settings)
2. Check speakers working (play music)
3. Click Mute button (might be muted)
4. Check input meter bouncing when speaking
5. Server running? (should see server console output)

### High PLC or underruns?
- This is normal on first test
- Jitter buffer is tuning
- Give it 10-20 seconds to stabilize
- If stays high: Check server console for errors

### Client crashes on Connect?
- Check console output for error
- Verify DLLs: `.\verify_dlls.ps1`
- Audio devices available?

---

## 🎉 Success Indicators:

**You have a WORKING VoIP system when:**

✅ Can hear your own voice  
✅ Latency < 50ms  
✅ PLC < 10 frames  
✅ Audio is clear  
✅ PTT works  
✅ Two users can communicate  

---

## 🚀 Next Level Features:

**Once basic voice works, you can add:**
- Channel switching
- Text chat
- File sharing
- Screen sharing
- Video calls
- Mobile clients
- Web interface

---

## 📝 Commands Reference:

```powershell
# Start server
cd C:\dev\VoIP-System\server
cargo run

# Start client
cd C:\dev\VoIP-System\client
.\build\Debug\voip-client.exe

# Rebuild client
.\build_and_deploy.bat

# Verify DLLs
.\verify_dlls.ps1

# Stop server
# Press Ctrl+C in server terminal

# Stop client
Stop-Process -Name "voip-client" -Force
```

---

**Ready to test! The client is open - enter your credentials and click Connect!** 🎯
