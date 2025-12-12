# Home Hosting Guide - Port Forwarding Setup

Host your VoIP server from your home computer with internet access.

**Cost**: $0/month
**Setup time**: 15-20 minutes
**Difficulty**: ⭐⭐ Easy

---

## What You Need

- ✅ Server running on your Windows computer (you have this!)
- ✅ Router admin access (you have this now!)
- ✅ Computer stays on 24/7 (or when you want server available)
- ✅ Stable internet connection

---

## Step 1: Configure Windows Firewall (5 minutes)

### Option A: Use Automated Script (Easiest)

You already have the script!

**Run as Administrator**:

1. **Right-click** PowerShell → "Run as Administrator"
2. **Navigate** to your project:
   ```powershell
   cd C:\dev\VoIP-System
   ```
3. **Run** the firewall script:
   ```powershell
   powershell -ExecutionPolicy Bypass -File fix-firewall.ps1
   ```

**You should see**:
```
========================================
VoIP Server - Firewall Configuration
========================================

[1/4] Checking existing firewall rules...
[2/4] Creating TCP firewall rule (Port 9000)...
  ✓ TCP Port 9000 allowed
[3/4] Creating UDP firewall rule (Port 9001)...
  ✓ UDP Port 9001 allowed
[4/4] Verifying rules...

========================================
Firewall Configuration Complete!
========================================

Firewall rules created:
  ✓ TCP Port 9000 (WebSocket Control)
  ✓ UDP Port 9001 (Voice Packets)

Your server is now accessible from the internet!
```

**Press any key** to continue.

✅ **Windows Firewall configured!**

### Option B: Manual Configuration

If the script doesn't work:

1. **Open** Windows Defender Firewall with Advanced Security
   - Press `Win + R`
   - Type: `wf.msc`
   - Press Enter

2. **Click** "Inbound Rules" (left sidebar)

3. **Click** "New Rule..." (right sidebar)

**Rule 1 - TCP Port 9000**:
- Rule Type: Port → Next
- Protocol: TCP
- Specific local ports: `9000` → Next
- Action: Allow the connection → Next
- Profile: Check all (Domain, Private, Public) → Next
- Name: `VoIP Server - WebSocket Control` → Finish

**Rule 2 - UDP Port 9001**:
- Rule Type: Port → Next
- Protocol: UDP
- Specific local ports: `9001` → Next
- Action: Allow the connection → Next
- Profile: Check all (Domain, Private, Public) → Next
- Name: `VoIP Server - Voice UDP` → Finish

✅ **Windows Firewall configured manually!**

---

## Step 2: Find Your Computer's Local IP (2 minutes)

You need this for port forwarding configuration.

### Find Local IP Address

**Open PowerShell**:

```powershell
ipconfig | findstr "IPv4"
```

**You'll see something like**:
```
IPv4 Address. . . . . . . . . . . : 192.168.1.100
```

**Your local IP**: `192.168.1.100` (example - yours will be different)

**Common patterns**:
- `192.168.1.xxx`
- `192.168.0.xxx`
- `10.0.0.xxx`

**Write this down** - you'll need it for router configuration.

### Optional: Set Static IP (Recommended)

**Why?** Your local IP might change when you restart your computer. Setting it to static ensures port forwarding keeps working.

**How**:

1. **Open** Network Connections
   - Press `Win + R`
   - Type: `ncpa.cpl`
   - Press Enter

2. **Right-click** your active network adapter (usually "Ethernet" or "Wi-Fi")
3. **Select** "Properties"
4. **Double-click** "Internet Protocol Version 4 (TCP/IPv4)"
5. **Select** "Use the following IP address"
6. **Enter**:
   - IP address: `192.168.1.100` (your current IP)
   - Subnet mask: `255.255.255.0` (usually auto-fills)
   - Default gateway: `192.168.1.1` (your router IP)
   - Preferred DNS: `8.8.8.8` (Google DNS)
   - Alternate DNS: `8.8.4.4`
7. **Click** OK, OK

**Test connection**:
```powershell
ping google.com
```

Should work normally.

---

## Step 3: Find Your Router's IP Address (1 minute)

You need this to access router admin panel.

**Find Default Gateway**:

```powershell
ipconfig | findstr "Default Gateway"
```

**You'll see**:
```
Default Gateway . . . . . . . . . : 192.168.1.1
```

**Your router IP**: `192.168.1.1` (example - common values are 192.168.1.1, 192.168.0.1, 10.0.0.1)

**Write this down**.

---

## Step 4: Access Router Admin Panel (2 minutes)

### Login to Router

1. **Open browser** (Chrome, Firefox, Edge)
2. **Go to**: `http://192.168.1.1` (or your router IP)
3. **Login** with router credentials

