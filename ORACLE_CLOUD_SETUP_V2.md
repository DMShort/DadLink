# Oracle Cloud Free Tier - Complete Setup Guide (Correct Order)

This guide follows Oracle Cloud's **actual workflow** with all steps in the correct sequence.

**Based on**: Official Oracle Documentation (Updated February 2025)

---

## ⚠️ Before You Start - Important Information

### What You'll Get (Always Free - Forever)
- **4 ARM CPU cores** (Ampere A1) - can split across 2 VMs
- **24 GB RAM total** - allocate as needed
- **200 GB block storage**
- **10 TB bandwidth/month**
- **Forever free** - no expiration

### What You Need
- ✅ Valid email address
- ✅ **Credit card** (NOT debit, prepaid, or virtual cards)
  - No charges unless you manually upgrade
  - Only for identity verification
- ✅ Phone number
- ✅ 1-2 hours of time

### Critical Decisions
- **Home Region**: Choose once, **cannot change later**
  - Pick closest to your users
  - Always Free resources only work in home region

---

## Phase 1: Create Oracle Cloud Account (15-20 minutes)

### Step 1.1: Start Signup

1. **Go to**: https://signup.cloud.oracle.com/
   - Or: https://www.oracle.com/cloud/free/
2. **Click**: "Start for free" button

### Step 1.2: Enter Basic Information

**Country/Territory**: Select your country

