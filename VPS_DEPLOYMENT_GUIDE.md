# VPS Deployment Guide - Docker Edition

Complete guide for deploying the VoIP server to a VPS using Docker.

**Recommended**: DigitalOcean ($6/month, easiest setup)

---

## Overview

This guide covers:
1. Creating a VPS (DigitalOcean focus)
2. Installing Docker and dependencies
3. Deploying VoIP server containers
4. Configuring firewall and networking
5. Testing external access
6. Setting up auto-updates and monitoring

**Total time**: 30-45 minutes

---

## Part 1: Create VPS

### Option A: DigitalOcean (Recommended)

**Why DigitalOcean**:
- ✅ $200 free credit (60 days)
- ✅ Easiest interface
- ✅ Best documentation
- ✅ 30-minute setup

#### Step 1: Sign Up

1. Go to: https://www.digitalocean.com/
2. Click "Sign Up"
3. Enter email, create password
4. Verify email
5. Add payment method (required for free credit)

#### Step 2: Create Droplet

1. Click "Create" → "Droplets"
2. **Choose Region**: Closest to your users
   - US: New York, San Francisco
   - EU: London, Frankfurt
   - Asia: Singapore, Bangalore
3. **Choose Image**: Ubuntu 22.04 LTS x64
4. **Choose Size**: Basic
   - $6/month: 1 GB RAM, 1 CPU, 25 GB SSD
   - **This is perfect for VoIP server**
5. **Choose Authentication**: SSH Key (recommended) or Password
6. **Hostname**: voip-server
7. Click "Create Droplet"

**Wait 60 seconds** for droplet to be created.

#### Step 3: Get IP Address

After creation, you'll see:
- **IP Address**: 123.456.789.012 (example)
- **Username**: root
- **Password**: Sent to email (if using password auth)

**Write down the IP address!**

### Option B: Hetzner ($4.50/month)

1. Go to: https://www.hetzner.com/cloud
2. Sign up, verify email
3. Create project
4. Add server:
   - Location: Closest to users
   - Image: Ubuntu 22.04
   - Type: CX11 (2 GB RAM, €4.15/month)
   - SSH key or password
5. Create server
6. Note IP address

### Option C: Other Providers

See [VPS_COMPARISON_2024.md](./VPS_COMPARISON_2024.md) for:
- Vultr
- Linode
- AWS Lightsail
- Oracle Cloud (free)

---

## Part 2: Connect to VPS

### From Windows (PowerShell)

#### If using password:

```powershell
ssh root@YOUR_VPS_IP
# Enter password when prompted
```

#### If using SSH key:

```powershell
# First time only: Create SSH key
ssh-keygen -t ed25519
# Press Enter 3 times (default location, no passphrase)

# Copy public key to VPS
type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh root@YOUR_VPS_IP "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"

# Connect
ssh root@YOUR_VPS_IP
```

**You should now see**:
```
root@voip-server:~#
```

---

## Part 3: Install Docker

### Quick Install Script

```bash
# Update system
apt update && apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh

# Install Docker Compose
curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
chmod +x /usr/local/bin/docker-compose

# Verify installation
docker --version
docker-compose --version
```

**Expected output**:
```
Docker version 24.0.x
docker-compose version v2.23.x
```

---

## Part 4: Transfer VoIP Server Files

### Option A: Git (Recommended)

```bash
# Install git
apt install -y git

# Clone repository (if you have one)
git clone https://github.com/yourusername/voip-system.git
cd voip-system
```

### Option B: SCP from Windows

```powershell
# From your local machine (new PowerShell window)
scp -r C:\dev\VoIP-System root@YOUR_VPS_IP:/root/voip-system
```

### Option C: Manual (small files)

```bash
# Create directory
mkdir -p /root/voip-system
cd /root/voip-system

# Create files manually (docker-compose.yml, etc.)
nano docker-compose.yml
# Paste content, Ctrl+X, Y, Enter
```

---

## Part 5: Configure Environment

### Create .env File

```bash
cd /root/voip-system

# Generate strong passwords
POSTGRES_PASS=$(openssl rand -base64 32)
JWT_SECRET=$(openssl rand -base64 48)

# Create .env file
cat > .env <<EOF
POSTGRES_PASSWORD=${POSTGRES_PASS}
JWT_SECRET=${JWT_SECRET}
EOF

# Verify
cat .env
```

**Output should show**:
```
POSTGRES_PASSWORD=randomstring123...
JWT_SECRET=anotherlongrandomstring...
```

---

## Part 6: Build and Start Services

### Build Docker Images

```bash
# Build server image
docker-compose build

# This takes 5-10 minutes (compiling Rust)
# You'll see: Successfully tagged ...
```

### Start Services

```bash
# Start all containers
docker-compose up -d

# Check status
docker-compose ps
```

**Expected output**:
```
NAME            IMAGE               STATUS
voip-postgres   postgres:16-alpine  Up (healthy)
voip-redis      redis:7-alpine      Up (healthy)
voip-server     voip-server:latest  Up (healthy)
```

### View Logs