**Common default credentials** (if you don't know them):
- Username: `admin`, Password: `admin`
- Username: `admin`, Password: `password`
- Username: `admin`, Password: (blank)
- Check sticker on router for default password

**Can't login?**
- Check router manual
- Look for sticker on back/bottom of router
- Google: "[Your Router Brand/Model] default password"

**Once logged in**, you'll see the router admin dashboard.

---

## Step 5: Configure Port Forwarding (10 minutes)

**Every router brand has different interface**, but the concept is the same.

### Common Router Interfaces

**Look for one of these menu options**:
- "Port Forwarding"
- "Virtual Server"
- "NAT" or "NAT Forwarding"
- "Applications & Gaming"
- "Firewall" → "Port Forwarding"
- "Advanced" → "Port Forwarding"

### Generic Port Forwarding Instructions

**You'll need to create 2 rules**:

#### Rule 1: VoIP Control (TCP 9000)

**Settings**:
- **Service Name/Description**: `VoIP Control` (or any name)
- **External Port** (or Start Port): `9000`
- **Internal Port** (or End Port): `9000`
- **Protocol**: `TCP` (or Both)
- **Internal IP** (or Server IP): `192.168.1.100` (your computer's local IP)
- **Enable/Active**: ✓ (checked)

**Click** Save/Apply

#### Rule 2: VoIP Voice (UDP 9001)

**Settings**:
- **Service Name/Description**: `VoIP Voice`
- **External Port**: `9001`
- **Internal Port**: `9001`
- **Protocol**: `UDP` (or Both)
- **Internal IP**: `192.168.1.100` (your computer's local IP)
- **Enable/Active**: ✓ (checked)

**Click** Save/Apply

### Brand-Specific Guides

#### TP-Link Routers

1. **Navigate**: Advanced → NAT Forwarding → Virtual Servers
2. **Click**: "Add"
3. **Service Port**: 9000
4. **Internal Port**: 9000
5. **IP Address**: 192.168.1.100
6. **Protocol**: TCP
7. **Status**: Enabled
8. **Click**: Save
9. Repeat for UDP 9001

#### Netgear Routers

1. **Navigate**: Advanced → Advanced Setup → Port Forwarding/Port Triggering
2. **Select**: Port Forwarding
3. **Click**: "Add Custom Service"
4. **Service Name**: VoIP Control
5. **External Port Range**: 9000-9000
6. **Internal Port Range**: 9000-9000
7. **Internal IP**: 192.168.1.100
8. **Protocol**: TCP
9. Repeat for UDP 9001

#### Linksys Routers

1. **Navigate**: Security → Apps and Gaming → Single Port Forwarding
2. **Application Name**: VoIP Control
3. **External Port**: 9000
4. **Internal Port**: 9000
5. **Protocol**: TCP
6. **To IP Address**: 192.168.1.100
7. **Enabled**: ✓
8. Repeat for UDP 9001

#### ASUS Routers

1. **Navigate**: WAN → Virtual Server/Port Forwarding
2. **Enable Port Forwarding**: Yes
3. **Service Name**: VoIP Control
4. **Port Range**: 9000
5. **Local IP**: 192.168.1.100
6. **Local Port**: 9000
7. **Protocol**: TCP
8. **Click**: Add (+)
9. Repeat for UDP 9001

#### D-Link Routers

1. **Navigate**: Advanced → Port Forwarding
2. **Name**: VoIP Control
3. **Public Port**: 9000
4. **Private Port**: 9000
5. **Traffic Type**: TCP
6. **Private IP**: 192.168.1.100
7. **Schedule**: Always
8. Repeat for UDP 9001

### Can't Find Port Forwarding?

**Try searching router manual**:
1. Google: "[Your Router Model] port forwarding guide"
2. Example: "TP-Link Archer C7 port forwarding"

**Or check**: https://portforward.com/router.htm
- Select your router model
- Follow specific instructions

---

## Step 6: Find Your Public IP Address (1 minute)

This is the IP address users will use to connect to your server.

### Method 1: PowerShell (Easiest)

```powershell
curl https://api.ipify.org
```

**You'll see**: `203.0.113.45` (example - yours will be different)

### Method 2: Browser

**Open browser** and go to one of these:
- https://whatismyipaddress.com/
- https://www.whatismyip.com/
- https://ipinfo.io/ip

**You'll see your public IP**: `203.0.113.45` (example)

**Write this down** - this is what users will use to connect!

---

## Step 7: Test Port Forwarding (5 minutes)

### Test from Same Network (Internal Test)

**On your Windows machine**:

1. **Run** the test script:
   ```powershell
   cd C:\dev\VoIP-System
   powershell -ExecutionPolicy Bypass -File test-connection.ps1
   ```

**You should see**:
```
========================================
VoIP Server - Connection Test
========================================

Testing connection to: 127.0.0.1:9000

[1/5] Testing TCP port 9000...
  ✓ TCP Port 9000 is accessible

[2/5] Checking if server is running...
  ✓ Server process found

[3/5] Checking firewall rules...
  ✓ TCP firewall rule exists
  ✓ UDP firewall rule exists

[4/5] Checking if port is listening...
  ✓ Server is listening on port 9000

[5/5] Network Information...
  Local IP: 192.168.1.100
  Public IP: 203.0.113.45

========================================
Summary
========================================

Status: ✓ Server should be accessible

Users can connect with:
  Server: 203.0.113.45
  Port: 9000
  Use TLS: ✓
```

✅ **Internal test passed!**

### Test from External Network (Real Test)

**You need to test from OUTSIDE your network** (port forwarding only works from external connections).

**Options**:

#### Option A: Test with Client from Different Network

1. **On a different computer** (friend's house, mobile hotspot, etc.)
2. **Run** voip-client.exe
3. **Server**: `YOUR_PUBLIC_IP` (e.g., 203.0.113.45)
4. **Port**: `9000`
5. **Use TLS**: ✓
6. **Try to connect**

**Should work!**

#### Option B: Online Port Checker

1. **Go to**: https://www.yougetsignal.com/tools/open-ports/
2. **Remote Address**: `YOUR_PUBLIC_IP`
3. **Port Number**: `9000`
4. **Click**: "Check"

**Result**:
- ✅ "Port 9000 is open" - Success!
- ❌ "Port 9000 is closed" - Port forwarding not working

#### Option C: Mobile Hotspot Test

1. **Disconnect** your computer from Wi-Fi/Ethernet
2. **Connect** to mobile hotspot (different network)
3. **Test** connection to your public IP
4. **Reconnect** to home network when done

---

## Step 8: Verify Server is Running (2 minutes)

### Check Server Status

**PowerShell**:

```powershell
# Check if server is running
tasklist | findstr voip
```

**You should see**:
```
voip_server.exe      12345 Console     1     45,678 K
```

If you don't see it, the server isn't running.

### Start Server

**If server is not running**:

```powershell
cd C:\dev\VoIP-System\server
cargo run --release
```

**Keep this window open** while server is running.

### Keep Server Running (Auto-Start)

**Option A: Keep PowerShell Open**
- Just leave the PowerShell window with `cargo run --release` running
- Server stops if you close window or restart computer

**Option B: Windows Service (Advanced)**

See "Run as Windows Service" section in [TROUBLESHOOTING.md](./TROUBLESHOOTING.md)

---

## Step 9: Share Your Server Address (1 minute)

**Give users this information**:

```
🎧 VoIP Server Connection Details

Server: YOUR_PUBLIC_IP
Port: 9000
Use TLS: ✓ (enabled)

Download client: [Link to VoIP-Client-v0.1.0.zip]

Instructions:
1. Extract ZIP file
2. Run voip-client.exe
3. Enter server address above
4. Create an account
5. Join a channel and start talking!
```

**Example**:
```
Server: 203.0.113.45
Port: 9000
Use TLS: ✓
```

---

## Troubleshooting

### Issue: Users can't connect from internet

**Possible causes**:

1. **Port forwarding not configured correctly**
   - Double-check router settings
   - Verify internal IP is correct
   - Ensure rules are enabled

2. **Windows Firewall blocking**
   - Re-run fix-firewall.ps1 script
   - Check firewall rules: `Get-NetFirewallRule | Where-Object {$_.DisplayName -like "*VoIP*"}`

3. **Server not running**
   - Check: `tasklist | findstr voip`
   - Start: `cargo run --release`

4. **Public IP changed**
   - Check current public IP: `curl https://api.ipify.org`
   - Update users with new IP
   - Consider dynamic DNS (see below)

5. **ISP blocking ports**
   - Some ISPs block incoming connections
   - Try using port 443 instead (see below)

### Issue: Local connection works but external doesn't

**This confirms**:
- ✅ Server is running
- ✅ Windows Firewall is open
- ❌ Router port forwarding not working

**Fix**:
- Re-check router port forwarding settings
- Ensure external port = 9000, internal port = 9000
- Verify internal IP is correct (192.168.1.100)
- Restart router

### Issue: Connection works for 5 minutes then drops

**Cause**: NAT timeout on router

**Fix**:
- Look for "NAT Timeout" or "Session Timeout" in router settings
- Increase to maximum (usually 3600 seconds)
- Or rely on keepalive (already implemented in server)

### Issue: Public IP keeps changing

**Cause**: Your ISP uses dynamic IP addresses

**Fix**: Use Dynamic DNS (see next section)

---

## Dynamic DNS (Optional - Recommended)

**Problem**: Your public IP changes periodically (every few days/weeks)

**Solution**: Use a Dynamic DNS service to get a permanent domain name

### Free Dynamic DNS Services

#### DuckDNS (Easiest, Recommended)

1. **Sign up**: https://www.duckdns.org/
   - Login with Google/GitHub
2. **Create subdomain**: `myvoipserver.duckdns.org`
3. **Get token**: (shown on dashboard)
4. **Install updater**:

**Windows PowerShell Script** (`C:\duckdns-update.ps1`):
```powershell
# DuckDNS Auto-Updater
$domain = "myvoipserver"  # Your subdomain
$token = "your-token-here"  # Your DuckDNS token

while ($true) {
    $response = Invoke-WebRequest -Uri "https://www.duckdns.org/update?domains=$domain&token=$token&ip="
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] DuckDNS updated: $($response.Content)"
    Start-Sleep -Seconds 300  # Update every 5 minutes
}
```

**Run script**:
```powershell
powershell -ExecutionPolicy Bypass -File C:\duckdns-update.ps1
```

**Users connect to**: `myvoipserver.duckdns.org:9000`

#### No-IP (Alternative)

1. **Sign up**: https://www.noip.com/
2. **Create hostname**: `myvoipserver.ddns.net`
3. **Download** No-IP DUC (Dynamic Update Client)
4. **Install and configure**
5. **Runs in background**, auto-updates IP

**Users connect to**: `myvoipserver.ddns.net:9000`

#### FreeDNS (Alternative)

1. **Sign up**: https://freedns.afraid.org/
2. **Create subdomain**
3. **Configure auto-update** (similar to DuckDNS)

---

## Using Non-Standard Ports (If ISP Blocks)

**Some ISPs block ports below 1024 or common ports**.

**If users can't connect**, try using port 443 (HTTPS standard port):

### Change Server Config

**Edit** `server/config/server.toml`:
```toml
[server]
control_port = 443  # Changed from 9000
voice_port = 9001   # Keep as is
```

### Update Firewall

```powershell
# Add firewall rule for port 443
New-NetFirewallRule -DisplayName "VoIP Server - Port 443" `
    -Direction Inbound -LocalPort 443 -Protocol TCP -Action Allow
```

### Update Router Port Forwarding

- External Port: 443
- Internal Port: 443
- Protocol: TCP
- Internal IP: 192.168.1.100

### Update Client Connection

**Users connect to**:
- Server: YOUR_PUBLIC_IP
- Port: 443 (changed from 9000)

**Why port 443?**
- Standard HTTPS port
- Rarely blocked by ISPs
- Less likely to be filtered

---

## Bandwidth Usage Estimate

**Per user voice transmission**:
- ~32 kbps (Opus codec)
- ~240 KB/minute
- ~14 MB/hour

**Example scenarios**:
- 5 users talking for 2 hours = ~140 MB
- 10 users talking for 4 hours = ~560 MB

**Most home internet** can handle 5-20 concurrent users easily.

---

## Advantages of Home Hosting

✅ **Free** - No monthly costs
✅ **Full control** - Your hardware, your rules
✅ **Low latency** - For local users
✅ **Privacy** - Data never leaves your network (for local users)
✅ **Easy updates** - Just restart server

---

## Disadvantages of Home Hosting

❌ **Computer must stay on** - 24/7 if you want 24/7 availability
❌ **Public IP may change** - Need dynamic DNS
❌ **Single point of failure** - If computer/internet goes down, server is offline
❌ **Home internet limits** - Upload bandwidth may be limited
❌ **Power usage** - Computer running 24/7 uses electricity
❌ **No redundancy** - No backup if hardware fails

---

## Next Steps

1. ✅ Configure Windows Firewall (fix-firewall.ps1)
2. ✅ Set static local IP (optional but recommended)
3. ✅ Configure router port forwarding
4. ✅ Test from external network
5. ✅ Set up Dynamic DNS (optional but recommended)
6. ✅ Share server address with users

**Your server is now accessible from the internet!**

---

## Quick Reference

**Your Network Configuration**:
- Local IP: `192.168.1.100` (your computer)
- Router IP: `192.168.1.1` (default gateway)
- Public IP: `203.0.113.45` (or use Dynamic DNS)

**Ports to Forward**:
- TCP 9000 → 192.168.1.100:9000 (WebSocket Control)
- UDP 9001 → 192.168.1.100:9001 (Voice Packets)

**Users Connect To**:
- Server: `YOUR_PUBLIC_IP:9000` (or domain.duckdns.org:9000)
- Use TLS: ✓

**Cost**: $0/month

**Perfect for**: Friends, gaming groups, small teams
