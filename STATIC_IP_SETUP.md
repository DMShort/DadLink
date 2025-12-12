# Static IP Setup - Nighthawk Router

Prevent your local IP from changing so port forwarding keeps working.

---

## Part 1: Reserve Static Local IP (Nighthawk Router)

This ensures your computer always gets the same local IP address (e.g., 192.168.1.100).

### Step 1: Find Your Computer's MAC Address

**Open PowerShell**:

```powershell
ipconfig /all | findstr "Physical"
```

**You'll see**:
```
Physical Address. . . . . . . . . : AA-BB-CC-DD-EE-FF
```

**Write down this MAC address** (e.g., `AA-BB-CC-DD-EE-FF`)

### Step 2: Access Nighthawk Router

1. **Open browser** → `http://192.168.1.1` or `http://routerlogin.net`
2. **Login** with admin credentials
   - Default username: `admin`
   - Default password: Check sticker on router, or `password`

### Step 3: Navigate to DHCP Reservation (Address Reservation)

**Nighthawk routers have different interfaces, here are the common paths**:

#### Nighthawk with Genie Interface (Most Common):
1. **Click**: "Advanced" tab (top)
2. **Navigate**: Setup → LAN Setup
3. **Scroll down** to: "Address Reservation" section
4. **Click**: "Add" button

#### Nighthawk with Classic Interface:
1. **Navigate**: Advanced → Setup → LAN Setup
2. **Find**: "Address Reservation" section
3. **Click**: "Add"

#### Nighthawk with New Interface:
1. **Navigate**: Settings → Setup → LAN Setup
2. **Find**: "Address Reservation"
3. **Click**: "Add"

### Step 4: Add Address Reservation

**Fill in the form**:

- **Device Name**: `VoIP-Server-PC` (or any name)
- **IP Address**: `192.168.1.100` (choose an IP you want to keep)
  - Use the current IP from `ipconfig`, or pick any IP in range 192.168.1.2 - 192.168.1.254
- **MAC Address**: `AA-BB-CC-DD-EE-FF` (from Step 1)
  - Some routers auto-detect devices - you can select from dropdown

**Click**: "Add" or "Apply"

### Step 5: Verify Reservation

**You should see** your reservation in the list:
```
Device Name: VoIP-Server-PC
IP Address: 192.168.1.100
MAC Address: AA-BB-CC-DD-EE-FF
```

### Step 6: Restart Network Connection (Optional but Recommended)

**Option A: Release and Renew IP**:

```powershell
ipconfig /release
ipconfig /renew
```

**Option B: Restart Computer**

After restart, verify:
```powershell
ipconfig | findstr IPv4
```

Should show: `192.168.1.100` (your reserved IP)

---

## Part 2: Handle Public IP Changes (Dynamic DNS)

Your **public IP** (122.150.218.65) might also change if your ISP uses dynamic IPs.

### Check if Your Public IP Changes

**Method 1: Ask your ISP**
- Call customer support
- Ask: "Do I have a static or dynamic IP address?"
- **Static IP** = never changes (rare for home internet)
- **Dynamic IP** = changes occasionally (common)

**Method 2: Monitor for a Week**

```powershell
# Check your public IP daily
curl https://api.ipify.org
```

Write it down each day. If it changes, you have dynamic IP.

---

## Solution: Dynamic DNS (If Public IP Changes)

**Dynamic DNS** gives you a permanent domain name that auto-updates when your IP changes.

### Option 1: Netgear Dynamic DNS (Built into Nighthawk)

**Nighthawk routers have built-in DDNS support!**

#### Step 1: Access Dynamic DNS Settings

1. **Login to router**: `http://192.168.1.1`
2. **Navigate**: Advanced → Advanced Setup → Dynamic DNS

#### Step 2: Enable Dynamic DNS

**Service Provider**: You'll see options like:
- No-IP.com (Free)
- DynDNS.org
- Netgear DDNS (No-IP.com)

**Choose**: "No-IP.com" (Free and reliable)

#### Step 3: Create No-IP Account

1. **Go to**: https://www.noip.com/sign-up
2. **Sign up** for free account
3. **Verify email**
4. **Create hostname**: Choose a name like `myvoipserver.ddns.net`