**Name and Email**:
- First Name
- Last Name
- Email address (you'll use this to sign in)

**Complete CAPTCHA**

**Click**: "Verify my email"

### Step 1.3: Email Verification ⏰

**Important**: Verification link expires in **30 minutes**

1. **Check your email** (check spam folder if needed)
2. **Subject**: "Verify your email for Oracle Cloud"
3. **From**: noreply@oracle.com
4. **Click** the verification link

You'll be redirected back to continue signup.

### Step 1.4: Cloud Account Details

**Password**:
- 9-40 characters
- At least 1 uppercase letter
- At least 1 lowercase letter
- At least 1 number
- Save this password!

**Cloud Account Name**:
- Choose carefully (becomes part of your URL)
- Example: `myvoipserver`
- Creates: `myvoipserver.oraclecloud.com`
- **Cannot change later**

**Cloud Account Home Region**: ⚠️ **CRITICAL DECISION**
- **This is PERMANENT** - you cannot change it later
- Always Free resources only available in home region
- Choose closest to your users:

**Americas**:
- US East (Ashburn) - East Coast USA
- US West (Phoenix) - West Coast USA
- US West (San Jose) - Silicon Valley
- Canada Southeast (Montreal)
- Brazil East (Sao Paulo)

**Europe**:
- Germany Central (Frankfurt) - Most popular for EU
- UK South (London)
- Netherlands Northwest (Amsterdam)
- France Central (Paris)

**Asia Pacific**:
- Japan East (Tokyo)
- South Korea Central (Seoul)
- India West (Mumbai)
- Australia East (Sydney)

**Click**: "Continue"

### Step 1.5: Address Information

**Address**:
- Street Address
- City
- State/Province
- Postal Code
- Phone Number (mobile preferred)

**Click**: "Continue"

### Step 1.6: Payment Verification

**Important**: You will NOT be charged. This is for identity verification only.

**What happens**:
- Small temporary authorization (typically $1-2)
- Removed within 24-48 hours
- You get $300 trial credits (30 days) + Always Free forever

**Card Requirements**:
- ✅ Credit cards (Visa, Mastercard, Amex)
- ❌ Debit cards with PIN
- ❌ Virtual cards
- ❌ Prepaid cards
- ❌ Single-use cards

**Steps**:
1. **Click**: "Add payment verification method"
2. **Select**: "Credit Card"
3. **Enter**:
   - Card number
   - Expiration date
   - CVV code
   - Billing address (should match card)
4. **Click**: "Finish"

### Step 1.7: Agreement and Finalize

1. **Read** terms and conditions
2. **Check**: "I have reviewed and agree..."
3. **Click**: "Start my free trial"

### Step 1.8: Account Provisioning

**Wait 2-5 minutes** while Oracle creates your account.

You'll see: "We are preparing your Oracle Cloud Free Tier account..."

### Step 1.9: Welcome Email

**Check email**: "Get Started Now with Oracle Cloud"
- Contains your sign-in information
- Cloud Account Name (tenancy name)
- Username
- Sign-in URL

**Click**: "Sign in to Oracle Cloud" button in email

---

## Phase 2: First Login and Dashboard Setup (5 minutes)

### Step 2.1: Sign In to Console

You'll be redirected to: https://cloud.oracle.com/

**If not, go there manually**:

**Cloud Account Name**: Enter your cloud account name (from email)
**Click**: "Next"

**Username**: Your email address (or username from welcome email)
**Password**: Password you set during signup

**Click**: "Sign In"

### Step 2.2: First-Time Console Tour (Optional)

**You'll see**:
- "Welcome to Oracle Cloud Infrastructure"
- Option for guided tour (takes 1 minute)

**Options**:
- Take the tour (recommended for first time)
- Skip: "Not right now"

### Step 2.3: Understand the Dashboard

**You'll see the OCI Console with**:

**Top bar**:
- ☰ Hamburger menu (left) - Access all services
- Region selector (right) - Shows your home region
- Profile icon (right) - Account settings

**Main area**:
- "Get Started" tab - Quick start guides
- "Dashboard" tab - Resource overview

**Bottom left**:
- "Launch Cloud Shell" - Browser-based terminal

### Step 2.4: Verify Home Region

**Check top-right corner**: Should show your home region
- Example: "US East (Ashburn)"
- **Important**: Always Free resources only work here

### Step 2.5: Understand Compartments

**What are compartments?**
- Logical containers for organizing resources
- Like folders for your cloud resources
- Every account has a **root compartment** (your tenancy)

**For this guide**:
- We'll use the **root compartment** (default)
- It's already created and selected

**To verify**:
1. **Click** ☰ Hamburger menu
2. **Navigate**: Identity & Security → Compartments
3. **You'll see**: Your tenancy name (root compartment)

✅ **You're ready to create resources!**

---

## Phase 3: Create Virtual Cloud Network (VCN) (10 minutes)

### Step 3.1: Open VCN Creation

1. **Click** ☰ Hamburger menu (top-left)
2. **Navigate**: Networking → Virtual Cloud Networks
3. **Select your region** (should show home region)
4. **Compartment**: Should show root compartment
   - If not, click compartment dropdown and select root

**You'll see**: "You do not have any virtual cloud networks"

### Step 3.2: Start VCN Wizard ⭐

**Click**: "Start VCN Wizard" (big blue button)

**NOT** "Create VCN" - we want the wizard for automatic setup!

### Step 3.3: Choose VCN Configuration

**Select**: "Create VCN with Internet Connectivity"
- This automatically creates everything you need
- Internet gateway, NAT gateway, service gateway
- Public and private subnets
- Security rules including SSH

**Click**: "Start VCN Wizard"

### Step 3.4: Configure VCN Basic Information

**VCN Name**: `voip-vcn` (or any name you prefer)

**Compartment**: (should show root compartment)

**VCN IPv4 CIDR Block**: `10.0.0.0/16` (default, leave as is)
- This gives you 65,536 IP addresses
- More than enough for VoIP server

### Step 3.5: Configure Subnets

**Public Subnet CIDR Block**: `10.0.0.0/24` (default, leave as is)
- 256 IP addresses for public-facing resources

**Private Subnet CIDR Block**: `10.0.1.0/24` (default, leave as is)
- 256 IP addresses for private resources

**Use DNS Hostnames**: ✓ (checked, leave as is)

### Step 3.6: Review Configuration

**Click**: "Next" at bottom

**You'll see a summary**:
- ✅ VCN: voip-vcn (10.0.0.0/16)
- ✅ Public Subnet (10.0.0.0/24)
- ✅ Private Subnet (10.0.1.0/24)
- ✅ Internet Gateway
- ✅ NAT Gateway
- ✅ Service Gateway
- ✅ Route tables
- ✅ Security lists

### Step 3.7: Create VCN

**Click**: "Create" (blue button)

**Wait 10-20 seconds** while Oracle creates all resources.

**You'll see**: "Virtual Cloud Network Created Successfully"
- ✅ VCN created
- ✅ Subnets created
- ✅ Gateways created
- ✅ Route tables configured
- ✅ Security lists configured

**Click**: "View Virtual Cloud Network"

✅ **VCN is ready!**

### Step 3.8: Note VCN Details

**On the VCN details page**, note:
- **VCN Name**: voip-vcn
- **CIDR Block**: 10.0.0.0/16
- **State**: Available (green)

**You'll need this VCN when creating the VM instance.**

---

## Phase 4: Configure VCN Security Lists (10 minutes)

**Why?** We need to open ports for VoIP (9000 TCP, 9001 UDP)

### Step 4.1: Access Security Lists

**You should be on the VCN details page** (voip-vcn)

**Left sidebar**: Click "Security Lists"

**You'll see**:
- "Default Security List for voip-vcn"
- "Security List for Private Subnet..."

### Step 4.2: Edit Public Subnet Security List

**Click**: "Default Security List for voip-vcn"

**You'll see**:
- **Ingress Rules** (inbound traffic)
- **Egress Rules** (outbound traffic)

**Default ingress rules** (already there):
- SSH: TCP port 22 from 0.0.0.0/0 ✓

### Step 4.3: Add VoIP Control Port (TCP 9000)

**Ingress Rules tab**: Click "Add Ingress Rules"

**Stateless**: ☐ (unchecked)

**Source Type**: CIDR

**Source CIDR**: `0.0.0.0/0` (allow from anywhere)

**IP Protocol**: TCP

**Source Port Range**: (leave blank)

**Destination Port Range**: `9000`

**Description**: VoIP WebSocket Control

**Click**: "Add Ingress Rules"

### Step 4.4: Add VoIP Voice Port (UDP 9001)

**Click**: "Add Ingress Rules" again

**Stateless**: ☐ (unchecked)

**Source Type**: CIDR

**Source CIDR**: `0.0.0.0/0`

**IP Protocol**: UDP

**Source Port Range**: (leave blank)

**Destination Port Range**: `9001`

**Description**: VoIP Voice UDP

**Click**: "Add Ingress Rules"

### Step 4.5: Add HTTPS Port (Optional but Recommended)

**Click**: "Add Ingress Rules" again

**Stateless**: ☐ (unchecked)

**Source Type**: CIDR

**Source CIDR**: `0.0.0.0/0`

**IP Protocol**: TCP

**Source Port Range**: (leave blank)

**Destination Port Range**: `443`

**Description**: HTTPS

**Click**: "Add Ingress Rules"

### Step 4.6: Verify Security Rules

**You should now see**:
- ✅ TCP port 22 (SSH)
- ✅ TCP port 9000 (VoIP Control)
- ✅ UDP port 9001 (VoIP Voice)
- ✅ TCP port 443 (HTTPS) - optional

✅ **Security rules configured!**

---

## Phase 5: Create VM Compute Instance (15 minutes)

### Step 5.1: Navigate to Compute Instances

1. **Click** ☰ Hamburger menu
2. **Navigate**: Compute → Instances
3. **Verify**:
   - Region: Your home region
   - Compartment: Root compartment

**You'll see**: "You do not have any instances"

### Step 5.2: Start Instance Creation

**Click**: "Create Instance" (blue button)

### Step 5.3: Name Your Instance

**Name**: `voip-server` (or any name you prefer)

**Create in compartment**: (should show root compartment)

### Step 5.4: Placement

**Availability domain**: (auto-selected, leave as is)
- Oracle automatically chooses the best one

### Step 5.5: Image and Shape (Critical!)

**Image**:
- **Current selection**: Usually Oracle Linux by default
- **Click**: "Edit" next to Image

**Change Image**:
1. **Click**: "Change Image"
2. **Select**: "Canonical Ubuntu"
3. **Version**: 22.04 (Latest)
4. **Click**: "Select Image"

**Shape** (Always Free ARM):
1. **Click**: "Change Shape"
2. **Instance type**: Virtual machine
3. **Shape series**: Click "Ampere"
4. **Shape name**: VM.Standard.A1.Flex
   - You'll see "Always Free Eligible" label ✓

**Configure resources**:
- **Number of OCPUs**: 2 (can use up to 4 free)
  - Start with 2, can create another instance with 2 more
- **Amount of memory (GB)**: 12 (can use up to 24 free)
  - Start with 12, allocate more if needed

**Click**: "Select Shape"

### Step 5.6: Networking

**Primary VNIC**:

**Primary network**:
- **Select existing virtual cloud network**: ✓ (selected)
- **Virtual cloud network in [root]**: Select "voip-vcn"
  - This is the VCN we created earlier

**Subnet**:
- **Subnet in [root]**: Select the **public subnet**
  - "Public Subnet-voip-vcn (Regional)"

**Public IPv4 address**:
- **Select**: "Assign a public IPv4 address" ✓
- This is critical - you need this to connect from internet

**Private IPv4 address**:
- Leave as default (auto-assigned)

### Step 5.7: Add SSH Keys

**IMPORTANT**: Without SSH keys, you can't connect to the instance!

**Choose one method**:

#### Option A: Generate Key Pair (Easiest)

1. **Select**: "Generate a key pair for me" (radio button)
2. **Click**: "Save Private Key"
   - Saves as: `ssh-key-YYYY-MM-DD.key`
   - **Save to**: `C:\Users\YourName\.ssh\oracle_voip.key`
3. **Click**: "Save Public Key"
   - Saves as: `ssh-key-YYYY-MM-DD.key.pub`
   - **Save to**: `C:\Users\YourName\.ssh\oracle_voip.key.pub`

**Keep these files safe!** You need them to connect.

#### Option B: Upload Your Own Public Key

If you already have SSH keys:

1. **Select**: "Upload public key files (.pub)"
2. **Click**: "Browse" or drag file
3. **Upload**: Your `id_rsa.pub` or similar file

#### Option C: Paste Public Key

1. **Select**: "Paste public keys"
2. **Paste** your public key content (starts with `ssh-rsa`)

### Step 5.8: Boot Volume

**Boot volume**: Leave defaults
- 50 GB (within free 200 GB total)
- Balanced performance

**Boot volume encryption**: Leave as default

### Step 5.9: Review and Create

**Scroll down to bottom**

**Verify**:
- ✅ Name: voip-server
- ✅ Shape: VM.Standard.A1.Flex (Always Free)
- ✅ Image: Ubuntu 22.04
- ✅ Network: voip-vcn (public subnet)
- ✅ Public IP: Assigned
- ✅ SSH keys: Configured

**Click**: "Create" (blue button)

### Step 5.10: Wait for Provisioning

**Status will change**:
1. Provisioning (orange) - 1-2 minutes
2. Running (green) - ✓ Ready!

**Do NOT close this page!**

### Step 5.11: Note Instance Details

**Once status is "Running" (green)**:

**Copy these details**:
- **Public IP Address**: (e.g., 158.101.xxx.xxx)
  - This is how you'll connect!
- **Username**: `ubuntu` (for Ubuntu image)
- **Private IP**: (e.g., 10.0.0.2)

**Save the public IP** - you'll use it constantly!

✅ **VM Instance is created and running!**

---

## Phase 6: Configure OS Firewall (5 minutes)

Oracle Cloud has **two firewalls**:
1. ✅ Cloud Security List (done in Phase 4)
2. ⚠️ OS Firewall (iptables) - **do this now**

### Step 6.1: Connect via SSH

**On your Windows machine**, open PowerShell:

```powershell
# Navigate to SSH key directory
cd C:\Users\YourName\.ssh

# Fix key permissions (Windows)
icacls oracle_voip.key /inheritance:r
icacls oracle_voip.key /grant:r "$env:USERNAME:(R)"

# Connect to instance
ssh -i oracle_voip.key ubuntu@YOUR_PUBLIC_IP
```

Replace `YOUR_PUBLIC_IP` with the IP from Step 5.11

**First time connecting**:
- You'll see: "The authenticity of host... can't be established"
- Type: `yes` and press Enter

**You should see**:
```
ubuntu@voip-server:~$
```

✅ **Connected!**

### Step 6.2: Configure iptables

```bash
# Allow SSH (so we don't lock ourselves out)
sudo iptables -I INPUT 1 -p tcp --dport 22 -j ACCEPT

# Allow VoIP Control (TCP 9000)
sudo iptables -I INPUT 2 -p tcp --dport 9000 -j ACCEPT

# Allow VoIP Voice (UDP 9001)
sudo iptables -I INPUT 3 -p udp --dport 9001 -j ACCEPT

# Allow HTTPS (optional)
sudo iptables -I INPUT 4 -p tcp --dport 443 -j ACCEPT

# Save rules
sudo apt install iptables-persistent -y
# When prompted: Yes (save current IPv4 rules), Yes (save IPv6 rules)
```

### Step 6.3: Verify Rules

```bash
# Check iptables rules
sudo iptables -L INPUT -n --line-numbers

# You should see:
# 1  ACCEPT tcp  --  0.0.0.0/0  0.0.0.0/0  tcp dpt:22
# 2  ACCEPT tcp  --  0.0.0.0/0  0.0.0.0/0  tcp dpt:9000
# 3  ACCEPT udp  --  0.0.0.0/0  0.0.0.0/0  udp dpt:9001
# 4  ACCEPT tcp  --  0.0.0.0/0  0.0.0.0/0  tcp dpt:443
```

✅ **OS firewall configured!**

---

## Phase 7: Install Dependencies (10 minutes)

### Step 7.1: Update System

```bash
# Update package lists
sudo apt update

# Upgrade packages
sudo apt upgrade -y

# This may take 3-5 minutes
```

### Step 7.2: Install Rust

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Press 1 (default installation), then Enter

# Load Rust into current shell
source $HOME/.cargo/env

# Verify
rustc --version
cargo --version
```

### Step 7.3: Install PostgreSQL

```bash
# Install PostgreSQL
sudo apt install postgresql postgresql-contrib -y

# Start and enable
sudo systemctl start postgresql
sudo systemctl enable postgresql

# Verify
sudo systemctl status postgresql
# Press 'q' to exit
```

### Step 7.4: Install Build Tools

```bash
# Install required dependencies
sudo apt install -y build-essential pkg-config libssl-dev git
```

✅ **Dependencies installed!**

---

## Phase 8: Setup Database (5 minutes)

```bash
# Switch to postgres user
sudo -u postgres psql
```

**You're now in PostgreSQL shell** (`postgres=#`)

**Run these commands**:
```sql
-- Create database
CREATE DATABASE voip_db;

-- Create user (CHANGE PASSWORD!)
CREATE USER voip_user WITH PASSWORD 'your_super_secure_password_12345';

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE voip_db TO voip_user;

-- Exit
\q
```

✅ **Database configured!**

---

## Phase 9: Upload and Deploy Server (15 minutes)

### Step 9.1: Upload Server Code from Windows

**Open a NEW PowerShell window** on your Windows machine:

```powershell
# Navigate to your project
cd C:\dev\VoIP-System

# Upload server directory to Oracle instance
scp -i C:\Users\YourName\.ssh\oracle_voip.key -r server ubuntu@YOUR_PUBLIC_IP:/home/ubuntu/
```

This uploads your entire server folder.

### Step 9.2: Configure Server

**Back in the SSH session**:

```bash
# Navigate to server directory
cd /home/ubuntu/server

# Edit configuration
nano config/server.toml
```

**Update these settings**:
```toml
[server]
bind_address = "0.0.0.0"
control_port = 9000
voice_port = 9001

[database]
host = "localhost"
port = 5432
database = "voip_db"
username = "voip_user"
password = "your_super_secure_password_12345"  # MATCH database password

[security]
jwt_secret = "your-long-random-string-at-least-32-characters-long-change-this"
tls_cert = "/home/ubuntu/server/certs/server.crt"
tls_key = "/home/ubuntu/server/certs/server.key"
```

**Save**: Ctrl+O, Enter, Ctrl+X

### Step 9.3: Generate SSL Certificate

```bash
# Create certs directory
mkdir -p /home/ubuntu/server/certs
cd /home/ubuntu/server/certs

# Generate certificate (replace YOUR_PUBLIC_IP)
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout server.key \
  -out server.crt \
  -days 365 \
  -subj "/CN=YOUR_PUBLIC_IP"

# Set permissions
chmod 600 server.key
chmod 644 server.crt
```

### Step 9.4: Build Server (ARM Architecture)

```bash
# Navigate to server directory
cd /home/ubuntu/server

# Build for ARM (takes 5-10 minutes on ARM CPU)
cargo build --release
```

**Wait patiently** - ARM compilation is slower than x86.

### Step 9.5: Test Server

```bash
# Run server
cargo run --release
```

**You should see**:
```
✅ Database ready
✅ Database tables created/verified
🎧 Voice server listening on 0.0.0.0:9001
🌐 Control server listening on 0.0.0.0:9000
```

**Press Ctrl+C** to stop.

✅ **Server works!**

---

## Phase 10: Create Auto-Start Service (5 minutes)

```bash
# Create service file
sudo nano /etc/systemd/system/voip-server.service
```

**Paste this**:
```ini
[Unit]
Description=VoIP Server - Encrypted Voice Communication
After=network.target postgresql.service
Requires=postgresql.service

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/home/ubuntu/server
ExecStart=/home/ubuntu/server/target/release/voip_server
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

**Save**: Ctrl+O, Enter, Ctrl+X

**Enable and start**:
```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable on boot
sudo systemctl enable voip-server

# Start now
sudo systemctl start voip-server

# Check status
sudo systemctl status voip-server
```

**You should see**: `Active: active (running)`

**Press 'q'** to exit.

✅ **Server is running 24/7!**

---

## Phase 11: Test from Client (5 minutes)

**On your Windows machine**:

1. **Run**: `voip-client.exe`
2. **Server**: `YOUR_ORACLE_PUBLIC_IP`
3. **Port**: `9000`
4. **Use TLS**: ✓
5. **Username**: Create new account
6. **Click**: Register

**You should see**:
```
🔑 SRTP key exchange complete
✅ Connected to server
```

### Test Voice

1. Create/join a channel
2. Press Space (push-to-talk)
3. Speak into microphone

✅ **IT WORKS!**

---

## Common Issues and Fixes

### Issue: Can't create VCN
**Cause**: Wrong compartment selected
**Fix**: Select root compartment in dropdown

### Issue: Can't SSH to instance
**Cause**: Security list missing SSH rule, or wrong key
**Fix**:
- Verify Security List has port 22 rule
- Check key permissions: `icacls oracle_voip.key ...`
- Use correct username: `ubuntu` (not `root`)

### Issue: Can't connect from client
**Cause**: Firewall not configured
**Fix**:
- Verify Cloud Security List has ports 9000, 9001
- Verify OS iptables: `sudo iptables -L INPUT -n`
- Check server logs: `sudo journalctl -u voip-server -n 50`

### Issue: "No Always Free resources available"
**Cause**: Not in home region, or capacity exhausted
**Fix**:
- Verify you're in HOME REGION (top-right)
- Try different availability domain
- Try different time of day (Oracle limits capacity)

---

## Useful Commands

### View Logs
```bash
sudo journalctl -u voip-server -f
```

### Control Server
```bash
sudo systemctl stop voip-server
sudo systemctl start voip-server
sudo systemctl restart voip-server
sudo systemctl status voip-server
```

### Monitor Resources
```bash
htop
df -h
ss -tuln | grep -E '9000|9001'
```

---

## Summary

**What you now have**:
- ✅ Oracle Cloud free tier account (forever free!)
- ✅ VCN with proper networking
- ✅ ARM-based VM (4 cores, 24GB available)
- ✅ VoIP server running 24/7
- ✅ PostgreSQL database
- ✅ Auto-restart on failure
- ✅ Public IP address

**Total cost**: $0/month forever

**Difficulty**: ⭐⭐⭐⭐⭐ Very Hard (but you did it!)

---

**Sources**:
- [Oracle Cloud Free Tier Official Docs](https://docs.oracle.com/en-us/iaas/Content/FreeTier/freetier.htm)
- [Sign Up Guide](https://docs.oracle.com/en-us/iaas/Content/GSG/Tasks/signingup_topic-Sign_Up_for_Free_Oracle_Cloud_Promotion.htm)
- [VCN Creation Guide](https://docs.oracle.com/en-us/iaas/Content/Network/Tasks/create_vcn.htm)
- [VCN Wizard Documentation](https://docs.oracle.com/en-us/iaas/Content/Network/Tasks/quickstartnetworking.htm)
- [Always Free Resources](https://docs.oracle.com/en-us/iaas/Content/FreeTier/freetier_topic-Always_Free_Resources.htm)
- [Creating VM Instances](https://docs.oracle.com/en-us/iaas/Content/Compute/Tasks/launchinginstance.htm)