```bash
# Follow server logs
docker-compose logs -f voip-server

# Should see:
# 🎤 VoIP Server v0.1.0
# ✅ Database ready
# 🚀 Server initialization complete
```

**Press Ctrl+C** to stop following logs (containers keep running).

---

## Part 7: Configure Firewall

### UFW Firewall (Ubuntu)

```bash
# Install UFW
apt install -y ufw

# Allow SSH (important! Don't lock yourself out)
ufw allow 22/tcp

# Allow VoIP ports
ufw allow 9000/tcp  # WebSocket control
ufw allow 9001/udp  # Voice UDP

# Enable firewall
ufw enable

# Verify rules
ufw status
```

**Expected output**:
```
Status: active

To                         Action      From
--                         ------      ----
22/tcp                     ALLOW       Anywhere
9000/tcp                   ALLOW       Anywhere
9001/udp                   ALLOW       Anywhere
```

---

## Part 8: Test External Access

### From Your Local Machine

```powershell
# Test WebSocket port
Test-NetConnection -ComputerName YOUR_VPS_IP -Port 9000

# Should show:
# TcpTestSucceeded : True
```

**If True**: Port forwarding works! ✅

**If False**: Check:
1. Firewall rules (`ufw status`)
2. Container is running (`docker-compose ps`)
3. Server logs (`docker-compose logs voip-server`)

### Test with Client

1. Update client to use VPS IP:
   - Server: `YOUR_VPS_IP` or domain name
   - Port: `9000`

2. Try connecting from client
3. Should connect successfully!

---

## Part 9: Set Up Domain (Optional)

### Use Your Own Domain

**If you have a domain** (example.com):

1. **Add A Record** in DNS:
   - Type: `A`
   - Name: `voip` (or `@` for root)
   - Value: `YOUR_VPS_IP`
   - TTL: `300`

2. **Wait 5-10 minutes** for DNS propagation

3. **Test**:
   ```bash
   nslookup voip.example.com
   # Should show YOUR_VPS_IP
   ```

4. **Update clients**:
   - Server: `voip.example.com`
   - Port: `9000`

### Free Subdomain (No-IP)

1. Go to: https://www.noip.com/
2. Sign up for free account
3. Create hostname: `yourname.ddns.net`
4. Point to YOUR_VPS_IP
5. Use `yourname.ddns.net:9000` in clients

---

## Part 10: Enable TLS/SSL (Optional)

### Option A: Self-Signed Certificate

```bash
# Create certs directory
mkdir -p /root/voip-system/certs
cd /root/voip-system/certs

# Generate certificate
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=YOUR_DOMAIN_OR_IP"

# Update docker-compose.yml to mount certs
nano /root/voip-system/docker-compose.yml
```

**Uncomment this line**:
```yaml
volumes:
  - ./certs:/app/certs:ro  # Remove the # at start of line
```

**Restart server**:
```bash
docker-compose restart voip-server
```

**Clients now use**:
- Protocol: `wss://` (instead of `ws://`)
- Server: `YOUR_VPS_IP:9000`

### Option B: Let's Encrypt (with domain)

**Requires**: Own domain pointing to VPS

```bash
# Install nginx and certbot
apt install -y nginx certbot python3-certbot-nginx

# Get certificate
certbot --nginx -d voip.example.com

# Follow prompts
# Certificate saved to: /etc/letsencrypt/live/voip.example.com/

# Copy to Docker certs directory
cp /etc/letsencrypt/live/voip.example.com/fullchain.pem /root/voip-system/certs/cert.pem
cp /etc/letsencrypt/live/voip.example.com/privkey.pem /root/voip-system/certs/key.pem

# Update docker-compose.yml (same as above)
# Restart server
docker-compose restart voip-server
```

---

## Part 11: Auto-Start on Reboot

Docker Compose services already auto-restart, but ensure Docker starts on boot:

```bash
# Enable Docker service
systemctl enable docker

# Test reboot
reboot

# Wait 2 minutes, reconnect
ssh root@YOUR_VPS_IP

# Check containers
docker-compose -f /root/voip-system/docker-compose.yml ps

# Should all be running
```

---

## Part 12: Set Up Backups

### Database Backup Script

```bash
# Create backup script
cat > /root/backup-voip.sh <<'EOF'
#!/bin/bash
BACKUP_DIR="/root/voip-backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Backup database
docker-compose -f /root/voip-system/docker-compose.yml \
  exec -T postgres pg_dump -U voip voip > "$BACKUP_DIR/voip_$DATE.sql"

# Keep only last 7 days
find $BACKUP_DIR -name "voip_*.sql" -mtime +7 -delete

echo "Backup completed: voip_$DATE.sql"
EOF

# Make executable
chmod +x /root/backup-voip.sh
```

### Schedule Daily Backups

```bash
# Add to crontab
crontab -e

# Add this line (runs at 2 AM daily)
0 2 * * * /root/backup-voip.sh >> /var/log/voip-backup.log 2>&1
```

### Test Backup

```bash
# Run backup manually
/root/backup-voip.sh

# Check backup created
ls -lh /root/voip-backups/
```

---

