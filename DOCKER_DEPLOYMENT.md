# Docker Deployment Guide - VoIP Server

Complete guide for deploying the VoIP server using Docker containers.

---

## Overview

The Docker setup includes:
- **Multi-stage Dockerfile** for optimized server image (~150 MB)
- **docker-compose.yml** for easy orchestration
- **PostgreSQL** database container
- **Redis** cache container (optional)
- **Automatic migrations** on startup

---

## Prerequisites

### Local Testing (Windows)

1. **Install Docker Desktop**
   - Download: https://www.docker.com/products/docker-desktop/
   - Minimum: 4 GB RAM, 20 GB disk space
   - Enable: WSL 2 backend

2. **Start Docker Desktop**
   - Open Docker Desktop application
   - Wait for "Docker Desktop is running" status
   - Verify: `docker --version`

### VPS Deployment (Linux)

1. **Install Docker**
```bash
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
# Logout and login again
```

2. **Install Docker Compose**
```bash
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
docker-compose --version
```

---

## Quick Start (Local)

### Step 1: Start Docker Desktop

**Windows**:
- Open "Docker Desktop" from Start Menu
- Wait for green "Running" status
- Check system tray for Docker icon

### Step 2: Build and Run

```powershell
# Navigate to project directory
cd C:\dev\VoIP-System

# Build and start all services
docker-compose up --build -d

# Check logs
docker-compose logs -f voip-server
```

**Expected output**:
```
voip-server | 🎤 VoIP Server v0.1.0
voip-server | ⚙️  Configuration loaded
voip-server | 🔌 Initializing database...
voip-server | ✅ Database ready
voip-server | ✅ Repositories initialized
voip-server | 🚀 Server initialization complete
voip-server | 📡 Voice: UDP port 9001
voip-server | 🔐 Control: WebSocket port 9000
```

### Step 3: Test Connection

```powershell
# Test WebSocket port
Test-NetConnection -ComputerName localhost -Port 9000

# Should show: TcpTestSucceeded : True
```

### Step 4: Stop Services

```powershell
# Stop all containers
docker-compose down

# Stop and remove volumes (fresh start)
docker-compose down -v
```

---

## Configuration

### Environment Variables

Create `.env` file in project root:

```bash
# Database credentials
POSTGRES_PASSWORD=your_secure_password_here

# Security
JWT_SECRET=your_long_random_secret_key_minimum_32_characters

# Optional: Redis
# REDIS_PASSWORD=your_redis_password
```

**Important**: Change these in production!

### Custom Configuration

Edit `docker-compose.yml` to customize:

```yaml
environment:
  # Increase connections
  VOIP__SERVER__MAX_CONNECTIONS: 2000

  # Adjust audio quality
  VOIP__AUDIO__MAX_BITRATE: 256000

  # Change ports (also update ports section)
  VOIP__SERVER__CONTROL_PORT: 8000
  VOIP__SERVER__VOICE_PORT: 8001
```

---

## TLS/SSL Certificates

### Option 1: Without TLS (Development)

Server runs in insecure mode (WS instead of WSS).

**Works fine for**:
- Local testing
- Behind reverse proxy (nginx, Cloudflare Tunnel)
- Internal networks

### Option 2: With TLS (Production)

**Generate self-signed certificates**:

```bash
# Create certs directory
mkdir certs
cd certs

# Generate certificate
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=yourdomain.com"

cd ..
```

**Mount certificates in docker-compose.yml**:

```yaml
  voip-server:
    volumes:
      - ./certs:/app/certs:ro  # Uncomment this line
```

**Restart**:

```bash
docker-compose restart voip-server
```

### Option 3: Let's Encrypt (VPS)

Use reverse proxy (nginx) with Let's Encrypt:

```bash
# Install nginx and certbot
sudo apt install nginx certbot python3-certbot-nginx

# Get certificate
sudo certbot --nginx -d yourdomain.com

# Configure nginx (see NGINX_REVERSE_PROXY.md)
```