**You'll get**: `myvoipserver.ddns.net` pointing to your current public IP

#### Step 4: Configure Router with No-IP Credentials

**Back in router Dynamic DNS settings**:

- **Service Provider**: No-IP.com
- **Host Name**: `myvoipserver.ddns.net` (from No-IP account)
- **Username**: Your No-IP username
- **Password**: Your No-IP password
- **Enable**: ✓ (checked)

**Click**: "Apply"

#### Step 5: Verify Dynamic DNS

**Check router status**:
- Should show: "DDNS Status: Registered"
- Or: "Last Update: Success"

**Test**:
```powershell
nslookup myvoipserver.ddns.net
```

Should return your current public IP: `122.150.218.65`

**Now users can connect to**: `myvoipserver.ddns.net:9000` instead of IP address!

---

### Option 2: DuckDNS (Alternative - No Router Support, Manual Update)

If No-IP doesn't work, use DuckDNS with auto-updater script:

1. **Sign up**: https://www.duckdns.org/
2. **Create subdomain**: `myvoipserver.duckdns.org`
3. **Get token** from dashboard
4. **Run updater script** on your computer (see below)

**Windows Auto-Updater Script**:

Create: `C:\duckdns-updater.ps1`

```powershell
# DuckDNS Auto-Updater for Windows
$domain = "myvoipserver"  # Your subdomain (without .duckdns.org)
$token = "your-token-here"  # From DuckDNS dashboard

while ($true) {
    try {
        $response = Invoke-WebRequest -Uri "https://www.duckdns.org/update?domains=$domain&token=$token&ip=" -UseBasicParsing
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$timestamp] DuckDNS updated: $($response.Content)"
    } catch {
        Write-Host "[$timestamp] Update failed: $_" -ForegroundColor Red
    }

    Start-Sleep -Seconds 300  # Update every 5 minutes
}
```

**Run automatically** (create scheduled task):

```powershell
# Create scheduled task to run on startup
$action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-ExecutionPolicy Bypass -WindowStyle Hidden -File C:\duckdns-updater.ps1"
$trigger = New-ScheduledTaskTrigger -AtStartup
Register-ScheduledTask -TaskName "DuckDNS Updater" -Action $action -Trigger $trigger -RunLevel Highest
```

**Users connect to**: `myvoipserver.duckdns.org:9000`

---

## Part 3: Update Port Forwarding (If You Changed Local IP)

If you changed your local IP address, update router port forwarding:

1. **Login to router**: `http://192.168.1.1`
2. **Navigate**: Port Forwarding / Virtual Server
3. **Edit existing rules**:
   - **Internal IP**: Change to new static IP (e.g., 192.168.1.100)
   - Both TCP 9000 and UDP 9001
4. **Save/Apply**

---

## Part 4: Test Everything

### Test Static Local IP

```powershell
# Restart network
ipconfig /release
ipconfig /renew

# Check IP
ipconfig | findstr IPv4
# Should show: 192.168.1.100 (your static IP)
```

### Test Port Forwarding Still Works

```powershell
cd C:\dev\VoIP-System
.\test-connection.ps1
```

Should show `[OK]` for all tests.

### Test Dynamic DNS (If Configured)

**If using domain name**:

```powershell
# Check DNS resolution
nslookup myvoipserver.ddns.net

# Should return your public IP: 122.150.218.65
```

**Test client connection**:
- Server: `myvoipserver.ddns.net` (instead of IP)
- Port: `9000`
- Should connect successfully

---

## Nighthawk Router Model-Specific Notes

### Nighthawk R7000 / R7000P
- **Interface**: Genie
- **DHCP Reservation**: Advanced → Setup → LAN Setup → Address Reservation
- **Dynamic DNS**: Advanced → Advanced Setup → Dynamic DNS

### Nighthawk AX Series (RAX40, RAX50, etc.)
- **Interface**: New Web UI
- **DHCP Reservation**: Settings → Setup → LAN Setup
- **Dynamic DNS**: Advanced → Advanced Setup → Dynamic DNS

### Nighthawk X6 / X10 (R8000, R9000)
- **Interface**: Advanced Genie
- **DHCP Reservation**: Advanced → Setup → LAN Setup
- **Dynamic DNS**: Internet → Dynamic DNS

