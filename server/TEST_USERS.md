# VoIP Server - Test User Accounts

## 🎯 **Multi-User Authentication System**

The server now supports multiple user accounts with **unique user IDs** for each user!

---

## 👥 **Available Test Accounts**

| Username | Password | User ID | Description |
|----------|----------|---------|-------------|
| `demo` | `demo123` | 1 | Original demo account |
| `alice` | `alice123` | 2 | Test user Alice |
| `bob` | `bob123` | 3 | Test user Bob |
| `charlie` | `charlie123` | 4 | Test user Charlie |
| `test1` | `123` | 5 | Quick test account 1 |
| `test2` | `123` | 6 | Quick test account 2 |

---

## 🧪 **Testing Scenarios**

### **Scenario 1: Different Users, Different IDs**

**Client 1:**
- Username: `alice`
- Password: `alice123`
- Expected User ID: `2`

**Client 2:**
- Username: `bob`
- Password: `bob123`
- Expected User ID: `3`

**Result:**
- ✅ Both clients see 2 users: `alice` and `bob`
- ✅ Each has unique user ID
- ✅ Voice works between them
- ✅ When one disconnects, the other is notified

---

### **Scenario 2: Same Username (Not Allowed in Production)**

**Client 1:**
- Username: `demo`
- Password: `demo123`
- User ID: `1`

**Client 2:**
- Username: `demo`
- Password: `demo123`
- User ID: `1` (same!)

**Result:**
- ⚠️ Both get the same user ID (limitation of current system)
- ⚠️ User list shows duplicates
- ✅ Voice still works
- ⚠️ Disconnect notifications may be confusing

**Note**: In production, this would be prevented by session management.

---

### **Scenario 3: Three Different Users**

**Client 1:** `alice` / `alice123` → ID: 2  
**Client 2:** `bob` / `bob123` → ID: 3  
**Client 3:** `charlie` / `charlie123` → ID: 4  

**Expected:**
- ✅ Client 1 sees: `alice` (self)
- ✅ When Client 2 joins → Client 1 sees: `alice`, `bob`
- ✅ When Client 3 joins → All see: `alice`, `bob`, `charlie`
- ✅ Each user has unique ID
- ✅ All can hear each other

---

## 🔐 **Authentication Flow**

1. **Client connects** → Server sends `challenge`
2. **Client sends** `authenticate` with username/password
3. **Server looks up** user in registry
4. **If found** → Returns `auth_result` with unique user ID
5. **If not found** → Returns error "Invalid credentials"
6. **Client joins channel** → Server broadcasts to others

---

## 📊 **Server Console Output**

**On startup:**
```
✅ Registered demo user: demo
✅ Registered demo user: alice
✅ Registered demo user: bob
✅ Registered demo user: charlie
✅ Registered demo user: test1
✅ Registered demo user: test2
✅ User registry initialized
✅ Channel manager initialized
```

**When alice logs in:**
```
INFO New WebSocket connection
✅ User alice (ID: 2) authenticated and registered
INFO User alice (ID: 2) joining channel 1
👤 User alice (ID: 2) joining channel 1
✅ User alice joined channel 1 (now has 1 users)
```

**When bob joins:**
```
INFO New WebSocket connection
✅ User bob (ID: 3) authenticated and registered
INFO User bob (ID: 3) joining channel 1
👤 User bob (ID: 3) joining channel 1
✅ User bob joined channel 1 (now has 2 users)
📢 Broadcasting to channel 1 (2 users)
```

**When alice disconnects:**
```
INFO Client closed connection
🔌 User alice (ID: 2) disconnected
👋 User ID 2 left channel 1
📢 Broadcasting to channel 1 (1 users)
```

---

## ✅ **What Works**

- ✅ Multiple users with unique accounts
- ✅ Each user gets unique ID (auto-incrementing)
- ✅ Password validation
- ✅ Failed login attempts logged
- ✅ User-specific WebSocket registration
- ✅ Channel broadcasts identify users correctly
- ✅ Disconnect notifications work properly

---

## ⚠️ **Current Limitations**

1. **No Session Management**: Same user can log in multiple times
   - Each connection gets the same user ID
   - Not a security issue for testing, but needs fixing for production

2. **In-Memory Only**: All users reset when server restarts
   - Solution: Add database persistence

3. **Plain Text Passwords**: Stored unhashed
   - Solution: Use bcrypt or argon2 for production

4. **No User Registration**: Users are pre-defined
   - Solution: Add registration endpoint

---

## 🚀 **Next Steps**

### **Immediate (For Testing)**
1. Test with 2-3 different users
2. Verify unique IDs in user list
3. Test join/leave notifications
4. Test voice between different users

### **Short-Term (Production Ready)**
1. Session management (prevent duplicate logins)
2. Database integration (PostgreSQL/SQLite)
3. Password hashing (bcrypt)
4. User registration API

### **Long-Term**
1. Email verification
2. Password reset
3. User profiles
4. Admin panel

---

## 🧪 **Quick Test Commands**

**Terminal 1 - Start Server:**
```powershell
cd C:\dev\VoIP-System\server
cargo run
```

**Terminal 2 - Client 1 (Alice):**
```powershell
cd C:\dev\VoIP-System\client
.\build\Debug\voip-client.exe
# Login: alice / alice123
```

**Terminal 3 - Client 2 (Bob):**
```powershell
cd C:\dev\VoIP-System\client
.\build\Debug\voip-client.exe
# Login: bob / bob123
```

**Expected Result:**
- Alice sees: `alice` (ID: 2)
- When Bob joins, Alice sees: `alice`, `bob`
- Bob sees: `alice`, `bob` (IDs: 2, 3)
- Voice works between them
- Unique IDs displayed in console

---

**Status**: ✅ **MULTI-USER SYSTEM READY FOR TESTING**  
**Date**: November 23, 2025  
**Quality**: Development/Testing Ready (Not production without sessions)
