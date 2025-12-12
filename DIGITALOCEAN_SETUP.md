# DigitalOcean Setup Guide - Simple & Fast

Get your VoIP server online in **30 minutes** with the easiest cloud hosting option.

**Cost**: $6/month (billed hourly, ~$0.009/hour)
**Specs**: 1 CPU, 1GB RAM, 25GB SSD, 1TB bandwidth

---

## Step 1: Create DigitalOcean Account (5 minutes)

### 1.1 Sign Up

1. **Go to**: https://www.digitalocean.com/
2. **Click**: "Sign Up"
3. **Enter**:
   - Email address
   - Password
4. **Click**: "Create Account"

### 1.2 Verify Email

1. Check your email inbox
2. Click verification link
3. You'll be redirected back to DigitalOcean

### 1.3 Add Payment Method

1. **Enter credit/debit card** information
2. **Or use PayPal** if you prefer
3. No charge yet - billed hourly after you create resources

**That's it!** You're ready to create your server.

---

## Step 2: Create Your Server (Droplet) (5 minutes)

### 2.1 Start Creating Droplet

1. **Click** green "Create" button (top right)
2. **Select**: "Droplets"

### 2.2 Choose Image

**Under "Choose an image"**:
- **Select**: "Ubuntu"
- **Version**: 22.04 LTS x64

### 2.3 Choose Plan

**Droplet Type**:
- **Select**: "Basic" (already selected)

**CPU options**:
- **Select**: "Regular" (already selected)

**Choose a plan**:
- **Select**: "$6/month" option
  - 1 GB RAM
  - 1 CPU
  - 25 GB SSD
  - 1000 GB transfer

### 2.4 Choose Datacenter Region

**Select closest to your users**:
- **US**: New York (NYC1) or San Francisco (SFO3)
- **Europe**: Frankfurt (FRA1) or London (LON1)
- **Asia**: Singapore (SGP1) or Bangalore (BLR1)

### 2.5 Authentication (SSH Keys or Password)

**Choose one**:

**Option A: Password** (easier, less secure):
1. **Select**: "Password"
2. **Enter** a strong password (save it somewhere!)
3. You'll use this to login

**Option B: SSH Key** (recommended, more secure):
1. **Select**: "SSH Key"
2. **Click**: "New SSH Key"

**Generate SSH key on Windows**:
```powershell
# Open PowerShell
ssh-keygen -t ed25519 -C "your-email@example.com"

# Save to: C:\Users\YourName\.ssh\digitalocean
# Press Enter (no passphrase, or add one for extra security)

# Copy the public key
Get-Content C:\Users\YourName\.ssh\digitalocean.pub | Set-Clipboard
```

3. **Paste** the public key into DigitalOcean
4. **Name it**: "My Windows PC"
5. **Click**: "Add SSH Key"

### 2.6 Finalize Details

**Choose a hostname**:
- Name: `voip-server` (or any name you like)

**Add tags** (optional):
- Skip this

**Project** (optional):
- Leave as default

### 2.7 Create Droplet

1. **Click**: "Create Droplet" (green button at bottom)
2. **Wait 30-60 seconds** while it provisions
3. ✅ **Your server is ready!**

### 2.8 Note Your Droplet IP

You'll see your droplet listed with:
- **IP Address**: e.g., `164.90.xxx.xxx`
- **Copy this IP** - you'll need it!

---

## Step 3: Configure Firewall (5 minutes)

### 3.1 Create Cloud Firewall

1. **Click** your droplet name
2. **Left sidebar**: Click "Networking"
3. **Click**: "Firewalls" tab
4. **Click**: "Create Firewall"

### 3.2 Name Your Firewall

**Name**: `voip-firewall`

### 3.3 Inbound Rules

**Default SSH rule** (already there):
- Keep it: TCP, Port 22, All IPv4, All IPv6

**Add VoIP Control Port**:
1. **Click**: "New rule"
2. **Type**: Custom
3. **Protocol**: TCP
4. **Port Range**: 9000
5. **Sources**: All IPv4 ✓, All IPv6 ✓

**Add VoIP Voice Port**:
1. **Click**: "New rule"
2. **Type**: Custom
3. **Protocol**: UDP
4. **Port Range**: 9001
5. **Sources**: All IPv4 ✓, All IPv6 ✓

