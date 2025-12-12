# Phase 2: Database Integration - Setup Guide

**Created:** November 27, 2025
**Status:** ✅ Implementation Complete - Ready for Testing

---

## 🎯 What Was Implemented

Phase 2 adds complete PostgreSQL database integration to the VoIP server:

✅ Database connection pool with SQLx
✅ Automatic migrations
✅ User repository (CRUD operations with Argon2)
✅ Channel repository (CRUD operations)
✅ Organization repository (CRUD operations)
✅ Health checks and statistics
✅ Docker Compose for easy PostgreSQL setup
✅ Seed data with proper password hashing

---

## 🚀 Quick Start - Option 1: Docker (Recommended)

###  1. Start PostgreSQL with Docker Compose

```powershell
cd c:\dev\VoIP-System
docker-compose up -d postgres
```

**This will:**
- Start PostgreSQL 15 on port 5432
- Create database `voip` with user `voip` / password `voip`
- Persist data in a Docker volume
- Auto-start on system boot

**Verify it's running:**
```powershell
docker ps
docker logs voip-postgres
```

Expected output:
```
PostgreSQL init process complete; ready for start up.
database system is ready to accept connections
```

### 2. Build the Server

```powershell
cd c:\dev\VoIP-System\server
cargo build --release
```

### 3. Run Migrations

The server will automatically run migrations on startup, but you can also run them manually:

```powershell
# Install sqlx-cli if not already installed
cargo install sqlx-cli --no-default-features --features postgres

# Run migrations
cd c:\dev\VoIP-System\server
sqlx migrate run --database-url postgresql://voip:voip@localhost/voip
```

### 4. Start the Server

```powershell
cd c:\dev\VoIP-System\server
cargo run --release
```

**Expected logs:**
```
🎤 VoIP Server v0.1.0
⚙️  Configuration loaded
   Control port: 9000
   Voice port: 9001
   Database: localhost/voip
🔌 Connecting to database: postgresql://voip:****@localhost/voip
✅ Database connection pool created (20 max connections)
🔄 Running database migrations...
✅ Database migrations completed successfully
✅ Database ready
✅ Repositories initialized
✅ Channel manager initialized
✅ Voice router initialized
🎙️  Starting UDP voice server on 0.0.0.0:9001
✅ UDP voice server started
🔐 Starting WebSocket control server on 0.0.0.0:9000
✅ WebSocket control server started
🚀 Server initialization complete

📡 Voice: UDP port 9001
🔐 Control: WebSocket port 9000
💾 Database: PostgreSQL connected

Press Ctrl+C to shutdown
```

---

## 🐳 Docker Compose Services

The [docker-compose.yml](docker-compose.yml) includes:

### PostgreSQL (Active)
- **Image:** postgres:15-alpine
- **Port:** 5432
- **Database:** voip
- **User:** voip / voip
- **Volume:** postgres_data (persistent)
- **Health Check:** Automatic

### Redis (Optional - Not used yet)
- **Image:** redis:7-alpine
- **Port:** 6379
- **Ready for Phase 3 (caching)**

### pgAdmin (Commented Out)
Uncomment in docker-compose.yml to enable web-based database management:
- **URL:** http://localhost:5050
- **Login:** admin@voip.local / admin

---

## 🛠️ Alternative Setup: Local PostgreSQL

If you prefer installing PostgreSQL directly:

### Windows

1. **Download PostgreSQL 15+:**
   - https://www.postgresql.org/download/windows/
   - Or use Chocolatey: `choco install postgresql`

2. **Create Database:**
   ```powershell
   psql -U postgres
   CREATE DATABASE voip;
   CREATE USER voip WITH PASSWORD 'voip';
   GRANT ALL PRIVILEGES ON DATABASE voip TO voip;
   \q
   ```

3. **Update Connection String (if needed):**
   Edit `server/config/server.yaml` or set environment variable:
   ```powershell
   $env:VOIP_DATABASE__URL="postgresql://voip:voip@localhost/voip"
   ```

