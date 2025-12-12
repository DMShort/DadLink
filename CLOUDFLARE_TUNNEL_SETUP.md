# Cloudflare Tunnel Setup - VoIP Server

Cloudflare Tunnel creates a secure connection from your server to Cloudflare's network, bypassing CGNAT and port forwarding entirely.

**Perfect for your situation**: Your IP changes every few seconds, making traditional port forwarding impossible.

---

## Important: UDP Limitation

**Cloudflare Tunnel Free Tier**:
- ✅ TCP traffic (WebSocket control port 9000)
- ❌ UDP traffic (voice packets port 9001)

**Impact**:
- Voice will automatically fall back to TCP
- Slightly higher latency (~30-80ms added)
- Still works perfectly, just not optimal

**Alternatives if UDP is critical**:
- See [TAILSCALE_SETUP.md](./TAILSCALE_SETUP.md) - supports UDP, free, easier setup
- Use VPS hosting - [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)

---

## Prerequisites

- Cloudflare account (free)
- Domain name (free options available)
- Windows computer running VoIP server

---

## Part 1: Create Cloudflare Account & Get Domain

### Step 1: Sign Up for Cloudflare

1. **Go to**: https://dash.cloudflare.com/sign-up
2. **Enter email** and create password
3. **Verify email**
4. **Login** to dashboard

### Step 2: Get a Free Domain (If you don't have one)

**Option A: Use a free subdomain service**

You can use services like:
- **FreeDNS (afraid.org)**: Free subdomains
- **Duck DNS**: Free subdomains
- **No-IP**: Free subdomains (you already have dadlink.ddns.net)

**Option B: Buy cheap domain ($1-2/year)**

- Namecheap: .xyz domains ~$1/year
- Porkbun: .xyz domains ~$1/year
- Cloudflare Registrar: At-cost pricing

**Option C: Use Cloudflare's free tunnel domain**

Cloudflare provides a free `*.trycloudflare.com` subdomain, but:
- ⚠️ URL changes on restart
- ⚠️ Not suitable for permanent deployment

**For this guide, we'll use Option C** (free Cloudflare subdomain) to get started quickly. You can switch to your own domain later.

---

## Part 2: Install Cloudflare Tunnel Client

### Step 1: Download cloudflared

1. **Go to**: https://github.com/cloudflare/cloudflared/releases/latest
2. **Download**: `cloudflared-windows-amd64.exe`
3. **Save to**: `C:\Program Files\Cloudflare\cloudflared.exe`

**Or use PowerShell** to download automatically:

```powershell
# Create directory
New-Item -ItemType Directory -Path "C:\Program Files\Cloudflare" -Force

# Download cloudflared
Invoke-WebRequest -Uri "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe" -OutFile "C:\Program Files\Cloudflare\cloudflared.exe"

# Verify installation
& "C:\Program Files\Cloudflare\cloudflared.exe" --version
```

Should show version like: `cloudflared version 2024.x.x`

### Step 2: Add to PATH (Optional)

```powershell
# Add to system PATH
$env:Path += ";C:\Program Files\Cloudflare"
[Environment]::SetEnvironmentVariable("Path", $env:Path, [EnvironmentVariableTarget]::Machine)
```

---

## Part 3: Quick Start - Free Temporary Tunnel

**For testing** (URL changes on restart):

```powershell
cd C:\dev\VoIP-System

# Start VoIP server (in one terminal)
cd server
cargo run --release

# Start Cloudflare tunnel (in another terminal)
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel --url http://localhost:9000
```

**Output**:
```
Your quick Tunnel has been created! Visit it at:
https://random-name-1234.trycloudflare.com
```

**Users connect to**: `wss://random-name-1234.trycloudflare.com` (instead of dadlink.ddns.net:9000)

**Limitation**: URL changes every time you restart cloudflared.

---

## Part 4: Persistent Tunnel Setup

**For production** (permanent URL):

### Step 1: Authenticate with Cloudflare

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel login
```

**This will**:
1. Open browser to Cloudflare login
2. Ask you to select domain
3. Download certificate to: `C:\Users\YourName\.cloudflared\cert.pem`

### Step 2: Create Named Tunnel

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel create voip-server
```

**Output**:
```
Created tunnel voip-server with id: 12345678-1234-1234-1234-123456789abc
```

**Write down the Tunnel ID** - you'll need it.

### Step 3: Create Tunnel Configuration

**Create file**: `C:\Users\YourName\.cloudflared\config.yml`

```yaml
tunnel: voip-server
credentials-file: C:\Users\YourName\.cloudflared\12345678-1234-1234-1234-123456789abc.json

ingress:
  - hostname: voip.yourdomain.com
    service: http://localhost:9000
  - service: http_status:404
```

**Replace**:
- `voip.yourdomain.com` with your actual domain
- Tunnel ID in credentials-file path

### Step 4: Create DNS Record

**If using your own domain** on Cloudflare:

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel route dns voip-server voip.yourdomain.com
```

**Or manually** in Cloudflare dashboard:
1. Go to DNS settings
2. Add CNAME record:
   - Name: `voip`
   - Target: `12345678-1234-1234-1234-123456789abc.cfargotunnel.com`
   - Proxy: Enabled ✓

### Step 5: Run Tunnel

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel run voip-server
```

