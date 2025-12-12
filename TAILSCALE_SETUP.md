# Tailscale Setup - VoIP Server

**Best solution for your CGNAT/changing IP situation!**

Tailscale creates a secure mesh VPN that:
- ✅ Supports both TCP and UDP (full voice quality)
- ✅ Works behind CGNAT
- ✅ Free for personal use (up to 100 devices)
- ✅ Very simple setup (10 minutes)
- ✅ Lower latency than Cloudflare Tunnel

---

## How Tailscale Works

**Traditional port forwarding** (doesn't work for you):
```
User → Internet → Your Router → Your PC
        ↑ Blocked by CGNAT
```

**Tailscale mesh VPN**:
```
User (Tailscale) ←→ Tailscale Coordination Server ←→ Your PC (Tailscale)
                          ↓
                  Establishes direct peer-to-peer connection
```

**Key features**:
- Assigns each device a permanent Tailscale IP (100.x.x.x)
- Creates direct connections when possible (low latency)
- Falls back to relay servers when needed
- Encrypted (WireGuard protocol)

---

## Part 1: Install Tailscale on Server PC

### Step 1: Download Tailscale

**Windows installer**:
1. **Go to**: https://tailscale.com/download/windows
2. **Download**: `tailscale-setup.exe`
3. **Run installer**
4. **Click**: "Install"

**Or use PowerShell**:

```powershell
# Download installer
Invoke-WebRequest -Uri "https://pkgs.tailscale.com/stable/tailscale-setup-latest.exe" -OutFile "$env:TEMP\tailscale-setup.exe"

# Run installer
Start-Process -FilePath "$env:TEMP\tailscale-setup.exe" -Wait
```

### Step 2: Sign Up / Login

**After installation**:
1. Tailscale icon appears in system tray
2. **Click** the Tailscale icon
3. **Click**: "Log in"
4. **Choose** login method:
   - Google account
   - Microsoft account
   - GitHub account
   - Email

5. **Authorize** in browser
6. **Return to desktop**

### Step 3: Get Your Tailscale IP

```powershell
# Check Tailscale status
tailscale status

# Get your IP
tailscale ip
```

**Output**:
```
100.123.45.67  # Your Tailscale IP
```

**Write this down!** Users will connect to this IP.

---

## Part 2: Install Tailscale on Client Devices

**Every user needs Tailscale installed** to connect to your server.

### Windows Client

1. **Download**: https://tailscale.com/download/windows
2. **Install** and **login** with same method (Google/Microsoft/GitHub)
3. **Connected!** You're now on the same virtual network

### macOS Client

1. **Download**: https://tailscale.com/download/mac
2. **Install** and **login**

### Linux Client

```bash
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up
```

### Android / iOS

1. **Download** from App Store / Play Store
2. **Login**

**Important**: All users must login to the **same Tailscale account** OR you must share your machine with them.

---

## Part 3: Share Your Server (For Multiple Users)

### Option 1: Everyone Uses Same Account (Simple)

**Best for**: Small groups (family, close friends)

1. Everyone logs in with **same** Google/Microsoft/GitHub account
2. Everyone automatically sees each other
3. No configuration needed

### Option 2: Share Machine (Better for Strangers)

**Best for**: Public server, don't want to share account

1. **Go to**: https://login.tailscale.com/admin/machines
2. **Find** your server machine
3. **Click**: "Share..."
4. **Share via link** or **email**
5. Users click link and login with their own account
6. They can now see your shared machine only

---

## Part 4: Configure VoIP Server

**No changes needed!** Your server still listens on:
- Port 9000 (WebSocket control)
- Port 9001 (Voice UDP)

Tailscale works at the network layer, not application layer.

---

## Part 5: Update Client Connection

### Get Your Tailscale IP

```powershell
tailscale ip -4
```

**Example output**: `100.123.45.67`

### Client Connection Settings

**Old** (doesn't work with CGNAT):
```
Server: dadlink.ddns.net
Port: 9000
```

**New** (Tailscale):
```
Server: 100.123.45.67  # Your Tailscale IP
Port: 9000
```

**URL format**:
```
ws://100.123.45.67:9000  # or wss:// if using TLS
```

---

## Part 6: Testing

### Test 1: Check Tailscale Connection

**On server**:
```powershell
tailscale status
```

**Should show**:
- Your machine name
- Status: "Connected"
- Other devices (if any)

### Test 2: Ping Test

**From client machine** (also on Tailscale):

```powershell
# Ping server Tailscale IP
ping 100.123.45.67

# Should show:
# Reply from 100.123.45.67: bytes=32 time=20ms
```

### Test 3: VoIP Connection

1. **Client** connects to: `100.123.45.67:9000`
2. **Should connect** successfully
3. **Voice should work** with full UDP quality

---

## Part 7: Make Server Always Accessible

### Enable "Exit Node" (Optional)

**Allows** server to be accessible even when behind NAT:

```powershell
# Advertise as exit node
tailscale up --advertise-exit-node

# Approve in admin console
```

### Disable Key Expiry (Important!)

**By default**, Tailscale keys expire after 180 days.

**To disable**:
1. **Go to**: https://login.tailscale.com/admin/machines
2. **Find** your server machine
3. **Click**: "..." menu
4. **Select**: "Disable key expiry"

**Now**: Server stays connected forever (survives reboots, no re-login needed)

---

## Part 8: Run as Windows Service (Already Automatic)

**Good news**: Tailscale already runs as a service!

**Features**:
- ✅ Starts automatically on boot
- ✅ Runs in background
- ✅ Survives reboots
- ✅ No manual startup needed

**Check service status**:

```powershell
Get-Service -Name "Tailscale"
```

---

## Advantages of Tailscale

✅ **Full UDP support** - Best voice quality
✅ **Works behind CGNAT** - No public IP needed
✅ **IP changes don't matter** - Stable Tailscale IPs
✅ **No port forwarding** - No router configuration
✅ **Free for personal use** - Up to 100 devices
✅ **Peer-to-peer** - Lower latency than relay solutions
✅ **Encrypted** - WireGuard protocol
✅ **Easy to use** - Just install and login

---

## Disadvantages

❌ **Requires Tailscale on every client** - All users must install
❌ **Tailscale account needed** - Users must login
❌ **Not truly "public"** - Only Tailscale users can connect
❌ **100 device limit** (free tier) - Okay for most use cases

---

## Comparison: Tailscale vs Cloudflare Tunnel

| Feature | Tailscale | Cloudflare Tunnel |
|---------|-----------|-------------------|
| UDP Support | ✅ Yes | ❌ No (free tier) |
| Voice Quality | ✅ Excellent | ⚠️ Good (TCP fallback) |
| Setup Difficulty | ✅ Very Easy | ⚠️ Moderate |
| Client Requirement | ❌ Must install | ✅ No install needed |
| Public Access | ❌ Tailscale users only | ✅ Anyone |
| Latency | ✅ Low (P2P) | ⚠️ Medium (relay) |
| Free Tier | ✅ 100 devices | ✅ Unlimited |

**Recommendation**:
- **Use Tailscale** if: Voice quality matters, users willing to install Tailscale
- **Use Cloudflare Tunnel** if: Need public access, users can't install software

---

## Tailscale for Public Access (Hybrid Approach)

**If you want**: Best voice quality + public access

**Use both**:
1. **Tailscale** for trusted users (best quality)
2. **Cloudflare Tunnel** for public/untrusted users (TCP fallback)

**Server listens on**:
- Tailscale IP: `100.123.45.67:9000` (UDP works)
- Cloudflare: `voip.yourdomain.com` (TCP only)

Users choose based on preference/access.

---

## Troubleshooting

### "Can't login to Tailscale"

**Check**:
- Internet connection working
- Firewall not blocking Tailscale
- Using supported login method

**Fix**:
```powershell
# Restart Tailscale service
Restart-Service -Name "Tailscale"

# Try login again
tailscale up
```

### "Client can't see server"

**Possible causes**:
1. **Different accounts** - Both must be on same Tailscale network
2. **Machine not shared** - Share machine in admin console
3. **Tailscale not running** - Check service status

**Fix**:
```powershell
# Check Tailscale status
tailscale status

# Should show both machines
```

### "Connection works but still high latency"

**Cause**: Not using direct P2P connection (using relay)

**Check**:
```powershell
tailscale status
```

**Look for**: `relay` vs `direct` connection type

**Fix**: Ensure both machines can do UDP hole punching (some corporate networks block it)

---

## Quick Start Commands

```powershell
# Download and install (manual)
# Visit: https://tailscale.com/download/windows

# Check status
tailscale status

# Get your IP
tailscale ip

# Login
tailscale up

# Logout
tailscale down

# Restart service
Restart-Service -Name "Tailscale"
```

---

## Distribution to Users

**Create instructions for users**:

```
1. Install Tailscale:
   - Windows: https://tailscale.com/download/windows
   - Mac: https://tailscale.com/download/mac
   - Linux: curl -fsSL https://tailscale.com/install.sh | sh

2. Login with Google/Microsoft/GitHub account

3. (If not using same account)
   - Ask admin for machine share link
   - Click link and approve

4. Connect to VoIP server:
   - Server: 100.123.45.67
   - Port: 9000

Done! You're connected through secure mesh VPN.
```

---

## Next Steps

1. **Install Tailscale** on server PC
2. **Get Tailscale IP** (`tailscale ip`)
3. **Test locally** by connecting to Tailscale IP
4. **Install Tailscale** on client device
5. **Test connection** from client
6. **Share machine** if users have different accounts
7. **Distribute instructions** to users

Once complete, your VoIP server will work perfectly behind CGNAT with full UDP voice quality!