### Linux/macOS

```bash
# Install PostgreSQL
sudo apt install postgresql postgresql-contrib  # Ubuntu/Debian
brew install postgresql@15                       # macOS

# Create database
sudo -u postgres psql
CREATE DATABASE voip;
CREATE USER voip WITH PASSWORD 'voip';
GRANT ALL PRIVILEGES ON DATABASE voip TO voip;
\q
```

---

## 📊 Database Schema Overview

The database includes 8 tables:

### Core Tables

**organizations** - Multi-tenant support
- id, name, tag (unique), owner_id, max_users, max_channels

**users** - User accounts
- id, org_id, username, email, password_hash (Argon2)
- Unique constraint: (org_id, username)

**channels** - Voice channels
- id, org_id, parent_id (hierarchy), name, description, password_hash, max_users

**roles** - Permission groups
- id, org_id, name, permissions (bitflags), priority

### Relationship Tables

**user_roles** - User-to-role mapping (many-to-many)
**channel_acl** - Channel-specific role permissions

### System Tables

**sessions** - Active JWT sessions (UUID, token_hash, expires_at)
**audit_log** - Security audit trail (action, resource, details)

### Seed Data

The migrations automatically create:
- **1 Organization:** "Demo Organization" (tag: DEMO)
- **4 Users:** demo, alice, bob, charlie (all password: `{username}123`)
- **4 Roles:** Admin, Officer, Member, Guest
- **5 Channels:** General, Operations, Alpha Squad, Bravo Squad, Social
- **Channel ACLs:** Proper permission setup

---

## 🔧 Configuration

### Environment Variables

Override defaults with environment variables:

```powershell
# Database
$env:VOIP_DATABASE__URL="postgresql://voip:voip@localhost/voip"
$env:VOIP_DATABASE__MAX_CONNECTIONS="20"

# Server
$env:VOIP_SERVER__CONTROL_PORT="9000"
$env:VOIP_SERVER__VOICE_PORT="9001"

# Security
$env:VOIP_SECURITY__JWT_SECRET="your-secret-key-here"
```

### Config File

Create `server/config/server.yaml`:

```yaml
database:
  url: "postgresql://voip:voip@localhost/voip"
  max_connections: 20

server:
  bind_address: "0.0.0.0"
  control_port: 9000
  voice_port: 9001

security:
  jwt_secret: "CHANGE_ME_IN_PRODUCTION"
```

---

## 🧪 Testing Database Integration

### 1. Test User Authentication

```powershell
# From client
# Login with: demo / demo123
# Should authenticate successfully
```

**Server logs should show:**
```
✅ User authenticated: demo (ID: 1)
✅ Registered user demo (ID: 1) for WebSocket broadcasts
```

### 2. Test User Registration

```powershell
# From client
# Register new user: testuser / testpass123
```

**Server logs should show:**
```
✅ User created: testuser (ID: 5, Org: 1)
```

### 3. Verify Data Persistence

```powershell
# Stop server (Ctrl+C)
# Restart server
# Login with newly registered user
# Should work! (data persisted)
```

### 4. Check Database Directly

```powershell
# Connect to database
docker exec -it voip-postgres psql -U voip -d voip

# Query users
SELECT id, username, org_id FROM users;

# Query channels
SELECT id, name, parent_id FROM channels;

# Exit
\q
```

---

## 🐛 Troubleshooting

### Issue: "Failed to connect to database"

**Check 1: Is PostgreSQL running?**
```powershell
docker ps | Select-String postgres
# OR
netstat -an | Select-String "5432"
```

**Check 2: Can you connect manually?**
```powershell
psql -h localhost -U voip -d voip
# Password: voip
```

**Check 3: Firewall blocking?**
- Windows: Allow port 5432 in Windows Firewall
- Docker: Ensure port mapping is correct

