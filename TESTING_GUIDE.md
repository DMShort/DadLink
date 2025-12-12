# VoIP System - Comprehensive Testing Guide

**Created:** November 27, 2025
**For:** Phases 2, 3, and 5 Testing
**Status:** Ready for testing

---

## 🎯 Overview

This guide will help you test all the features we've implemented:
- ✅ **Phase 2:** Database Integration (PostgreSQL)
- ✅ **Phase 3:** TLS/HTTPS (Secure WebSocket)
- ✅ **Phase 5:** Permissions & ACL (Role-based access control)

**Estimated Testing Time:** 2 hours

---

## 📋 Pre-Test Checklist

Before starting, ensure you have:
- [ ] Docker installed (for PostgreSQL)
- [ ] Rust toolchain installed
- [ ] OpenSSL or PowerShell (for certificate generation)
- [ ] C++ build tools (for client)
- [ ] Qt 6 installed (for client)

---

## 🚀 Part 1: Database Setup & Integration (30 minutes)

### 1.1 Start PostgreSQL

```powershell
cd c:\dev\VoIP-System
docker-compose up -d postgres
```

**✅ Expected Output:**
```
Creating network "voip-system_default" with the default driver
Creating volume "voip-system_postgres_data" with default driver
Creating voip-postgres ... done
```

**Verify:**
```powershell
docker ps
```
Should show `voip-postgres` running on port 5432

### 1.2 Build and Start Server

```powershell
cd server
cargo build --release
cargo run --release
```

**✅ Expected Logs:**
```
🎤 VoIP Server v0.1.0
⚙️  Configuration loaded
   Control port: 9000
   Voice port: 9001
   Database: postgresql://voip:****@localhost/voip
🔌 Initializing database...
✅ Database connection pool created
🔄 Running database migrations...
✅ Database migrations completed successfully
✅ Database ready
✅ Repositories initialized
✅ Channel manager initialized
✅ Voice router initialized
✅ Permission checker initialized
```

**If you see TLS warnings, that's okay for now.**

### 1.3 Test Database Connection

**Manual verification:**
```powershell
docker exec -it voip-postgres psql -U voip -d voip
```

**Inside PostgreSQL:**
```sql
-- List all users (should show seed data)
SELECT id, username, org_id FROM users;

-- List all roles
SELECT id, name, permissions FROM roles;

-- List all channels
SELECT id, name, parent_id FROM channels;

-- Exit
\q
```

**✅ Expected Results:**
- 4 users: demo, alice, bob, charlie
- 4 roles: Admin, Officer, Member, Guest
- 5 channels: General, Operations, Alpha Squad, Bravo Squad, Social

### 1.4 Test User Authentication

**From the server logs, you should see:**
- No errors
- Database queries executing successfully

**Next:** Move to Part 2 to test with the client.

---

## 🔐 Part 2: TLS/HTTPS Testing (20 minutes)

### 2.1 Generate TLS Certificates

```powershell
cd c:\dev\VoIP-System\server
.\tools\generate_certs.ps1
```

**✅ Expected Output:**
```
🔐 Generating TLS certificates for development...
   Domain: localhost
   Valid for: 365 days

📝 Using OpenSSL to generate certificates...
   Generating private key...
   Generating self-signed certificate...

✅ Certificate generation complete!

📁 Files created:
   Private key: certs\key.pem
   Certificate: certs\cert.pem
```

**Verify files exist:**
```powershell
ls certs/
```

Should show `cert.pem` and `key.pem`.

### 2.2 Restart Server with TLS

```powershell
# Stop the server (Ctrl+C)
cargo run --release
```

**✅ Expected Logs (NEW):**
```
✅ TLS certificates loaded successfully
Starting SECURE WebSocket server (WSS) on 0.0.0.0:9000
✅ WebSocket control server started
```

**If you see:**
```
⚠️  Failed to load TLS certificates: ...
Starting INSECURE WebSocket server (WS) on 0.0.0.0:9000
```
→ Check that `certs/cert.pem` and `certs/key.pem` exist.

