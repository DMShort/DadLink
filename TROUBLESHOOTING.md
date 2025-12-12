# VoIP Server - Connection Troubleshooting Guide

## ❌ "Connection Timeout" Error

Users getting **"Connection timeout"** or **"Failed to connect to server"** means the client can't reach your server.

---

## 🔍 Quick Diagnosis

Run this automated test:

```cmd
cd C:\dev\VoIP-System
powershell -ExecutionPolicy Bypass -File test-connection.ps1
```

This will check:
- ✓ Is the server running?
- ✓ Is the port listening?
- ✓ Are firewall rules configured?
- ✓ Can connections reach the server?

---

## 🛠️ Step-by-Step Fixes

### Fix #1: Configure Windows Firewall (Most Common)

**Problem:** Windows Firewall is blocking incoming connections.

**Solution:** Run the firewall configuration script:

```cmd
cd C:\dev\VoIP-System

REM Right-click PowerShell and "Run as Administrator"
powershell -ExecutionPolicy Bypass -File fix-firewall.ps1
```

This opens ports:
- TCP 9000 (WebSocket control)
- UDP 9001 (Voice packets)

**Manual alternative:**
1. Open Windows Firewall with Advanced Security
2. Click "Inbound Rules" → "New Rule"
3. Port → TCP → 9000 → Allow → All profiles → Name: "VoIP WebSocket"
4. Repeat for UDP 9001

---

### Fix #2: Router Port Forwarding

**Problem:** You're behind a router (most home/office networks).

**Check if you need this:**
- Your computer has a local IP (192.168.x.x or 10.x.x.x)
- Users are connecting from outside your network
- Port is open in Windows Firewall but still times out

**Solution:** Configure port forwarding on your router.

#### How to Forward Ports:

1. **Find your router's IP:**
   ```cmd
   ipconfig | findstr "Default Gateway"
   ```
   Usually: `192.168.1.1` or `192.168.0.1`

2. **Open router admin panel:**
   - Open browser: `http://192.168.1.1`
   - Login (usually on sticker on router)

3. **Find port forwarding section:**
   - Look for: "Port Forwarding", "Virtual Server", "NAT"
   - Different for each router brand

4. **Add these rules:**
   ```
   Rule 1:
   - Service Name: VoIP Control
   - External Port: 9000
   - Internal Port: 9000
   - Protocol: TCP
   - Internal IP: <your computer's local IP>

   Rule 2:
   - Service Name: VoIP Voice
   - External Port: 9001
   - Internal Port: 9001
   - Protocol: UDP
   - Internal IP: <your computer's local IP>
   ```

5. **Find your computer's local IP:**
   ```cmd
   ipconfig | findstr "IPv4"
   ```

**Test after forwarding:**
```cmd
powershell -ExecutionPolicy Bypass -File test-connection.ps1 -ServerIP YOUR_PUBLIC_IP
```

---

### Fix #3: TLS Certificate Issues

**Problem:** Self-signed certificate is being rejected.

**Symptoms:**
- Connection works locally (127.0.0.1)
- Times out or "SSL error" from remote clients
- Browser shows "Your connection is not private"

**Quick fix - Development mode:**