**Optional - HTTPS** (for future SSL):
1. **Click**: "New rule"
2. **Type**: HTTPS
3. **Protocol**: TCP
4. **Port Range**: 443
5. **Sources**: All IPv4 ✓, All IPv6 ✓

### 3.4 Outbound Rules

**Keep defaults**:
- All TCP, All UDP, All ICMP

### 3.5 Apply to Droplet

**Apply to Droplets**:
1. **Search** for: `voip-server` (your droplet name)
2. **Select** your droplet
3. **Click**: "Create Firewall"

✅ **Firewall configured!**

---

## Step 4: Connect to Your Server (2 minutes)

### 4.1 SSH Connection

**Open PowerShell** on your Windows machine:

**If you used Password**:
```powershell
ssh root@YOUR_DROPLET_IP

# Enter the password you set earlier
```

**If you used SSH Key**:
```powershell
ssh -i C:\Users\YourName\.ssh\digitalocean root@YOUR_DROPLET_IP
```

**First time connecting**:
- You'll see: "The authenticity of host... can't be established"
- **Type**: `yes` and press Enter

You should see:
```
root@voip-server:~#
```

✅ **You're connected!**

---

## Step 5: Update System (2 minutes)

```bash
# Update package lists
apt update

# Upgrade packages (say 'yes' if asked)
apt upgrade -y

# This takes 1-2 minutes
```

---

## Step 6: Install Dependencies (5 minutes)

### 6.1 Install Rust

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# When prompted, press 1 (default installation)
# Then press Enter

# Load Rust into current shell
source $HOME/.cargo/env

# Verify
rustc --version
cargo --version
```

### 6.2 Install PostgreSQL

```bash
# Install PostgreSQL
apt install postgresql postgresql-contrib -y

# Start PostgreSQL
systemctl start postgresql
systemctl enable postgresql
```

### 6.3 Install Build Tools

```bash
# Install required build dependencies
apt install -y build-essential pkg-config libssl-dev git
```

---

## Step 7: Setup Database (3 minutes)

```bash
# Switch to postgres user
sudo -u postgres psql

# You're now in PostgreSQL shell (postgres=#)
```

**Run these SQL commands**:
```sql
-- Create database
CREATE DATABASE voip_db;

-- Create user with strong password (CHANGE THIS!)
CREATE USER voip_user WITH PASSWORD 'your_super_secure_password_here_123';

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE voip_db TO voip_user;

-- Exit PostgreSQL
\q
```

---

## Step 8: Upload Server Code (5 minutes)

### Option A: Upload from Windows (Recommended)

**On your Windows machine** (new PowerShell window):

```powershell
# Navigate to your server directory
cd C:\dev\VoIP-System

# Upload server code to droplet
scp -r server root@YOUR_DROPLET_IP:/root/

# If using SSH key:
scp -i C:\Users\YourName\.ssh\digitalocean -r server root@YOUR_DROPLET_IP:/root/
```

This uploads your entire server folder to the droplet.

### Option B: Clone from Git

If your code is on GitHub/GitLab:

```bash
# On the droplet
cd /root
git clone https://github.com/yourusername/VoIP-System.git
cd VoIP-System/server
```

---

## Step 9: Configure Server (3 minutes)

**On the droplet**:

```bash
# Navigate to server directory
cd /root/server

# Edit configuration
nano config/server.toml
```

**Update these settings** (use arrow keys to navigate):

```toml
[server]
bind_address = "0.0.0.0"  # Listen on all interfaces
control_port = 9000
voice_port = 9001

[database]
host = "localhost"
port = 5432
database = "voip_db"
username = "voip_user"
password = "your_super_secure_password_here_123"  # MATCH what you set in Step 7

[security]
jwt_secret = "change-this-to-a-long-random-string-at-least-32-characters-long"
tls_cert = "/root/server/certs/server.crt"
tls_key = "/root/server/certs/server.key"
```

**Save and exit**:
- Press `Ctrl+O` (write out)
- Press `Enter` (confirm filename)
- Press `Ctrl+X` (exit)

---

## Step 10: Generate SSL Certificate (2 minutes)

```bash
# Create certs directory
mkdir -p /root/server/certs
cd /root/server/certs

