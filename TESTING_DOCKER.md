# Docker Testing Guide - VoIP Server

Step-by-step guide to test your Docker deployment locally before deploying to VPS.

---

## Prerequisites

✅ Docker Desktop installed and **running**
✅ At least 4 GB free RAM
✅ 10 GB free disk space

---

## Test 1: Docker Installation

### Verify Docker is Running

```powershell
# Check Docker version
docker --version
# Expected: Docker version 20.10.x or higher

# Check Docker is running
docker ps
# Expected: CONTAINER ID   IMAGE   ... (may be empty)

# If error "cannot connect to Docker daemon"
# → Open Docker Desktop and wait for it to start
```

**Pass**: Docker commands work without errors
**Fail**: Start Docker Desktop application

---

## Test 2: Build Docker Image

### Build the VoIP Server Image

```powershell
# Navigate to project
cd C:\dev\VoIP-System

# Build image (takes 5-10 minutes first time)
docker build -t voip-server:test -f server/Dockerfile server/
```

**Expected output**:
```
[+] Building 180.3s (15/15) FINISHED
 => [internal] load build definition from Dockerfile
 => [builder 1/8] FROM docker.io/library/rust:1.70-slim
 => [builder 6/8] RUN cargo build --release
 => [stage-1 3/4] COPY --from=builder /app/target/release/voip-server
 => exporting to image
 => => naming to docker.io/library/voip-server:test
```

**Pass**: "Successfully tagged voip-server:test"
**Fail**: See error troubleshooting below

### Verify Image Size

```powershell
docker images voip-server:test
```

**Expected**:
```
REPOSITORY      TAG       SIZE
voip-server     test      150-200MB
```

**Pass**: Image size under 300 MB
**Fail**: Image over 500 MB (multi-stage build not working)

---

## Test 3: Start Services with Docker Compose

### Create Environment File

```powershell
# Create .env file
@"
POSTGRES_PASSWORD=test123
JWT_SECRET=test_secret_key_minimum_32_characters_long
"@ | Out-File -Encoding ASCII .env
```

### Start All Services

```powershell
# Start in background
docker-compose up -d

# Wait 30 seconds for startup
timeout 30

# Check running containers
docker-compose ps
```

**Expected output**:
```
NAME                IMAGE               STATUS              PORTS
voip-postgres       postgres:16-alpine  Up (healthy)        5432/tcp
voip-redis          redis:7-alpine      Up (healthy)        6379/tcp
voip-server         voip-server:latest  Up (healthy)        9000/tcp, 9001/udp
```

**Pass**: All 3 containers show "Up (healthy)"
**Fail**: Any container "Restarting" or "Exited" → Check logs

### View Server Logs

```powershell
# Follow server logs
docker-compose logs -f voip-server
```

**Expected output**:
```
voip-server | 🎤 VoIP Server v0.1.0
voip-server | ⚙️  Configuration loaded
voip-server | 🔌 Initializing database...
voip-server | ✅ Database ready
voip-server | ✅ Repositories initialized
voip-server | ✅ Channel manager initialized
voip-server | ✅ Voice router initialized
voip-server | ✅ UDP voice server started
voip-server | ✅ WebSocket control server started
voip-server | 🚀 Server initialization complete
```

**Pass**: See "🚀 Server initialization complete"
**Fail**: See errors → Scroll down to troubleshooting

---

## Test 4: Database Connection

### Check Database is Accessible

```powershell
# Connect to database
docker-compose exec postgres psql -U voip -d voip -c "\dt"
```

**Expected output**:
```
              List of relations
 Schema |      Name       | Type  | Owner
--------+-----------------+-------+-------
 public | users           | table | voip
 public | organizations   | table | voip
 public | channels        | table | voip
 public | roles           | table | voip
 ...
```

**Pass**: Tables listed (migrations ran successfully)
**Fail**: No tables → Check migration logs

### Verify Data

```powershell
# Count organizations (should have default org)
docker-compose exec postgres psql -U voip -d voip -c "SELECT COUNT(*) FROM organizations;"
```

**Expected**: At least 1 row (default organization)

---

## Test 5: Network Connectivity

### Test WebSocket Port (TCP 9000)

```powershell
# Test from host machine
Test-NetConnection -ComputerName localhost -Port 9000
```

**Expected output**:
```
ComputerName     : localhost
RemoteAddress    : ::1
RemotePort       : 9000
TcpTestSucceeded : True
```

**Pass**: `TcpTestSucceeded : True`
**Fail**: `False` → Check if container is running

### Test Voice Port (UDP 9001)

```powershell
# Check if UDP port is open (harder to test)
docker-compose exec voip-server netstat -uln | findstr 9001
```

**Expected**:
```
udp        0      0 0.0.0.0:9001            0.0.0.0:*
```

**Pass**: Port 9001 listening
**Fail**: Not shown → Check logs

---

## Test 6: API Endpoints

### Test Health Check (if implemented)

```powershell
# Try connecting to server
curl http://localhost:9000
```