## Part 13: Monitoring and Logs

### Check Container Health

```bash
# View all containers
docker-compose -f /root/voip-system/docker-compose.yml ps

# View resource usage
docker stats --no-stream
```

### View Logs

```bash
# Server logs
docker-compose -f /root/voip-system/docker-compose.yml logs -f voip-server

# Database logs
docker-compose -f /root/voip-system/docker-compose.yml logs -f postgres

# All logs
docker-compose -f /root/voip-system/docker-compose.yml logs -f
```

### Log Rotation

Docker handles log rotation automatically, but you can configure limits:

```bash
# Edit docker daemon config
nano /etc/docker/daemon.json
```

Add:
```json
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  }
}
```

Restart Docker:
```bash
systemctl restart docker
docker-compose -f /root/voip-system/docker-compose.yml up -d
```

---

## Part 14: Updating the Server

### Pull Latest Code

```bash
cd /root/voip-system

# If using git
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

# Rebuild everything
docker-compose build

# Restart
docker-compose up -d
```

---

## Troubleshooting VPS Deployment

### Can't connect via SSH

**Check**:
- IP address correct?
- Internet connection working?
- Firewall on local machine?

**Try**:
```powershell
ping YOUR_VPS_IP
# Should get replies
```

### Firewall blocks everything

**Reset firewall**:
```bash
# Disable UFW
ufw disable

# Re-configure
ufw allow 22
ufw allow 9000/tcp
ufw allow 9001/udp
ufw enable
```

### Container won't start

**Check logs**:
```bash
docker-compose logs voip-server

# Look for errors like:
# - "Database connection failed" → Postgres not ready
# - "Port already in use" → Something else using port
# - "Permission denied" → File permissions issue
```

### Can't build Docker image

**If build fails**:
```bash
# Clean Docker cache
docker system prune -a

# Rebuild with no cache
docker-compose build --no-cache
```

### Out of disk space

**Check disk usage**:
```bash
df -h

# Clean Docker
docker system prune -a -f

# Remove old backups
rm -f /root/voip-backups/voip_*.sql
```

---

## Security Checklist

- [ ] SSH key authentication enabled (no password)
- [ ] UFW firewall enabled and configured
- [ ] Strong passwords in .env file
- [ ] TLS/SSL certificates installed (production)
- [ ] Regular backups scheduled
- [ ] Log monitoring set up
- [ ] Auto-updates enabled
- [ ] Non-root user for SSH (optional but recommended)
- [ ] Fail2ban installed (optional, blocks brute force)

---

## Performance Tuning

### For 100+ Concurrent Users

**Increase PostgreSQL connections**:

Edit `docker-compose.yml`:
```yaml
environment:
  VOIP__DATABASE__MAX_CONNECTIONS: 50
  VOIP__SERVER__MAX_CONNECTIONS: 2000
```

**Restart**:
```bash
docker-compose restart voip-server
```

### For Low Latency

**Choose VPS region** closest to most users.

**Use SSD storage** (all our recommendations use SSD).

**Enable BBR** (TCP congestion control):
```bash
echo "net.core.default_qdisc=fq" >> /etc/sysctl.conf
echo "net.ipv4.tcp_congestion_control=bbr" >> /etc/sysctl.conf
sysctl -p
```

---

## Cost Summary

### DigitalOcean

- **First 60 days**: FREE ($200 credit)
- **After**: $6/month
- **Annual**: $72/year

### Hetzner

- **From day 1**: €4.15/month (~$4.50)
- **Annual**: ~$54/year

### Bandwidth

All providers include enough bandwidth for VoIP:
- DigitalOcean: 1 TB/month
- Hetzner: 20 TB/month
- VoIP uses ~30 GB/month for 10 concurrent users

---

## Next Steps

1. ✅ VPS created and Docker installed
2. ✅ VoIP server deployed and running
3. ✅ Firewall configured
4. ✅ External access tested
5. **→ Update clients** to use VPS IP/domain
6. **→ Test voice quality** from different networks
7. **→ Set up monitoring** and alerts
8. **→ Schedule backups**
9. **→ Configure TLS** for production

---

## Quick Command Reference

```bash
# Start services
docker-compose -f /root/voip-system/docker-compose.yml up -d

# Stop services
docker-compose -f /root/voip-system/docker-compose.yml down

# View logs
docker-compose -f /root/voip-system/docker-compose.yml logs -f voip-server

# Restart server
docker-compose -f /root/voip-system/docker-compose.yml restart voip-server

# Check status
docker-compose -f /root/voip-system/docker-compose.yml ps

# Backup database
docker-compose -f /root/voip-system/docker-compose.yml exec postgres pg_dump -U voip voip > backup.sql

# Update server
cd /root/voip-system && git pull && docker-compose build voip-server && docker-compose up -d
```

---

## Support

**Issues?** Check:
1. [TROUBLESHOOTING.md](./TROUBLESHOOTING.md)
2. Docker logs: `docker-compose logs`
3. Firewall: `ufw status`
4. Network: `netstat -tuln`

Your VoIP server is now live and accessible from anywhere! 🎉