---

## Building the Image

### Local Build

```bash
# Build only the server image
docker build -t voip-server:latest -f server/Dockerfile server/

# Check image size
docker images voip-server

# Expected: ~150-200 MB
```

### Build for Different Architecture

```bash
# Build for ARM64 (VPS with ARM CPU)
docker buildx build --platform linux/arm64 -t voip-server:arm64 -f server/Dockerfile server/

# Build for AMD64 (most VPS)
docker buildx build --platform linux/amd64 -t voip-server:amd64 -f server/Dockerfile server/
```

---

## Running in Production (VPS)

### Step 1: Transfer Files to VPS

```bash
# From local machine
scp -r VoIP-System/ user@your-vps-ip:/home/user/

# Or use git
ssh user@your-vps-ip
git clone https://github.com/yourusername/voip-system.git
cd voip-system
```

### Step 2: Configure Environment

```bash
# Create .env file
nano .env
```

Add:
```bash
POSTGRES_PASSWORD=strong_random_password_here
JWT_SECRET=very_long_random_secret_key_at_least_32_chars
```

### Step 3: Start Services

```bash
# Build and start
docker-compose up -d

# Check status
docker-compose ps

# View logs
docker-compose logs -f voip-server
```

### Step 4: Configure Firewall

```bash
# Allow ports
sudo ufw allow 9000/tcp   # WebSocket control
sudo ufw allow 9001/udp   # Voice UDP
sudo ufw enable
```

### Step 5: Test External Access

```bash
# From local machine
Test-NetConnection -ComputerName your-vps-ip -Port 9000

# Should show: TcpTestSucceeded : True
```

---

## Docker Commands Reference

### Container Management

```bash
# Start services
docker-compose up -d

# Stop services
docker-compose down

# Restart single service
docker-compose restart voip-server

# View logs
docker-compose logs -f voip-server

# Execute command in container
docker-compose exec voip-server /bin/sh

# View running containers
docker-compose ps

# Remove all containers and volumes (fresh start)
docker-compose down -v
```

### Image Management

```bash
# List images
docker images

# Remove old images
docker image prune -a

# Build without cache
docker-compose build --no-cache

# Pull latest base images
docker-compose pull
```

### Database Management

```bash
# Access PostgreSQL
docker-compose exec postgres psql -U voip -d voip

# Backup database
docker-compose exec postgres pg_dump -U voip voip > backup.sql

# Restore database
docker-compose exec -T postgres psql -U voip voip < backup.sql

# View database logs
docker-compose logs postgres
```

---

## Troubleshooting

### "Docker Desktop is not running"

**Error**:
```
ERROR: error during connect: Head "http://...": The system cannot find the file specified.
```

**Fix**:
1. Open Docker Desktop application
2. Wait for "Running" status
3. Try command again

### "Port already in use"

**Error**:
```
ERROR: for voip-server Cannot start service: Ports are not available: listen tcp 0.0.0.0:9000: bind: address already in use
```

**Fix**:
```bash
# Find process using port
netstat -ano | findstr :9000

# Kill process (replace PID)
taskkill /F /PID <PID>

# Or change port in docker-compose.yml
ports:
  - "9002:9000"  # Map host 9002 to container 9000
```

### "Database connection failed"

**Error**:
```
voip-server | ERROR: Database connection failed
```

**Fix**:
```bash
# Check if postgres is running
docker-compose ps

# View postgres logs
docker-compose logs postgres

# Restart postgres
docker-compose restart postgres

# If still fails, recreate
docker-compose down -v
docker-compose up -d
```

### "Build failed: network timeout"

**Error during build**:
```
ERROR: failed to solve: failed to fetch ...
```

**Fix**:
```bash
# Retry with more time
docker-compose build

# Or build manually
docker build --network=host -t voip-server -f server/Dockerfile server/
```

### "Container keeps restarting"