### Issue: "Migration failed"

**Solution 1: Reset database**
```powershell
docker-compose down -v  # Remove volumes
docker-compose up -d postgres
cargo run --release     # Re-run migrations
```

**Solution 2: Manual migration**
```powershell
cd server
sqlx migrate run --database-url postgresql://voip:voip@localhost/voip
```

### Issue: "Too many connections"

**Increase max_connections:**

Edit `docker-compose.yml`:
```yaml
postgres:
  command: postgres -c max_connections=100
```

Or update config:
```yaml
database:
  max_connections: 50  # Default is 20
```

---

## 📚 Database Operations Reference

### User Operations

**Create user:**
```rust
let user_id = user_repo.create_user(org_id, username, Some(email), password).await?;
```

**Authenticate:**
```rust
let user = user_repo.authenticate(username, password).await?;
```

**Get user by ID:**
```rust
let user = user_repo.get_user_by_id(user_id).await?;
```

### Channel Operations

**Create channel:**
```rust
let channel_id = channel_repo.create_channel(
    org_id, parent_id, name, description,
    password, max_users, position
).await?;
```

**Get channels:**
```rust
let channels = channel_repo.get_channels_by_org(org_id).await?;
```

### Organization Operations

**Create org:**
```rust
let org_id = org_repo.create_organization(
    name, tag, owner_id, max_users, max_channels
).await?;
```

---

## 🔐 Security Notes

### Password Hashing

All passwords are hashed with **Argon2id**:
- Memory cost: 19456 KiB
- Time cost: 2 iterations
- Parallelism: 1
- Salt: 128-bit random (per-password)

**Never store plaintext passwords!**

### JWT Tokens

- Algorithm: HS256 (HMAC-SHA256)
- Default expiration: 1 hour
- Includes: user_id, org_id, roles, jti (revocation tracking)

**Change JWT_SECRET in production!**

### Database Credentials

**Development:**
- User: voip
- Password: voip
- Database: voip

**Production:** Use strong passwords and encrypted connections!

---

## 📈 Performance Tuning

### Connection Pooling

Default: 20 connections
```yaml
database:
  max_connections: 20  # Adjust based on load
```

**Rule of thumb:**
- For 100 concurrent users: 20-30 connections
- For 1000 concurrent users: 50-100 connections

### Indexes

All critical indexes are included in migrations:
- `idx_users_org` - User lookups by organization
- `idx_users_username` - Authentication queries
- `idx_channels_org` - Channel listing
- `idx_sessions_expires` - Session cleanup

### Query Optimization

All queries use prepared statements (SQLx compile-time verification).

---

## 🎯 What's Next

Phase 2 is complete! Next steps:

**Phase 3: TLS/HTTPS** (2-3 days)
- Secure WebSocket control channel
- Certificate management

**Phase 4: Password Security** (1 day)
- Already using Argon2! ✅
- But old UserRegistry (file-based) needs removal

**Phase 5: Permissions** (2-3 days)
- Enforce channel ACLs
- Role-based access control

**Phase 6: Load Testing** (3-5 days)
- Test with 50-100 users
- Performance optimization

---

## ✅ Phase 2 Checklist

- [✅] Database connection module created
- [✅] SQLx migrations implemented
- [✅] User repository with Argon2
- [✅] Channel repository
- [✅] Organization repository
- [✅] Docker Compose setup
- [✅] Seed data with proper hashes
- [✅] Health checks
- [✅] Auto-migration on startup
- [⏳] Integration testing (your turn!)
- [⏳] Replace old UserRegistry in main.rs

---

## 📞 Support

If you encounter issues:

1. Check server logs for detailed errors
2. Verify PostgreSQL is running: `docker ps`
3. Test database connection: `psql -h localhost -U voip -d voip`
4. Check firewall/network settings
5. Review troubleshooting section above

---

**Database integration complete! Ready for Phase 3.** 🚀