**Expected**: Connection succeeds (may show error message, that's OK)

**Pass**: Connection established (not timeout)
**Fail**: "Connection refused" or timeout

---

## Test 7: Client Connection

### Test WebSocket Connection

```powershell
# Install websocat (WebSocket CLI tool)
# Download from: https://github.com/vi/websocat/releases

# Test WebSocket connection
.\websocat ws://localhost:9000
```

**Expected**: Connection established (cursor waits for input)

**Pass**: Connected without error
**Fail**: "Connection refused"

**Exit**: Press Ctrl+C

---

## Test 8: Resource Usage

### Check Container Resource Usage

```powershell
# Monitor containers
docker stats --no-stream
```

**Expected output**:
```
CONTAINER       CPU %   MEM USAGE / LIMIT    MEM %   NET I/O   BLOCK I/O
voip-server     0.5%    50MiB / 2GiB         2.5%    1kB/0B    0B/0B
voip-postgres   1.0%    80MiB / 2GiB         4.0%    2kB/0B    8kB/0B
voip-redis      0.2%    10MiB / 2GiB         0.5%    0B/0B     0B/0B
```

**Pass**:
- voip-server: < 200 MB RAM
- voip-postgres: < 150 MB RAM
- Total CPU < 10% when idle

**Fail**: High memory or CPU → May indicate issue

---

## Test 9: Logs and Debugging

### Check for Errors in Logs

```powershell
# Check all service logs
docker-compose logs | findstr ERROR

# Should be empty or only minor errors
```

**Pass**: No critical errors
**Fail**: Repeated errors → Investigate specific issue

### Test Log Streaming

```powershell
# Stream logs from all services
docker-compose logs -f

# Keep open, perform actions, watch logs
# Should see real-time activity
```

---

## Test 10: Stop and Restart

### Graceful Shutdown

```powershell
# Stop all services
docker-compose down

# Wait for clean shutdown
# Expected: No errors during shutdown
```

**Pass**: All containers stopped cleanly
**Fail**: Containers timeout → May indicate cleanup issue

### Restart and Verify

```powershell
# Start again
docker-compose up -d

# Wait 30 seconds
timeout 30

# Check all healthy
docker-compose ps

# Expected: All containers "Up (healthy)"
```

**Pass**: Services start and become healthy
**Fail**: Containers fail to start → Check logs

---

## Troubleshooting

### Error: "Docker Desktop is not running"

**Symptom**:
```
ERROR: error during connect: Head "http://...": The system cannot find the file specified.
```

**Fix**:
1. Open Docker Desktop from Start Menu
2. Wait for "Docker Desktop is running"
3. Retry command

---

### Error: "Port already in use"

**Symptom**:
```
ERROR: for voip-server  Cannot start service: Ports are not available: listen tcp 0.0.0.0:9000: bind: Only one usage of each socket address
```

**Fix**:
```powershell
# Find what's using port 9000
netstat -ano | findstr :9000

# Example output:
#   TCP    0.0.0.0:9000     0.0.0.0:0      LISTENING       12345

# Kill the process (replace 12345 with actual PID)
taskkill /F /PID 12345

# Or change port in docker-compose.yml
# ports:
#   - "9002:9000"  # Use different host port
```

---

### Error: "Database connection failed"

**Symptom**:
```
voip-server | ERROR: Database connection failed: connection refused
```

**Fix**:
```powershell
# Check if postgres is healthy
docker-compose ps postgres

# If not healthy, check logs
docker-compose logs postgres

# Restart postgres
docker-compose restart postgres

# Wait 30 seconds and check server logs
docker-compose logs voip-server
```

---

### Error: "Migrations failed"

**Symptom**:
```
voip-server | ERROR: Migration failed: ...
```

**Fix**:
```powershell
# Reset database
docker-compose down -v  # WARNING: Deletes all data!
docker-compose up -d

# Check logs
docker-compose logs -f voip-server
```

---

### Error: "Build failed: Rust compilation error"

**Symptom**:
```
error[E0XXX]: ...
error: could not compile `voip-server`
```

**Fix**:
```powershell
# Try building directly on host to see full error
cd server
cargo build --release

# Fix any code issues, then rebuild Docker image
cd ..
docker-compose build --no-cache voip-server
```

---

### Container Keeps Restarting

**Symptom**:
```
voip-server is restarting (exit code 1)
```

**Fix**:
```powershell
# Check logs for error
docker-compose logs --tail=100 voip-server

# Common causes:
# 1. Missing environment variable
# 2. Database not ready (wait longer)
# 3. Port conflict
# 4. Configuration error

# Try running with manual restart
docker-compose up voip-server
# (no -d flag, watch output)
```

---

## Test Summary Checklist

Use this checklist to verify everything works:

- [ ] Test 1: Docker running and accessible
- [ ] Test 2: Image builds successfully (150-200 MB)
- [ ] Test 3: All containers start and show "healthy"
- [ ] Test 4: Database tables created, default data present
- [ ] Test 5: TCP port 9000 accessible
- [ ] Test 6: UDP port 9001 listening
- [ ] Test 7: WebSocket connection succeeds
- [ ] Test 8: Resource usage reasonable (< 200 MB RAM)
- [ ] Test 9: No critical errors in logs
- [ ] Test 10: Clean shutdown and restart works

**If all checkboxes pass**: Ready for VPS deployment!

**If any fail**: See troubleshooting section or ask for help.

---

## Next Steps

Once all tests pass locally:

1. **Review [VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md)**
2. **Choose VPS provider** (DigitalOcean, Hetzner, etc.)
3. **Deploy to VPS** following guide
4. **Test from external network**
5. **Configure clients** to use VPS address

---

## Clean Up After Testing

```powershell
# Stop all containers
docker-compose down

# Remove volumes (delete database data)
docker-compose down -v

# Remove images
docker rmi voip-server:test

# Free up space
docker system prune -a
```

**Warning**: `docker-compose down -v` deletes all database data!