**Users connect to**: `wss://voip.yourdomain.com` (no port needed!)

---

## Part 5: Run as Windows Service (Auto-start)

**Install as service** so tunnel starts automatically:

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" service install
```

**Start service**:

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" service start
```

**Check status**:

```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" service status
```

**Now**:
- Tunnel starts automatically on boot
- Runs in background
- Survives reboots

---

## Part 6: Update Client Connection

### Client Configuration Changes

**Old connection** (direct):
```
Server: dadlink.ddns.net
Port: 9000
Protocol: wss://
```

**New connection** (Cloudflare Tunnel):
```
Server: voip.yourdomain.com
Port: (none - use default 443)
Protocol: wss://
```

**Or if using temporary tunnel**:
```
Server: random-name-1234.trycloudflare.com
Port: (none)
Protocol: wss://
```

### Client Code Changes

**If your client code uses**:
```rust
ws://dadlink.ddns.net:9000
```

**Change to**:
```rust
wss://voip.yourdomain.com  // Note: wss:// not ws://, no port
```

---

## Part 7: Testing

### Test Tunnel is Working

```powershell
# Check tunnel status
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel info voip-server

# Should show:
# Status: connected
```

### Test from Browser

**Open**: https://voip.yourdomain.com

**Expected**:
- Connection should work (even from different network)
- May show error if server expects WebSocket upgrade (normal)
- Should NOT show "connection timeout" or "can't reach"

### Test from VoIP Client

1. **Update client** to use `wss://voip.yourdomain.com`
2. **Connect**
3. **Should work** from any network (no CGNAT issues!)

---

## Troubleshooting

### "tunnel login failed"

**Cause**: Need Cloudflare account and domain

**Fix**:
1. Create Cloudflare account
2. Add domain to Cloudflare (or use temporary tunnel)

### "tunnel not connected"

**Check**:
```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel info voip-server
```

**Restart tunnel**:
```powershell
& "C:\Program Files\Cloudflare\cloudflared.exe" service restart
```

### "connection works but no audio"

**Cause**: UDP (port 9001) not supported by Cloudflare Tunnel

**Impact**: Voice falls back to TCP (higher latency)

**Fix**: This is expected. Voice will work but with 30-80ms added latency.

**Better solution**: Use [Tailscale](./TAILSCALE_SETUP.md) instead (supports UDP)

### "random URL keeps changing"

**Cause**: Using quick tunnel mode (`--url`)

**Fix**: Set up persistent named tunnel (Part 4)

---

## Advantages of Cloudflare Tunnel

✅ **Works behind CGNAT** - No public IP needed
✅ **IP changes don't matter** - Tunnel stays connected
✅ **No port forwarding** - No router configuration
✅ **Free** - No cost for TCP traffic
✅ **DDoS protection** - Cloudflare protects your server
✅ **SSL/TLS included** - Automatic HTTPS
✅ **Global CDN** - Low latency worldwide

---

## Disadvantages

❌ **No UDP support** (free tier) - Voice uses TCP fallback
❌ **Requires domain** (for persistent tunnel)
❌ **Extra latency** (~20-50ms added)
❌ **Cloudflare sees all traffic** - Privacy consideration

---

## Voice Quality with TCP Fallback

**UDP (normal)**:
- Latency: ~20-50ms
- Packet loss: Acceptable (voice codecs handle it)
- Jitter: Low

**TCP fallback (Cloudflare Tunnel)**:
- Latency: ~50-130ms (20-80ms added)
- Packet loss: Zero (TCP retransmits)
- Jitter: Higher (TCP buffering)

**Real impact**: Voice still works well, just slightly less responsive. Most users won't notice for PTT voice chat.

---

## Alternative: Tailscale (Better for UDP)

If voice quality is critical, use **Tailscale** instead:
- ✅ Supports UDP natively
- ✅ Lower latency than Cloudflare Tunnel
- ✅ Free for personal use
- ✅ Peer-to-peer when possible

See [TAILSCALE_SETUP.md](./TAILSCALE_SETUP.md) for setup guide.

---

## Quick Command Reference

```powershell
# Install cloudflared
Invoke-WebRequest -Uri "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe" -OutFile "C:\Program Files\Cloudflare\cloudflared.exe"

# Login
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel login

# Create tunnel
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel create voip-server

# Run tunnel (testing)
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel run voip-server

# Install as service
& "C:\Program Files\Cloudflare\cloudflared.exe" service install

# Start service
& "C:\Program Files\Cloudflare\cloudflared.exe" service start

# Check status
& "C:\Program Files\Cloudflare\cloudflared.exe" tunnel info voip-server
```

---

## Next Steps

1. **Install cloudflared** (Part 2)
2. **Test with quick tunnel** (Part 3) to verify it works
3. **Set up persistent tunnel** (Part 4) for production use
4. **Install as service** (Part 5) for auto-start
5. **Update client** to use new URL (Part 6)
6. **Test from external network** (Part 7)

Once working, your VoIP server will be accessible from anywhere, regardless of IP changes or CGNAT!