The client already has this enabled! Check [login_dialog.cpp:37](client/src/ui/login_dialog.cpp#L37):
```cpp
websocket_->ignoreSslErrors();
```

**Proper fix - Production:**

Get a real SSL certificate:

1. **Free option - Let's Encrypt:**
   ```bash
   # Install certbot
   # For domain: yourdomain.com
   certbot certonly --standalone -d yourdomain.com
   ```

2. **Update server config:**
   ```toml
   # server/config/server.toml
   [security]
   tls_cert = "/etc/letsencrypt/live/yourdomain.com/fullchain.pem"
   tls_key = "/etc/letsencrypt/live/yourdomain.com/privkey.pem"
   ```

3. **Restart server**

**Or use a reverse proxy (nginx):**
```nginx
server {
    listen 443 ssl;
    server_name yourdomain.com;

    ssl_certificate /etc/letsencrypt/live/yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/yourdomain.com/privkey.pem;

    location /control {
        proxy_pass http://localhost:9000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

---

### Fix #4: Server Not Running

**Problem:** Server crashed or never started.

**Check:**
```cmd
tasklist | findstr voip
```

**Start server:**
```cmd
cd C:\dev\VoIP-System\server
cargo run --release
```

**Keep server running:**

Option A - Keep terminal open
Option B - Run as Windows Service (see below)

---

### Fix #5: Port Already in Use

**Problem:** Another program is using port 9000.

**Symptoms:**
```
Error: Address already in use
```

**Check what's using the port:**
```cmd
netstat -ano | findstr :9000
```

**Solutions:**
1. **Kill the other process:**
   ```cmd
   taskkill /PID <process_id> /F
   ```

2. **Change server port:**
   ```toml
   # server/config/server.toml
   [server]
   control_port = 9002  # Changed from 9000
   ```

   Update clients to use new port.

---

## 🧪 Testing Checklist

### Local Testing (Same Computer)
```
✓ Server running: cargo run --release
✓ Client connects to: 127.0.0.1:9000
✓ Should work without firewall/router config
```

### LAN Testing (Same Network)
```
✓ Server running
✓ Find local IP: ipconfig
✓ Client connects to: 192.168.x.x:9000
✓ May need Windows Firewall configured
```

### Internet Testing (Different Networks)
```
✓ Server running
✓ Windows Firewall configured
✓ Router port forwarding configured
✓ Find public IP: curl https://api.ipify.org
✓ Client connects to: YOUR_PUBLIC_IP:9000
```

---

## 🔧 Advanced Diagnostics

### Test with curl
```cmd
REM Should return 400 Bad Request (expected - it's not HTTP)
curl -k https://YOUR_PUBLIC_IP:9000

REM Or test WebSocket:
curl -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" ^
     -H "Sec-WebSocket-Version: 13" ^
     -H "Sec-WebSocket-Key: test" ^
     https://YOUR_PUBLIC_IP:9000/control
```

### Test with telnet
```cmd
REM Enable telnet client (Windows Features)
telnet YOUR_PUBLIC_IP 9000

REM If it connects: "Connection successful"
REM If it fails: "Could not open connection"
```

### Check server logs
```cmd
cd C:\dev\VoIP-System\server
cargo run --release 2>&1 | tee server.log
```

Look for:
- `✅ Database ready`
- `🎧 Voice server listening`
- `🌐 Control server listening`

### Test from outside your network

Use online port checker:
- https://www.yougetsignal.com/tools/open-ports/
- Enter your public IP and port 9000
- Should show "open"

---

## 📞 User-Reported Issues

### "I can connect locally but not over internet"
→ **Router port forwarding not configured**
   - Follow Fix #2 above

### "Connection works for 5 minutes then times out"
→ **Router NAT timeout**
   - Increase NAT timeout on router
   - Or implement keepalive pings (already done in code)

### "Some users can connect, others can't"
→ **Users' firewall or ISP blocking**
   - They need to check THEIR firewall
   - Some ISPs block non-standard ports
   - Try port 443 instead of 9000 (HTTPS standard port)

### "Voice doesn't work but control does"
→ **UDP port 9001 not forwarded**
   - Check UDP forwarding on router
   - Some networks block UDP entirely

---

## 🚀 Production Deployment Tips

### Use a Domain Name
Instead of IP address:
```
Server: voice.yourdomain.com
Port: 9000
```

Benefits:
- Easier to remember
- Can change server without telling everyone
- Proper SSL certificates

Get free domain:
- Freenom (free domains)
- Cloudflare (DNS + DDoS protection)

### Use Standard Ports
Change to HTTPS standard ports:
```toml
[server]
control_port = 443  # Standard HTTPS
voice_port = 443    # Can use same port with different protocol
```

Benefits:
- Less likely to be blocked by firewalls
- No port forwarding needed (usually)

### Run as Windows Service

Keep server running even when you log out:

1. Install NSSM (Non-Sucking Service Manager):
   ```cmd
   choco install nssm
   ```

2. Create service:
   ```cmd
   nssm install VoIPServer "C:\path\to\voip_server.exe"
   nssm set VoIPServer AppDirectory "C:\dev\VoIP-System\server"
   nssm start VoIPServer
   ```

### Use Cloud Hosting

For 24/7 availability:
- **DigitalOcean** ($5/month)
- **AWS EC2** (free tier)
- **Azure VM** (free tier)
- **Heroku** (free tier)

All have public IPs and no port forwarding needed.

---

## 📊 Quick Reference

| Issue | Symptom | Fix |
|-------|---------|-----|
| Firewall | Timeout locally | Run fix-firewall.ps1 |
| Port forward | Timeout internet | Configure router |
| TLS cert | SSL error | Use Let's Encrypt or ignore errors |
| Port conflict | Address in use | Change port or kill process |
| Server down | Connection refused | Start server |
| UDP blocked | No voice | Forward UDP 9001 |

---

## ✅ Verified Working Setup

If everything is configured correctly:

1. **Server shows:**
   ```
   ✅ Database ready
   🎧 Voice server listening on 0.0.0.0:9001
   🌐 Control server listening on 0.0.0.0:9000
   ```

2. **Test script shows:**
   ```
   ✓ TCP Port 9000 is accessible
   ✓ Server process found
   ✓ TCP firewall rule exists
   ✓ UDP firewall rule exists
   ✓ Server is listening on port 9000
   ```

3. **Users can connect from anywhere**

---

**Still having issues?** Run the diagnostic script and share the output:
```cmd
powershell -ExecutionPolicy Bypass -File test-connection.ps1 > diagnosis.txt
```
