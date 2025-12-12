# Oracle Cloud Free Tier - VoIP Server Deployment Guide

Complete step-by-step guide to deploy your VoIP server on Oracle Cloud's **Always Free** tier.

---

## Why Oracle Cloud Free Tier?

Oracle Cloud offers the **best free tier** for hosting a VoIP server:

- **4 ARM CPU cores** (Ampere A1)
- **24 GB RAM**
- **200 GB storage**
- **Forever free** (no expiration, no credit card charges)
- **10 TB outbound bandwidth/month**
- **Full root access**
- **Public IPv4 address included**

Perfect for running a production VoIP server 24/7 with no cost.

---

## Part 1: Create Oracle Cloud Account

### Step 1: Sign Up

1. **Go to**: https://signup.oraclecloud.com/
2. **Click**: "Start for free"
3. **Fill in**:
   - Country
   - Name
   - Email address (use a real email - verification required)
   - Click "Verify my email"

### Step 2: Email Verification

1. Check your email inbox
2. Find email from Oracle Cloud (noreply@oracle.com)
3. **Important**: Verification link expires in 30 minutes
4. Click the verification link

### Step 3: Account Details

1. **Cloud Account Name**: Choose a unique name (e.g., `myvoipserver`)
   - This becomes your subdomain: `myvoipserver.oraclecloud.com`
   - **Cannot be changed later**

2. **Home Region**: Choose closest to your users
   - **Important**: This is PERMANENT - cannot change later
   - Recommendations:
     - US East (Ashburn) - Best for US/Americas
     - EU West (Frankfurt) - Best for Europe
     - Asia Pacific (Tokyo/Seoul) - Best for Asia

3. **Click**: Continue

### Step 4: Identity Verification

1. **Enter personal information**:
   - Address
   - Phone number
   - Date of birth

2. **Payment verification** (no charge will be made):
   - **Credit card required** (debit/prepaid cards may not work)
   - Oracle verifies identity only - no charges
   - You get $300 trial credit (30 days) + Always Free resources
   - After trial expires, Always Free resources continue forever

3. **Agree to terms** and click "Start my free trial"

### Step 5: Account Activation

1. Wait 2-5 minutes for account provisioning
2. You'll receive email: "Get Started Now with Oracle Cloud"
3. Click link in email or go to: https://cloud.oracle.com/
4. **Sign in** with your credentials

---

## Part 2: Create a Free Tier VM Instance

### Step 1: Navigate to Compute

1. Sign in to Oracle Cloud Console
2. Click hamburger menu (☰) top-left
3. **Navigate**: Compute → Instances
4. **Click**: "Create Instance"

### Step 2: Configure Instance Basics

**Name**: `voip-server` (or any name you like)

**Placement**:
- Leave defaults (Availability Domain will be auto-selected)

### Step 3: Choose Image and Shape

**Image**:
1. **Click**: "Change Image"
2. **Select**: Ubuntu 22.04
   - Canonical Ubuntu 22.04 Minimal (recommended)
   - Or Oracle Linux 8 if you prefer
3. **Click**: "Select Image"

**Shape** (This is critical for Free Tier):
1. **Click**: "Change Shape"
2. **Select**: "Ampere" (ARM-based)
3. **Choose**: `VM.Standard.A1.Flex`
4. **Configure resources**:
   - **Number of OCPUs**: 2 (can use up to 4 total across all VMs)
   - **Amount of memory (GB)**: 12 (can use up to 24 total)
   - These are **Always Free** - no charges ever
5. **Click**: "Select Shape"

### Step 4: Configure Networking

**Virtual Cloud Network**:
- Leave default (auto-creates VCN if first time)

**Subnet**:
- Leave default (public subnet)

**Public IP address**:
- **Select**: "Assign a public IPv4 address" ✓
- This is your server's address

### Step 5: SSH Keys

You need SSH keys to access the server:

**Windows users**:
```cmd
# Generate SSH key (if you don't have one)
ssh-keygen -t rsa -b 4096 -f C:\Users\YourName\.ssh\oracle_cloud_key

# This creates:
# - oracle_cloud_key (private key - keep secure!)
# - oracle_cloud_key.pub (public key - upload to Oracle)
```