**Check why**:
```bash
# View logs
docker-compose logs --tail=50 voip-server

# Check container status
docker inspect voip-server

# Common causes:
# - Database not ready (wait 30 seconds)
# - Missing environment variables
# - Port conflicts
```

---

## Health Checks

### Container Health

```bash
# Check health status
docker-compose ps

# Should show: Up (healthy)
```

### Manual Health Check

```bash
# Check if server binary exists
docker-compose exec voip-server test -f /app/voip-server && echo "OK" || echo "FAIL"

# Check listening ports
docker-compose exec voip-server netstat -tuln | grep -E '9000|9001'

# Check database connection
docker-compose exec postgres pg_isready -U voip
```

---

## Performance Tuning

### Database

```yaml
# In docker-compose.yml under postgres service
environment:
  POSTGRES_SHARED_BUFFERS: 256MB
  POSTGRES_MAX_CONNECTIONS: 100
  POSTGRES_EFFECTIVE_CACHE_SIZE: 1GB
```

### Server

```yaml
# In docker-compose.yml under voip-server
environment:
  VOIP__DATABASE__MAX_CONNECTIONS: 50
  VOIP__SERVER__MAX_CONNECTIONS: 2000
```

### Resource Limits

```yaml
# Limit CPU and memory
voip-server:
  deploy:
    resources:
      limits:
        cpus: '1.0'
        memory: 512M
      reservations:
        cpus: '0.5'
        memory: 256M
```

---

## Monitoring

### Docker Stats

```bash
# Real-time resource usage
docker stats

# Watch specific container
docker stats voip-server
```

### Logs

```bash
# Tail logs
docker-compose logs -f --tail=100 voip-server

# Save logs to file
docker-compose logs voip-server > server.log

# Filter logs
docker-compose logs voip-server | grep ERROR
```

---

## Upgrading

### Update Server Code

```bash
# Pull latest code
git pull

# Rebuild and restart
docker-compose build voip-server
docker-compose up -d voip-server

# Check logs
docker-compose logs -f voip-server
```

### Update Base Images

```bash
# Pull latest base images
docker-compose pull

# Rebuild
docker-compose build --no-cache

# Restart
docker-compose up -d
```

---

## Backup and Restore

### Backup Everything

```bash
# Backup database
docker-compose exec postgres pg_dump -U voip voip > backup_$(date +%Y%m%d).sql

# Backup volumes
docker run --rm \
  -v voip-system_postgres_data:/data \
  -v $(pwd):/backup \
  alpine tar czf /backup/postgres_backup.tar.gz /data
```

### Restore

```bash
# Restore database
docker-compose exec -T postgres psql -U voip voip < backup_20241212.sql

# Restore volumes
docker run --rm \
  -v voip-system_postgres_data:/data \
  -v $(pwd):/backup \
  alpine sh -c "cd /data && tar xzf /backup/postgres_backup.tar.gz --strip 1"
```

---

## Security Checklist

- [ ] Change default passwords in `.env`
- [ ] Generate strong JWT secret (32+ characters)
- [ ] Enable TLS/SSL certificates
- [ ] Configure firewall (allow only 9000, 9001)
- [ ] Regular backups automated
- [ ] Update images regularly
- [ ] Monitor logs for suspicious activity
- [ ] Use non-root user in containers (already configured)
- [ ] Limit resource usage (CPU, memory)
- [ ] Secure docker socket (don't expose)

---

## Next Steps

1. **Local Testing**: Start with `docker-compose up` locally
2. **VPS Deployment**: Follow VPS section above
3. **Client Configuration**: Update clients to connect to VPS IP
4. **Monitoring**: Set up log monitoring
5. **Backups**: Automate database backups

See also:
- [VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md) - Detailed VPS setup
- [NGINX_REVERSE_PROXY.md](./NGINX_REVERSE_PROXY.md) - nginx configuration
- [TESTING_DOCKER.md](./TESTING_DOCKER.md) - Testing procedures
