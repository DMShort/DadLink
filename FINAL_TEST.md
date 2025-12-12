# Final VoIP Server Test - Internet Access

Your Dynamic DNS is working! Now let's verify clients can connect over the internet.

**Your VoIP Server Address**: `dadlink.ddns.net:9000`

---

## Current Setup Status

✅ **Server Running**: Port 9000 (TCP) and 9001 (UDP)
✅ **Windows Firewall**: Configured
✅ **Dynamic DNS**: dadlink.ddns.net registered and updating
⏳ **DNS Sync**: 1-5 minute delay normal (your IP changes frequently)

---

## Test 1: Local Connection (Same Network)

**Test from your computer**:

1. Open VoIP client
2. **Server**: `192.168.1.111` or `localhost`
3. **Port**: `9000`
4. **Connect**

**Expected**: Should connect successfully (proves server works)

---

## Test 2: Internet Connection (Different Network)

**Test from phone/other computer on different network**:

1. **Use phone hotspot** or friend's network (NOT your home WiFi)
2. Open VoIP client
3. **Server**: `dadlink.ddns.net`
4. **Port**: `9000`
5. **Connect**

**Expected**: Should connect and work

**If fails**:
- Wait 5 minutes (DNS might be updating)
- Verify port forwarding in router
- Check firewall rules

---

## Test 3: Port Forwarding Check

**Verify ports are forwarded correctly**:

```powershell
# Check if TCP 9000 is accessible externally
# This tests from OUTSIDE your network
Test-NetConnection -ComputerName dadlink.ddns.net -Port 9000 -InformationLevel Detailed
```

**Expected output**:
```
ComputerName     : dadlink.ddns.net
RemoteAddress    : 122.150.218.X
TcpTestSucceeded : True
```

**If TcpTestSucceeded = False**:
- Port forwarding not configured in router
- Firewall blocking
- Server not running

---

## Test 4: Voice Quality Test

**Once connected**:

1. Join a channel
2. Press Push-To-Talk
3. Speak
4. Verify voice comes through clearly
5. Check for latency/delay

**Expected**: Clear voice, low latency (<100ms)

---

## Understanding DNS Sync

Your public IP changes very frequently (every few minutes). This is unusual but No-IP handles it:

**Timeline**:
1. Your IP changes: `122.150.218.28` → `122.150.218.35`
2. No-IP detects change (within 5 minutes)
3. DNS updates (takes 1-5 minutes to propagate)
4. Your domain now points to new IP

**During the sync window** (1-5 minutes):
- DNS might point to old IP
- Existing connections stay active
- New connections might fail briefly

**This is normal** and happens with all Dynamic DNS services.

---

## Router Port Forwarding Verification

**Ensure these are configured in your Nighthawk router**:

1. **Login**: http://192.168.1.1
2. **Navigate**: Advanced → Advanced Setup → Port Forwarding
3. **Verify rules exist**:

| Service Name | External Port | Internal IP   | Internal Port | Protocol |
|--------------|---------------|---------------|---------------|----------|
| VoIP-Control | 9000          | 192.168.1.111 | 9000          | TCP      |
| VoIP-Voice   | 9001          | 192.168.1.111 | 9001          | UDP      |

**Both rules must**:
- Point to **192.168.1.111** (your static IP)
- Be **Enabled** ✓

---

## Sharing with Users

**Once tested and working**, share this with users:

```
VoIP Server Connection Info:
- Server Address: dadlink.ddns.net
- Port: 9000
- Download client: [Your distribution link]
- Connection is encrypted (TLS + SRTP)
```

**Note**: Your domain name (dadlink.ddns.net) is permanent. Even when your public IP changes, users always use the same address.

---

## Troubleshooting

### "Connection timeout" when using dadlink.ddns.net

**Check DNS sync**:
```powershell
# Current public IP
curl https://api.ipify.org

# DNS points to
nslookup dadlink.ddns.net
```

**If they don't match**: Wait 5 minutes, then try again.

**If they match but still fails**: Port forwarding issue.

### "Connection refused"

**Possible causes**:
1. Server not running
2. Port forwarding not configured
3. Firewall blocking

**Check server is running**:
```powershell
cd C:\dev\VoIP-System
.\test-connection.ps1
```

Should show all `[OK]`.

### Works locally but not over internet

**This means**:
- Server works ✓
- Port forwarding NOT configured ✗

**Fix**: Configure router port forwarding (see above).

### Voice is choppy or delayed

**Possible causes**:
1. UDP port 9001 not forwarded (falls back to TCP - slower)
2. Network congestion
3. ISP throttling

**Check UDP works**:
- Voice should use UDP (low latency)
- If UDP blocked, falls back to TCP (higher latency)

**Fix**: Ensure UDP 9001 is forwarded in router.

---

## Next Steps

1. **Test locally** (`192.168.1.111:9000`) ✓
2. **Test internet** (`dadlink.ddns.net:9000` from phone hotspot)
3. **If works**: Share with users
4. **If fails**: Check port forwarding in router

---

## Monitoring Your Server

**Check if server is accessible**:

```powershell
# Quick test
Test-NetConnection -ComputerName dadlink.ddns.net -Port 9000
```

**Check current IP vs DNS**:

```powershell
Write-Host "Public IP: $(curl -s https://api.ipify.org)"
Write-Host "DNS Points To: $(nslookup dadlink.ddns.net | Select-String 'Address:' | Select-Object -Last 1)"
```

**Full diagnostic**:

```powershell
cd C:\dev\VoIP-System
.\test-connection.ps1
```

---

## Summary

✅ **Server**: Running on port 9000
✅ **Firewall**: Configured
✅ **Dynamic DNS**: dadlink.ddns.net working
⏳ **Port Forwarding**: Needs verification
⏳ **Internet Test**: Needs testing from external network

**Next step**: Test connection from phone hotspot using `dadlink.ddns.net:9000`