### Nighthawk M1 / M5 (Mobile Routers)
- **Interface**: Simplified
- **DHCP Reservation**: May not be available (check manual)
- **Dynamic DNS**: Usually not supported

**Can't find the setting?**
- Check your exact model at: https://www.netgear.com/support/
- Search: "[Your Model] DHCP reservation" or "address reservation"

---

## Access Point Considerations

You mentioned you're **hardwired into an access point**.

### Important Notes:

1. **DHCP is handled by the router, not the access point**
   - Set DHCP reservation on the **router** (main Nighthawk device)
   - Access point just passes traffic through

2. **MAC Address**
   - Use the MAC address of your computer's **Ethernet adapter**
   - Not the access point's MAC address

3. **IP Range**
   - If access point has its own DHCP (rare), disable it
   - Use only router's DHCP with reservations

### Verify Setup:

```powershell
# Check default gateway (should be router, not AP)
ipconfig | findstr "Default Gateway"
# Should show: 192.168.1.1 (router IP)

# Check DHCP server (should be router)
ipconfig /all | findstr "DHCP Server"
# Should show: 192.168.1.1 (router IP)
```

---

## Summary Checklist

**For Local IP (Required for port forwarding)**:
- [ ] Find MAC address: `ipconfig /all | findstr Physical`
- [ ] Login to Nighthawk router: `http://192.168.1.1`
- [ ] Navigate to Address Reservation / DHCP Reservation
- [ ] Add reservation: IP 192.168.1.100 → MAC AA-BB-CC-DD-EE-FF
- [ ] Apply changes
- [ ] Restart network: `ipconfig /release` then `ipconfig /renew`
- [ ] Verify: `ipconfig | findstr IPv4` shows 192.168.1.100
- [ ] Update port forwarding internal IP if changed

**For Public IP (Optional, if IP changes)**:
- [ ] Check if public IP is dynamic (monitor for a week)
- [ ] If dynamic, set up Dynamic DNS:
  - [ ] Option A: Use router built-in DDNS (No-IP)
  - [ ] Option B: Use DuckDNS with updater script
- [ ] Test DNS: `nslookup myvoipserver.ddns.net`
- [ ] Update users with domain name instead of IP

**Final Test**:
- [ ] Run: `.\test-connection.ps1` - all [OK]
- [ ] Connect client to server - works
- [ ] Restart computer - IP stays the same
- [ ] Check IP tomorrow - still works

---

## Troubleshooting

### "I set reservation but IP changed anyway"

**Possible causes**:
1. **Wrong MAC address** - Make sure you used Ethernet adapter MAC, not Wi-Fi or access point
2. **Reservation not saved** - Click Apply/Save in router interface
3. **Router rebooted** - Check reservation is still there after router restart
4. **IP conflict** - Another device might be using that IP

**Fix**:
```powershell
# Release old IP
ipconfig /release

# Clear ARP cache
arp -d

# Renew with reservation
ipconfig /renew

# Verify
ipconfig /all
```

### "Can't find Address Reservation in router"

**Different names in Nighthawk routers**:
- "Address Reservation"
- "Reserved IP Addresses"
- "DHCP Reservation"
- "Static IP Assignment"

**Look in these sections**:
- Setup → LAN Setup
- Advanced → Setup → LAN Setup
- Settings → Network Settings → DHCP Settings

### "Dynamic DNS shows 'Update Failed'"

**Common causes**:
1. **Wrong credentials** - Double-check No-IP username/password
2. **Hostname not verified** - Check No-IP email and verify hostname
3. **Service down** - Try different DDNS provider
4. **Router firmware outdated** - Update Nighthawk firmware

---

## Quick Command Reference

```powershell
# Get MAC address
ipconfig /all | findstr "Physical"

# Get current local IP
ipconfig | findstr IPv4

# Get public IP
curl https://api.ipify.org

# Release and renew IP
ipconfig /release
ipconfig /renew

# Test DNS
nslookup myvoipserver.ddns.net

# Test connection
cd C:\dev\VoIP-System
.\test-connection.ps1
```

---

**Once you set up static local IP, your port forwarding will work reliably!**

The Dynamic DNS is optional - only needed if your public IP changes and you want a permanent address to share with users.
