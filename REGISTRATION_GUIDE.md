# User Registration System - Quick Start Guide

## 🎯 **What's New**

Users can now **create their own accounts** on first login! The system:
- ✅ Saves accounts to `server/users.json`
- ✅ Persists across server restarts
- ✅ Validates usernames and passwords
- ✅ Assigns unique user IDs automatically

---

## 🧪 **Testing Registration**

### **Step 1: Start Server**
```powershell
cd C:\dev\VoIP-System\server
cargo run
```

**Expected output:**
```
User file not found, creating demo users
✅ Registered demo user: demo
✅ Registered demo user: alice
...
💾 Saved 6 users to users.json
✅ Loaded 6 users from users.json
```

---

### **Step 2: Register New Account**

```powershell
cd C:\dev\VoIP-System\client
.\build\Debug\voip-client.exe
```

**In the login dialog:**
1. Enter desired username (e.g., `john`)
2. Enter password (min 3 chars, e.g., `john123`)
3. Server: `127.0.0.1:9000`
4. Click **"Register New Account"** button

**Expected results:**
- ✅ Client shows: "Registration successful!"
- ✅ Server logs: "✅ New user registered: john (ID: 7)"
- ✅ File `server/users.json` updated with new user

---

### **Step 3: Login with New Account**

1. Close the client
2. Reopen `.\build\Debug\voip-client.exe`
3. Enter: `john` / `john123`
4. Click **"Login"**

**Expected:**
- ✅ Authentication successful!
- ✅ You can join channels and talk

---

## 📋 **Username Requirements**

- **Length**: 3-20 characters
- **Characters**: Letters, numbers, and underscores only
- **Examples**:
  - ✅ `john`
  - ✅ `player_123`
  - ✅ `Alice2025`
  - ❌ `ab` (too short)
  - ❌ `john@home` (@ not allowed)
  - ❌ `this_is_a_very_long_username_123` (too long)

---

## 🔐 **Password Requirements**

- **Minimum**: 3 characters
- **Recommendation**: Use at least 8 characters in production
- **Note**: Passwords are currently stored in plain text (development only!)

---

## 📊 **What Happens on Registration**

1. **Client → Server**: Sends `register` message
   ```json
   {
     "type": "register",
     "username": "john",
     "password": "john123",
     "email": null
   }
   ```

2. **Server validates**:
   - Username not already taken
   - Username 3-20 chars, alphanumeric + underscore
   - Password min 3 chars

3. **Server responds**:
   ```json
   {
     "type": "register_result",
     "success": true,
     "user_id": 7,
     "message": "Registration successful! You can now login as 'john'"
   }
   ```

4. **Server saves to file**:
   ```json
   [
     {"id": 1, "username": "demo", "password": "demo123", "org_id": 1},
     ...
     {"id": 7, "username": "john", "password": "john123", "org_id": 1}
   ]
   ```

---

## 🧪 **Testing Multiple Users**

### **Test: Three Different Users**

**Terminal 1** - Register & Login as `john`:
```
Username: john
Password: john123
[Register] → Success!
[Login] → User ID: 7
```

**Terminal 2** - Register & Login as `jane`:
```
Username: jane
Password: jane456
[Register] → Success!
[Login] → User ID: 8
```

**Terminal 3** - Login as existing user `demo`:
```
Username: demo
Password: demo123
[Login] → User ID: 1
```

**Result:**
- All 3 users see each other in the channel
- Each has unique ID (7, 8, 1)
- Voice works between all
- User list shows: `john`, `jane`, `demo`

---

## ❌ **Common Errors**

### **"Username already exists"**
```
❌ Registration failed: Username 'john' already exists
```
**Solution**: Choose a different username or login with existing credentials

### **"Username must be 3-20 characters"**
```
❌ Registration failed: Username must be 3-20 characters
```
**Solution**: Choose a username between 3-20 characters

### **"Username can only contain letters, numbers, and underscores"**
```
❌ Registration failed: Username can only contain letters, numbers, and underscores
```
**Solution**: Remove special characters like @, #, !, etc.

### **"Password must be at least 3 characters"**
```
❌ Registration failed: Password must be at least 3 characters
```
**Solution**: Use a longer password

---

## 📁 **User Data File**

**Location**: `server/users.json`

**Format**:
```json
[
  {
    "id": 1,
    "username": "demo",
    "password": "demo123",
    "org_id": 1
  },
  {
    "id": 7,
    "username": "john",
    "password": "john123",
    "org_id": 1
  }
]
```

**Manual Management**:
- View: `cat server/users.json`
- Backup: `copy server/users.json server/users.backup.json`
- Reset: Delete `users.json` and restart server (recreates demo users)

---

## 🔄 **Account Persistence**

### **Server Restart**
```powershell
# Stop server (Ctrl+C)
# Restart
cargo run
```

**Result:**
- ✅ All registered users still exist
- ✅ `users.json` loaded automatically
- ✅ User IDs preserved
- ✅ Next new user gets ID 9, 10, etc.

---

## 🚀 **Next Steps (Future Enhancements)**

### **Already Implemented** ✅
- [x] User registration
- [x] File persistence
- [x] Unique user IDs
- [x] Username validation
- [x] Multiple accounts

### **Coming Soon** 🎯
- [ ] Remember credentials in client (QSettings)
- [ ] Auto-login checkbox
- [ ] Password hashing (bcrypt)
- [ ] Email field for password reset
- [ ] User profile editing
- [ ] Account deletion

---

## 📝 **Developer Notes**

### **Adding Custom Users via JSON**
```powershell
# Stop server
# Edit users.json
notepad server/users.json

# Add new user (make sure ID is unique and higher than existing)
{
  "id": 99,
  "username": "admin",
  "password": "admin_password",
  "org_id": 1
}

# Start server
cargo run
```

### **Resetting to Demo Users**
```powershell
# Delete user file
rm server/users.json

# Restart server - recreates 6 demo users
cargo run
```

---

## ✅ **Success Criteria**

- [x] Register new account via client UI
- [x] Accounts saved to JSON file
- [x] Accounts persist across restarts
- [x] Unique IDs assigned automatically
- [x] Usernames validated (3-20 chars, alphanumeric)
- [x] Passwords validated (min 3 chars)
- [x] Multiple users can register
- [x] Registered users can login
- [x] Server logs registration events

---

**Status**: ✅ **FULLY FUNCTIONAL**  
**Date**: November 23, 2025  
**Quality**: Development/Testing Ready