**In Oracle Cloud Console**:
1. **SSH keys** section
2. **Select**: "Paste public keys"
3. **Open**: `C:\Users\YourName\.ssh\oracle_cloud_key.pub` in Notepad
4. **Copy entire contents** (starts with `ssh-rsa`)
5. **Paste** into Oracle Cloud text box

### Step 6: Boot Volume

**Boot volume size**: 50 GB (Free tier includes 200 GB total)

Leave other settings as default.

### Step 7: Create Instance

1. **Click**: "Create" (bottom of page)
2. Wait 1-2 minutes for provisioning
3. **Status** will change: Provisioning → Running (orange → green)

### Step 8: Note Your Instance Details

Once running, note these details:

1. **Public IP Address**: (e.g., 158.101.xxx.xxx)
   - This is your server address
   - Users will connect to: `wss://158.101.xxx.xxx:9000`

2. **Username**: `ubuntu` (for Ubuntu) or `opc` (for Oracle Linux)

---

## Part 3: Configure Oracle Cloud Firewall

Oracle Cloud has **two firewalls**:
1. **Cloud Security List** (Oracle's network firewall)
2. **OS Firewall** (Ubuntu's firewall)

Both must be configured.

### Step 1: Configure Cloud Security List

1. **From Instance Details page**, scroll down to "Primary VNIC"
2. **Click** the Subnet link (e.g., "subnet-xxxxxxxxx")
3. **Click** "Default Security List for vcn-xxxxxxxxx"
4. **Click** "Add Ingress Rules"

**Rule 1 - WebSocket Control (TCP 9000)**:
- **Source CIDR**: `0.0.0.0/0` (allow from anywhere)
- **IP Protocol**: TCP
- **Source Port Range**: Leave blank
- **Destination Port Range**: `9000`
- **Description**: VoIP WebSocket Control
- **Click**: "Add Ingress Rules"

**Rule 2 - Voice UDP (UDP 9001)**:
1. **Click** "Add Ingress Rules" again
2. **Source CIDR**: `0.0.0.0/0`
3. **IP Protocol**: UDP
4. **Source Port Range**: Leave blank
5. **Destination Port Range**: `9001`
6. **Description**: VoIP Voice UDP
7. **Click**: "Add Ingress Rules"

**Rule 3 - HTTPS (TCP 443) - Optional but recommended**:
1. **Click** "Add Ingress Rules" again
2. **Source CIDR**: `0.0.0.0/0`
3. **IP Protocol**: TCP
4. **Destination Port Range**: `443`
5. **Description**: HTTPS
6. **Click**: "Add Ingress Rules"

---

## Part 4: Connect to Your Instance

### Step 1: SSH Connection

**Windows (using PowerShell)**:
```powershell
# Connect to instance
ssh -i C:\Users\YourName\.ssh\oracle_cloud_key ubuntu@YOUR_PUBLIC_IP

# Example:
ssh -i C:\Users\YourName\.ssh\oracle_cloud_key ubuntu@158.101.xxx.xxx
```

First time connecting:
- You'll see: "The authenticity of host... can't be established"
- Type: `yes` and press Enter

You should now see:
```
ubuntu@voip-server:~$
```

### Step 2: Update System

```bash
# Update package lists
sudo apt update

# Upgrade packages
sudo apt upgrade -y

# Reboot if kernel was updated
sudo reboot
```

Wait 1 minute, then reconnect with SSH.

---

## Part 5: Configure OS Firewall

Ubuntu uses `iptables` by default. We need to open ports.

```bash
# Allow SSH (so we don't lock ourselves out)
sudo iptables -I INPUT 1 -p tcp --dport 22 -j ACCEPT

# Allow VoIP WebSocket Control (TCP 9000)
sudo iptables -I INPUT 2 -p tcp --dport 9000 -j ACCEPT

# Allow VoIP Voice (UDP 9001)
sudo iptables -I INPUT 3 -p udp --dport 9001 -j ACCEPT

# Allow HTTPS (optional, for reverse proxy later)
sudo iptables -I INPUT 4 -p tcp --dport 443 -j ACCEPT

# Save rules
sudo apt install iptables-persistent -y
# When prompted, select "Yes" to save current rules
```

**Verify rules**:
```bash
sudo iptables -L INPUT -n --line-numbers
```

You should see ports 22, 9000, 9001, 443 allowed.

---

## Part 6: Install Dependencies

### Step 1: Install Rust

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Select option 1 (default installation)
# Press Enter

# Load Rust environment
source $HOME/.cargo/env

# Verify installation
rustc --version
cargo --version
```

### Step 2: Install PostgreSQL

```bash
# Install PostgreSQL
sudo apt install postgresql postgresql-contrib -y

# Start PostgreSQL
sudo systemctl start postgresql
sudo systemctl enable postgresql

# Verify it's running
sudo systemctl status postgresql
```

### Step 3: Install Build Tools

```bash
# Install required build dependencies
sudo apt install -y build-essential pkg-config libssl-dev git
```

---

## Part 7: Setup Database

### Step 1: Create Database and User

```bash
# Switch to postgres user
sudo -u postgres psql

# You're now in PostgreSQL shell (postgres=#)
```

**In PostgreSQL shell**:
```sql
-- Create database
CREATE DATABASE voip_db;

-- Create user with password
CREATE USER voip_user WITH PASSWORD 'your_secure_password_here';

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE voip_db TO voip_user;

-- Exit PostgreSQL
\q
```

### Step 2: Configure PostgreSQL to Accept Connections

```bash
# Edit PostgreSQL config
sudo nano /etc/postgresql/14/main/postgresql.conf

# Find line: #listen_addresses = 'localhost'
# Change to: listen_addresses = 'localhost'
# (Uncomment it if commented)

# Save: Ctrl+O, Enter
# Exit: Ctrl+X
```

```bash
# Edit access control
sudo nano /etc/postgresql/14/main/pg_hba.conf

# Add this line at the end:
# local   voip_db    voip_user    md5

# Save and exit
```

```bash
# Restart PostgreSQL
sudo systemctl restart postgresql
```

### Step 3: Test Database Connection

```bash
# Test connection
psql -h localhost -U voip_user -d voip_db

# Enter password when prompted
# You should see: voip_db=>

# Exit
\q
```

---

## Part 8: Deploy Server Code

### Option A: Clone from Git (Recommended)

If you have your code on GitHub/GitLab:

```bash
# Create directory for server
mkdir -p /home/ubuntu/voip-server
cd /home/ubuntu/voip-server

# Clone your repository
git clone https://github.com/yourusername/VoIP-System.git .

# Or if not using git, proceed to Option B
```

### Option B: Upload Server Code Manually

**From your Windows machine**:

```powershell
# Navigate to your server directory
cd C:\dev\VoIP-System\server

# Upload server code using SCP
scp -i C:\Users\YourName\.ssh\oracle_cloud_key -r . ubuntu@YOUR_PUBLIC_IP:/home/ubuntu/voip-server/
```

This uploads all files to the instance.

---

## Part 9: Configure Server

### Step 1: Edit Configuration File

```bash
# Navigate to server directory
cd /home/ubuntu/voip-server/server

# Edit configuration
nano config/server.toml
```

**Update these settings**:
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
password = "your_secure_password_here"  # Match what you set in PostgreSQL

[security]
jwt_secret = "generate-a-long-random-string-here-at-least-32-characters"
tls_cert = "/home/ubuntu/voip-server/server/certs/server.crt"
tls_key = "/home/ubuntu/voip-server/server/certs/server.key"
```

**Save**: Ctrl+O, Enter
**Exit**: Ctrl+X

### Step 2: Generate TLS Certificates

For now, we'll use self-signed certificates (we'll add Let's Encrypt later):

```bash
# Create certs directory
mkdir -p /home/ubuntu/voip-server/server/certs
cd /home/ubuntu/voip-server/server/certs

# Generate self-signed certificate (valid 365 days)
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout server.key \
  -out server.crt \
  -days 365 \
  -subj "/CN=YOUR_PUBLIC_IP"

# Example:
# -subj "/CN=158.101.xxx.xxx"

# Set permissions
chmod 600 server.key
chmod 644 server.crt
```

---

## Part 10: Build and Run Server

### Step 1: Initial Database Setup

The server will create tables on first run, but let's compile it first:

```bash
# Navigate to server directory
cd /home/ubuntu/voip-server/server

# Build server (this takes 5-10 minutes on ARM)
cargo build --release
```

This compiles the Rust server for ARM architecture.

### Step 2: Initialize Database

```bash
# Run server once to initialize database
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

### Step 3: Test Server is Accessible

**From your Windows machine**:

```powershell
# Test if port is open
Test-NetConnection -ComputerName YOUR_PUBLIC_IP -Port 9000
```

You should see:
```
TcpTestSucceeded : True
```

If it shows `False`, double-check:
1. Oracle Cloud Security List has port 9000 open
2. OS firewall (iptables) has port 9000 open
3. Server is running

---

## Part 11: Create Systemd Service (Auto-Start)

To keep server running 24/7 and auto-restart on reboot:

### Step 1: Create Service File

```bash
sudo nano /etc/systemd/system/voip-server.service
```

**Paste this configuration**:
```ini
[Unit]
Description=VoIP Server - Encrypted Voice Communication
After=network.target postgresql.service
Requires=postgresql.service

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/home/ubuntu/voip-server/server
ExecStart=/home/ubuntu/voip-server/server/target/release/voip_server
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/home/ubuntu/voip-server

[Install]
WantedBy=multi-user.target
```

**Save**: Ctrl+O, Enter
**Exit**: Ctrl+X

### Step 2: Enable and Start Service

```bash
# Reload systemd to recognize new service
sudo systemctl daemon-reload

# Enable service (start on boot)
sudo systemctl enable voip-server

# Start service
sudo systemctl start voip-server

# Check status
sudo systemctl status voip-server
```

You should see:
```
● voip-server.service - VoIP Server - Encrypted Voice Communication
     Loaded: loaded
     Active: active (running)
```

### Step 3: View Logs

```bash
# View real-time logs
sudo journalctl -u voip-server -f

# View recent logs
sudo journalctl -u voip-server -n 100

# Press Ctrl+C to exit
```

---

## Part 12: Test Connection from Client

### Step 1: Update Client Configuration

On your **Windows machine**, test connecting:

1. **Run**: `voip-client.exe`
2. **Server**: `YOUR_PUBLIC_IP` (e.g., 158.101.xxx.xxx)
3. **Port**: `9000`
4. **Use TLS**: ✓ (checked)
5. **Username**: Create a new account
6. **Click**: Login/Register

You should see:
```
🔑 SRTP key exchange complete
✅ Connected to server
```

### Step 2: Test Voice Transmission

1. Join a channel
2. Press push-to-talk
3. Speak into microphone
4. You should see voice packets being sent

**Connect from another computer** (friend, different network) to test multi-user voice chat.

---

## Part 13: Production Enhancements (Optional)

### A. Add Domain Name

Instead of using IP address, use a friendly domain:

1. **Register domain** (free options):
   - Freenom.com (free domains)
   - DuckDNS.org (free subdomain)
   - Or buy domain ($10/year)

2. **Create DNS A record**:
   - Name: `voice.yourdomain.com`
   - Type: A
   - Value: YOUR_PUBLIC_IP
   - TTL: 3600

3. **Wait 5-10 minutes** for DNS propagation

4. **Test**:
   ```powershell
   nslookup voice.yourdomain.com
   ```

Now users can connect to: `wss://voice.yourdomain.com:9000`

### B. Get Real SSL Certificate (Let's Encrypt)

Replace self-signed certificate with free trusted certificate:

```bash
# Install Certbot
sudo apt install certbot -y

# Stop VoIP server temporarily
sudo systemctl stop voip-server

# Get certificate (replace with your domain)
sudo certbot certonly --standalone -d voice.yourdomain.com

# Follow prompts (enter email, agree to terms)
```

Certificates will be in:
- `/etc/letsencrypt/live/voice.yourdomain.com/fullchain.pem`
- `/etc/letsencrypt/live/voice.yourdomain.com/privkey.pem`

**Update server.toml**:
```bash
nano /home/ubuntu/voip-server/server/config/server.toml
```

Change:
```toml
[security]
tls_cert = "/etc/letsencrypt/live/voice.yourdomain.com/fullchain.pem"
tls_key = "/etc/letsencrypt/live/voice.yourdomain.com/privkey.pem"
```

**Fix permissions**:
```bash
# Allow server to read certificates
sudo chmod 755 /etc/letsencrypt/live
sudo chmod 755 /etc/letsencrypt/archive
```

**Restart server**:
```bash
sudo systemctl restart voip-server
```

**Auto-renew certificates** (Let's Encrypt expires every 90 days):
```bash
# Test renewal
sudo certbot renew --dry-run

# Schedule auto-renewal (already configured by certbot)
sudo systemctl status certbot.timer
```

### C. Monitoring and Maintenance

**Check server health**:
```bash
# CPU and memory usage
htop

# Disk space
df -h

# Active connections
sudo ss -tuln | grep 9000
sudo ss -tuln | grep 9001

# View logs
sudo journalctl -u voip-server -f
```

**Server commands**:
```bash
# Stop server
sudo systemctl stop voip-server

# Start server
sudo systemctl start voip-server

# Restart server
sudo systemctl restart voip-server

# Check status
sudo systemctl status voip-server
```

**Update server code**:
```bash
# Pull latest changes (if using git)
cd /home/ubuntu/voip-server
git pull

# Rebuild
cd server
cargo build --release

# Restart service
sudo systemctl restart voip-server
```

---

## Part 14: Share Your Server

Your server is now running 24/7 on Oracle Cloud!

**Give users these connection details**:
```
Server: YOUR_PUBLIC_IP (or voice.yourdomain.com)
Port: 9000
Use TLS: ✓

Download client: [Your distribution link]
```

**Example**:
```
🎧 Connect to my VoIP server:

Server: 158.101.xxx.xxx
Port: 9000
Use TLS: ✓ (enabled)

Download: https://drive.google.com/...

Create an account and join a channel!
```

---

## Troubleshooting

### Can't SSH to instance
- **Check**: Security List allows port 22
- **Check**: Used correct private key
- **Check**: Username is `ubuntu` (not `root`)

### Can't connect to server from client
- **Check**: Oracle Security List has ports 9000/9001 open
- **Check**: OS firewall (iptables) allows ports
- **Check**: Server is running: `sudo systemctl status voip-server`
- **Check**: Logs: `sudo journalctl -u voip-server -n 50`

### Server won't start
- **Check database**: `sudo systemctl status postgresql`
- **Check config**: `nano config/server.toml` (database password correct?)
- **Check logs**: `sudo journalctl -u voip-server -n 100`

### High CPU usage
- ARM CPUs are slower than x86, but 2 cores should be enough
- Consider allocating all 4 free cores if needed

### Out of memory
- Free tier gives 12-24 GB - plenty for VoIP
- Check: `free -h`
- Increase if needed (up to 24GB is still free)

---

## Cost Summary

**Total monthly cost**: $0.00

**What you get for free forever**:
- 4 ARM CPU cores (can split across 2 VMs)
- 24 GB RAM total
- 200 GB storage
- 10 TB bandwidth/month
- Public IPv4 address
- 24/7 uptime

**After $300 trial expires** (30 days):
- Always Free resources continue working
- No charges unless you manually upgrade

---

## Security Best Practices

1. **Change default passwords** - Use strong random passwords
2. **Keep system updated**: `sudo apt update && sudo apt upgrade`
3. **Monitor logs** regularly: `sudo journalctl -u voip-server`
4. **Use Let's Encrypt** for production (real SSL certificate)
5. **Backup database** regularly:
   ```bash
   pg_dump -U voip_user voip_db > backup.sql
   ```
6. **Don't share SSH private key** - keep it secure

---

## Performance Tips

1. **Use all 4 free cores** if handling many users:
   - Edit instance
   - Change shape
   - Increase to 4 OCPUs, 24GB RAM

2. **Enable swap** for extra memory safety:
   ```bash
   sudo fallocate -l 4G /swapfile
   sudo chmod 600 /swapfile
   sudo mkswap /swapfile
   sudo swapon /swapfile
   echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
   ```

3. **Monitor bandwidth** - 10TB/month is very generous
   - Voice uses ~250 KB/minute per user
   - 10TB = ~40 million minutes of voice

---

## Next Steps

1. Share server address with friends
2. Test voice quality over internet
3. Set up domain name (optional)
4. Get Let's Encrypt certificate (optional)
5. Monitor usage and performance
6. Consider implementing JWT tokens for admin API
7. Add more channels as needed

---

**Congratulations!** You now have a production VoIP server running 24/7 on Oracle Cloud's free tier.

Your users can connect from anywhere in the world with end-to-end encrypted voice communication.

**Questions or issues?** Check the troubleshooting section or refer to:
- [TROUBLESHOOTING.md](./TROUBLESHOOTING.md) - Client connection issues
- [OVERVIEW.md](./OVERVIEW.md) - System architecture
- Oracle Cloud Docs: https://docs.oracle.com/en-us/iaas/