# Generate self-signed certificate (valid 1 year)
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout server.key \
  -out server.crt \
  -days 365 \
  -subj "/CN=YOUR_DROPLET_IP"

# Replace YOUR_DROPLET_IP with actual IP, e.g.:
# -subj "/CN=164.90.xxx.xxx"

# Set permissions
chmod 600 server.key
chmod 644 server.crt
```

---

## Step 11: Build Server (5 minutes)

```bash
# Navigate to server directory
cd /root/server

# Build server (this takes 3-5 minutes)
cargo build --release
```

You'll see lots of compilation messages. Wait for it to complete.

---

## Step 12: Test Server (1 minute)

```bash
# Run server
cargo run --release
```

You should see:
```
✅ Database ready
✅ Database tables created/verified
🎧 Voice server listening on 0.0.0.0:9001
🌐 Control server listening on 0.0.0.0:9000
```

**Press Ctrl+C** to stop the server.

✅ **Server works!**

---

## Step 13: Create Auto-Start Service (3 minutes)

### 13.1 Create Service File

```bash
nano /etc/systemd/system/voip-server.service
```

**Paste this configuration**:

```ini
[Unit]
Description=VoIP Server - Encrypted Voice Communication
After=network.target postgresql.service
Requires=postgresql.service

[Service]
Type=simple
User=root
WorkingDirectory=/root/server
ExecStart=/root/server/target/release/voip_server
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

**Save and exit**: `Ctrl+O`, `Enter`, `Ctrl+X`

### 13.2 Enable and Start Service

```bash
# Reload systemd
systemctl daemon-reload

# Enable service (start on boot)
systemctl enable voip-server

# Start service now
systemctl start voip-server

# Check status
systemctl status voip-server
```

You should see:
```
● voip-server.service - VoIP Server
   Active: active (running)
```

**Press 'q'** to exit status view.

---

## Step 14: Verify It's Working (2 minutes)

### 14.1 Check Server Logs

```bash
# View real-time logs
journalctl -u voip-server -f

# You should see:
# ✅ Database ready
# 🎧 Voice server listening on 0.0.0.0:9001
# 🌐 Control server listening on 0.0.0.0:9000
```

**Press Ctrl+C** to exit.

### 14.2 Check Ports are Listening

```bash
# Check port 9000
ss -tuln | grep 9000

# Should show: tcp LISTEN 0.0.0.0:9000

# Check port 9001
ss -tuln | grep 9001

# Should show: udp 0.0.0.0:9001
```

✅ **Server is running and listening!**

---

## Step 15: Test from Your Client (2 minutes)

**On your Windows machine**:

1. **Run**: `voip-client.exe`
2. **Server**: `YOUR_DROPLET_IP` (e.g., 164.90.xxx.xxx)
3. **Port**: `9000`
4. **Use TLS**: ✓ (checked)
5. **Username**: Create new account
6. **Click**: Register

You should see:
```
🔑 SRTP key exchange complete
✅ Connected to server
```

### Test Voice

1. Create or join a channel
2. Press Space (push-to-talk)
3. Speak into microphone
4. You should see voice packets being sent

✅ **IT WORKS!**

---

## Useful Commands

### View Logs
```bash
# Real-time logs
journalctl -u voip-server -f

# Last 100 lines
journalctl -u voip-server -n 100

# Today's logs
journalctl -u voip-server --since today
```

### Control Server
```bash
# Stop server
systemctl stop voip-server

# Start server
systemctl start voip-server

# Restart server
systemctl restart voip-server

# Check status
systemctl status voip-server
```

### Check Server Resources
```bash
# CPU and memory usage
htop

# Disk space
df -h

# Active connections
ss -tuln | grep -E '9000|9001'
```

### Update Server Code
```bash
# Stop server
systemctl stop voip-server

# Upload new code from Windows
# (On Windows PowerShell)
# scp -r server root@YOUR_DROPLET_IP:/root/

# Rebuild on droplet
cd /root/server
cargo build --release

# Start server
systemctl start voip-server
```

---

## Optional Enhancements

### Add Domain Name

Instead of IP address like `164.90.xxx.xxx`, use a friendly domain:

1. **Get a domain**:
   - Buy one: Namecheap, Google Domains (~$10/year)
   - Free: DuckDNS.org, FreeDNS

2. **Add DNS A Record**:
   - Name: `voice.yourdomain.com` (or just `@` for root domain)
   - Type: A
   - Value: YOUR_DROPLET_IP
   - TTL: 3600

3. **Wait 5-10 minutes** for DNS propagation

4. **Test**:
   ```powershell
   nslookup voice.yourdomain.com
   ```

5. **Users connect to**: `voice.yourdomain.com:9000`

### Get Real SSL Certificate (Let's Encrypt)

Replace self-signed certificate with trusted certificate:

```bash
# Stop server temporarily
systemctl stop voip-server

# Install certbot
apt install certbot -y

# Get certificate (replace with your domain)
certbot certonly --standalone -d voice.yourdomain.com

# Follow prompts (enter email, agree to terms)

# Certificates saved to:
# /etc/letsencrypt/live/voice.yourdomain.com/fullchain.pem
# /etc/letsencrypt/live/voice.yourdomain.com/privkey.pem

# Update server.toml
nano /root/server/config/server.toml

# Change:
# tls_cert = "/etc/letsencrypt/live/voice.yourdomain.com/fullchain.pem"
# tls_key = "/etc/letsencrypt/live/voice.yourdomain.com/privkey.pem"

# Start server
systemctl start voip-server
```

Certificates auto-renew via certbot timer.

---

## Troubleshooting

### Can't SSH to server
- **Check**: Did you copy the correct IP?
- **Check**: Is firewall port 22 open? (should be by default)
- **Check**: Using correct username? (should be `root`)

### Can't connect from client
- **Check**: Server is running: `systemctl status voip-server`
- **Check**: Ports are listening: `ss -tuln | grep -E '9000|9001'`
- **Check**: DigitalOcean firewall allows ports 9000 and 9001
- **Check**: Server logs: `journalctl -u voip-server -n 50`

### Database connection failed
- **Check**: PostgreSQL running: `systemctl status postgresql`
- **Check**: Password in server.toml matches database user password
- **Check**: Database and user exist: `sudo -u postgres psql -c '\l'`

### Server won't start
- **Check logs**: `journalctl -u voip-server -n 100`
- **Check config**: `nano /root/server/config/server.toml`
- **Test manually**: `cd /root/server && cargo run --release`

---

## Cost Breakdown

**DigitalOcean Droplet**: $6/month
- Billed hourly: $0.00893/hour
- 1 GB RAM / 1 CPU / 25 GB SSD / 1 TB transfer

**Supabase Database** (if using instead of local PostgreSQL): $0/month
- Free tier: 500 MB database, 2 GB bandwidth

**Domain** (optional): $10/year (~$0.83/month)

**SSL Certificate** (Let's Encrypt): FREE

**Total**: $6/month (or $6.83/month with domain)

---

## Next Steps

1. ✅ Server is online 24/7
2. ✅ Auto-restarts if it crashes
3. ✅ Auto-starts on server reboot

**Share with users**:
```
🎧 Connect to my VoIP server:

Server: YOUR_DROPLET_IP:9000
Use TLS: ✓ (enabled)

Download client: [your distribution link]

Create an account and start talking!
```

**Monitor your server**:
- Check logs occasionally: `journalctl -u voip-server -n 100`
- Monitor resources: `htop`
- Check active connections: `ss -tuln | grep -E '9000|9001'`

---

## Summary

**What you now have**:
- ✅ VoIP server running on DigitalOcean
- ✅ PostgreSQL database
- ✅ Firewall configured
- ✅ Auto-start on boot
- ✅ SSL/TLS encryption
- ✅ Public IP accessible from anywhere

**Total setup time**: ~30 minutes
**Monthly cost**: $6
**Difficulty**: ⭐⭐ Easy

**Much simpler than Oracle Cloud!** 🎉

---

**Need help?** Check:
- Server logs: `journalctl -u voip-server -f`
- [TROUBLESHOOTING.md](./TROUBLESHOOTING.md) - Client connection issues
- [HOSTING_OPTIONS.md](./HOSTING_OPTIONS.md) - Alternative providers