### 2.3 Test WSS Connection (Client Side)

1. **Build and start client:**
   ```powershell
   cd c:\dev\VoIP-System\client\build
   .\Release\voip-client.exe
   ```

2. **In login dialog:**
   - Server: `localhost`
   - Port: `9000`
   - ✅ **Check "Use TLS/SSL (wss://)"**  ← IMPORTANT!
   - Username: `demo`
   - Password: `demo123`
   - Click "Login"

**✅ Expected (Client Console):**
```
Connecting to WebSocket: wss://localhost:9000
WebSocket connect() initiated (async)
✅ WebSocket connected!
```

**✅ Expected (Server Logs):**
```
New WebSocket connection
✅ User demo (ID: 1) authenticated and registered
```

### 2.4 Test WS Fallback (Without TLS)

1. **Rename certs folder:**
   ```powershell
   cd server
   mv certs certs.bak
   cargo run --release
   ```

2. **Expected:**
   ```
   ⚠️  Failed to load TLS certificates: Certificate file not found
   Starting INSECURE WebSocket server (WS) on 0.0.0.0:9000
   ```

3. **In client:**
   - ☐ **Uncheck "Use TLS/SSL (wss://)"**
   - Click "Login"

**✅ Should still connect via `ws://` (insecure).**

4. **Restore certs:**
   ```powershell
   mv certs.bak certs
   ```

---

## 🔑 Part 3: Permissions & ACL Testing (45 minutes)

### 3.1 Test Channel Password Protection

**Setup:**
1. Login as `demo` user (password: `demo123`)
2. Try to join a channel

**Note:** In the seed data, channels don't have passwords set by default.

**To test password protection:**

```sql
-- In PostgreSQL:
docker exec -it voip-postgres psql -U voip -d voip

-- Add password to a channel (hash for "secret123")
UPDATE channels SET password_hash = '$argon2id$v=19$m=19456,t=2,p=1$<your_hash>' WHERE id = 1;
```

**Generate password hash:**
```powershell
cd server
cargo run --bin generate_password_hash secret123
```

**Expected behavior:**
- Joining without password → Permission denied
- Joining with wrong password → Permission denied
- Joining with correct password → Success

### 3.2 Test Role-Based Permissions

**Current Status:**
- All authenticated users have full permissions (development mode)
- Role checking is implemented but needs role assignments

**To test role permissions:**

#### Step 1: Assign roles to users

**Via WebSocket messages (you'll need to implement client support or use a tool like `wscat`):**

```json
{
  "type": "assign_role",
  "user_id": 2,
  "role_id": 3
}
```

**Response:**
```json
{
  "type": "role_operation_result",
  "success": true,
  "message": "Role 3 assigned to user 2"
}
```

#### Step 2: List roles

```json
{
  "type": "list_roles",
  "org_id": 1
}
```

**Response:**
```json
{
  "type": "roles_list",
  "roles": [
    {
      "id": 1,
      "org_id": 1,
      "name": "Admin",
      "permissions": 31,
      "priority": 1000
    },
    ...
  ]
}
```

#### Step 3: Get user roles

```json
{
  "type": "get_user_roles",
  "user_id": 1
}
```

**Response:**
```json
{
  "type": "user_roles_list",
  "user_id": 1,
  "roles": [...]
}
```

### 3.3 Test Channel ACL

**Set channel-specific permissions:**

```json
{
  "type": "set_channel_acl",
  "channel_id": 1,
  "role_id": 3,
  "permissions": 3
}
```

Where `permissions` is a bitflag:
- `1` (JOIN) = 0b0001
- `2` (SPEAK) = 0b0010
- `3` (JOIN + SPEAK) = 0b0011
- `4` (WHISPER) = 0b0100
- `8` (MANAGE) = 0b1000
- `16` (KICK) = 0b10000

**Response:**
```json
{
  "type": "role_operation_result",
  "success": true,
  "message": "ACL set for channel 1 role 3"
}
```

### 3.4 Manual Database Verification

```sql
-- Check user roles
SELECT u.username, r.name as role_name, r.permissions
FROM users u
JOIN user_roles ur ON u.id = ur.user_id
JOIN roles r ON ur.role_id = r.id
ORDER BY u.username;

-- Check channel ACLs
SELECT c.name as channel_name, r.name as role_name, ca.permissions
FROM channel_acl ca
JOIN channels c ON ca.channel_id = c.id
JOIN roles r ON ca.role_id = r.id
ORDER BY c.name, r.name;

-- Check effective permissions for a user in a channel
-- (This is what the permission checker does internally)
```

---

## 🧪 Part 4: Integration Testing (25 minutes)

### 4.1 Multi-User Scenario

**Goal:** Test multiple users joining the same channel.

**Setup:**
1. **User 1:** Login as `demo` / `demo123`
2. **User 2:** Login as `alice` / `alice123` (separate client instance)
3. **User 3:** Login as `bob` / `bob123` (separate client instance)

**Test Steps:**
1. All users join channel #1 (General)
2. **Expected:** Each user sees others in the user list
3. One user speaks (PTT)
4. **Expected:** Other users see "speaking" indicator
5. One user leaves
6. **Expected:** Others receive "user left" notification

**✅ Success Criteria:**
- All users successfully join
- User lists are synchronized
- Speaking indicators work
- Leave notifications received

### 4.2 Permission Denial Test

**Goal:** Verify permission checking works.

**Setup:**
1. Create a test user with limited permissions
2. Assign only "Member" role (JOIN + SPEAK, no MANAGE)

**Test:**
1. Try to assign a role to another user
   - **Expected:** Success (for now, as we're not checking admin perms yet)
   - **TODO:** Add admin-only check for this operation

2. Try to join a password-protected channel without password
   - **Expected:** Permission denied error

3. Try to join a channel without JOIN permission
   - **Expected:** Permission denied error

### 4.3 Data Persistence Test

**Goal:** Verify database persists data across restarts.

**Test Steps:**
1. Register a new user: `testuser` / `testpass123`
2. Assign a role to this user
3. Stop the server (Ctrl+C)
4. Restart the server
5. Login as `testuser`
   - **Expected:** Login succeeds (password verified from database)
6. Check user's roles
   - **Expected:** Roles are still assigned

**✅ Success Criteria:**
- User can login after restart
- Roles persist across restarts
- No data loss

---

## 📊 Part 5: Performance & Load Testing (Optional, 30 minutes)

### 5.1 Connection Load Test

**Using `wscat` (install: `npm install -g wscat`):**

```powershell
# Connect 10 clients
for ($i=1; $i -le 10; $i++) {
    Start-Process wscat -ArgumentList "-c wss://localhost:9000/control"
}
```

**Monitor server logs** for:
- Connection count
- Memory usage (Task Manager)
- CPU usage

**✅ Expected:**
- All 10 connections succeed
- Memory increases linearly (~6MB per connection)
- CPU remains reasonable (<10% on modern hardware)

### 5.2 Database Query Performance

**Check query times in logs:**
```
# Server logs should show:
User authentication: ~60ms (includes Argon2 verification)
Channel join: ~10ms
Permission check: ~5ms
```

**If queries are slow:**
- Check database indexes are created
- Verify connection pool is not exhausted
- Run `EXPLAIN ANALYZE` on slow queries

### 5.3 Message Throughput

**Send rapid messages and measure:**
- Message latency (ping to pong)
- Messages per second

**Tools:**
- Custom WebSocket stress test script
- Or use `k6` load testing tool

---

## 🐛 Common Issues & Solutions

### Issue 1: Database Connection Failed

**Error:** `Failed to connect to database`

**Solutions:**
1. Check PostgreSQL is running: `docker ps`
2. Verify port 5432 is not in use: `netstat -an | findstr 5432`
3. Check credentials in config
4. Restart Docker: `docker-compose restart postgres`

### Issue 2: Migrations Failed

**Error:** `Database migrations completed with errors`

**Solutions:**
1. Reset database:
   ```powershell
   docker-compose down -v
   docker-compose up -d postgres
   cargo run --release
   ```
2. Check migration files are valid SQL
3. Verify database user has CREATE permissions

### Issue 3: TLS Certificate Errors

**Error:** `Failed to load TLS certificates`

**Solutions:**
1. Regenerate certificates: `.\tools\generate_certs.ps1`
2. Check file permissions: `ls -l certs/`
3. Verify paths in config match actual file locations

### Issue 4: Permission Denied (Unexpected)

**Error:** User can't join channel despite having correct role

**Debug Steps:**
1. Check user has roles assigned:
   ```sql
   SELECT * FROM user_roles WHERE user_id = <user_id>;
   ```
2. Check channel ACL exists:
   ```sql
   SELECT * FROM channel_acl WHERE channel_id = <channel_id>;
   ```
3. Check logs for permission checking details
4. Verify bitflags are correct: `JOIN = 1, SPEAK = 2`

### Issue 5: Client Can't Connect

**Error:** WebSocket connection refused

**Solutions:**
1. Verify server is running
2. Check TLS checkbox matches server mode (WSS vs WS)
3. Verify firewall allows port 9000
4. Check server logs for errors
5. Try connecting without TLS first

---

## ✅ Test Results Checklist

After completing all tests, verify:

### Database Integration
- [ ] PostgreSQL starts successfully
- [ ] Migrations run without errors
- [ ] Seed data is loaded
- [ ] Users can register and login
- [ ] Passwords are hashed (Argon2)
- [ ] Data persists across restarts

### TLS/HTTPS
- [ ] Certificates generate successfully
- [ ] Server starts in WSS mode with certs
- [ ] Server falls back to WS without certs
- [ ] Client can connect via wss://
- [ ] Client can connect via ws://
- [ ] TLS checkbox works correctly

### Permissions/ACL
- [ ] Channel password protection works
- [ ] Role assignment works
- [ ] Role removal works
- [ ] List roles works
- [ ] Get user roles works
- [ ] Channel ACL can be set
- [ ] Permission checks prevent unauthorized access
- [ ] Effective permissions calculated correctly

### Integration
- [ ] Multiple users can join same channel
- [ ] User lists synchronize
- [ ] Speaking indicators work
- [ ] Leave notifications work
- [ ] No memory leaks observed
- [ ] Performance is acceptable

---

## 📝 Reporting Issues

If you find issues during testing:

1. **Note the exact error message**
2. **Save server logs** to a file:
   ```powershell
   cargo run --release > test-logs.txt 2>&1
   ```
3. **Note the steps to reproduce**
4. **Check database state** if applicable
5. **Include system information**:
   - OS version
   - Docker version
   - Rust version
   - Client build configuration

---

## 🎯 Success Criteria

Testing is considered successful when:
- ✅ All database operations work correctly
- ✅ TLS encryption functions properly
- ✅ Permissions are enforced
- ✅ No crashes or panics observed
- ✅ Data persists correctly
- ✅ Performance is acceptable
- ✅ All critical paths tested

---

## 🚀 Next Steps After Testing

**If all tests pass:**
1. Deploy to staging environment
2. Conduct user acceptance testing
3. Implement monitoring (Phase 6)
4. Add voice encryption (Phase 7)

**If tests fail:**
1. Document all failures
2. Fix critical bugs first
3. Re-test after fixes
4. Iterate until stable

---

## 📚 Additional Resources

- [Database Setup Guide](PHASE_2_DATABASE_SETUP.md)
- [TLS Setup Guide](PHASE_3_TLS_SETUP.md)
- [Complete Implementation Summary](PHASES_2_3_5_COMPLETE.md)
- [Server Logs Location](server/logs/)
- [Database Schema](server/migrations/001_initial_schema.sql)

---

**Happy Testing!** 🎉

If you encounter any issues, refer to the troubleshooting section or check the detailed setup guides.
