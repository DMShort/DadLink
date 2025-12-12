# Nighthawk Router - Port Forwarding Setup

**CRITICAL**: You must configure port forwarding in your router for internet access to work!

**What you're doing**: Telling your router to send incoming connections on ports 9000 and 9001 to your computer (192.168.1.111).

---

## Step 1: Access Your Nighthawk Router

1. **Open browser**: http://192.168.1.1
2. **Login** with admin credentials
   - Username: `admin`
   - Password: (your router password)

---

## Step 2: Navigate to Port Forwarding

**Most Nighthawk routers** (R7000, R7000P, RAX40, RAX50):

1. Click: **Advanced** tab (at top)
2. Click: **Advanced Setup** (left sidebar)
3. Click: **Port Forwarding / Port Triggering**

**Alternative paths**:
- WAN Setup → Port Forwarding
- Firewall → Port Forwarding
- Internet → Port Forwarding

**Look for**: Section titled "Port Forwarding" or "Virtual Server"

---

## Step 3: Add First Rule (TCP Port 9000)

Click **"Add Custom Service"** or **"Add"** button.

**Fill in the form**:

| Field                    | Value                         |
|--------------------------|-------------------------------|
| Service Name             | `VoIP-Control`                |
| Service Type             | `TCP`                         |
| External Port (Start)    | `9000`                        |
| External Port (End)      | `9000`                        |
| Internal Port (Start)    | `9000`                        |
| Internal Port (End)      | `9000`                        |
| Internal IP Address      | `192.168.1.111`               |

**Click**: **Apply**

---

## Step 4: Add Second Rule (UDP Port 9001)

Click **"Add Custom Service"** or **"Add"** button again.

**Fill in the form**:

| Field                    | Value                         |
|--------------------------|-------------------------------|
| Service Name             | `VoIP-Voice`                  |
| Service Type             | `UDP`                         |
| External Port (Start)    | `9001`                        |
| External Port (End)      | `9001`                        |
| Internal Port (Start)    | `9001`                        |
| Internal Port (End)      | `9001`                        |
| Internal IP Address      | `192.168.1.111`               |

**Click**: **Apply**

---

## Step 5: Verify Rules Are Active

**You should see both rules in the list**:

| Service Name | External Port | Internal IP   | Internal Port | Type | Status  |
|--------------|---------------|---------------|---------------|------|---------|
| VoIP-Control | 9000          | 192.168.1.111 | 9000          | TCP  | Enabled |
| VoIP-Voice   | 9001          | 192.168.1.111 | 9001          | UDP  | Enabled |

**Important**: Both rules must show **"Enabled"** or have a checkmark.

---

## Step 6: Save Changes

1. Look for **"Apply"** or **"Save"** button at top or bottom of page
2. Click it
3. Router may reboot (wait 1-2 minutes)

---

## Step 7: Test Port Forwarding

**After router finishes applying changes**, test:

```powershell
# Wait 2 minutes for router to apply changes

# Test external access
Test-NetConnection -ComputerName dadlink.ddns.net -Port 9000

# Should show:
# TcpTestSucceeded : True
```

**If still fails**: Wait 5 minutes and try again (DNS might be syncing).

---

## Nighthawk Model-Specific Instructions

### Nighthawk R7000 / R7000P

**Path**: Advanced → Advanced Setup → Port Forwarding / Port Triggering

**Form fields**:
- Service Name: VoIP-Control
- Protocol: TCP
- External Starting Port: 9000
- External Ending Port: 9000
- Internal Starting Port: 9000
- Internal Ending Port: 9000
- Internal IP Address: 192.168.1.111
- Enable: ✓

### Nighthawk AX Series (RAX40, RAX50)

**Path**: Advanced → Advanced Setup → Port Forwarding

**Form fields**:
- Service Name: VoIP-Control
- Service Type: TCP
- External Port: 9000
- Internal Port: 9000
- Internal IP: 192.168.1.111

### Nighthawk X6/X10 (R8000, R9000)

**Path**: Internet → Port Forwarding

**Form fields**:
- Application Name: VoIP-Control
- Start Port: 9000
- End Port: 9000
- Server IP Address: 192.168.1.111
- Protocol: TCP

---

## Troubleshooting

### "I can't find Port Forwarding"

**Try these search terms** in router interface:
- "Port Forwarding"
- "Virtual Server"
- "Port Mapping"
- "NAT Forwarding"
- "Firewall Rules"

**Or check your router manual**:
1. Look at bottom of router for model number
2. Google: "[Model Number] port forwarding"
3. Check Netgear support: https://www.netgear.com/support/

### "Rule created but test still fails"

**Possible causes**:

1. **Router hasn't applied changes yet**
   - Wait 2-5 minutes
   - Try rebooting router

2. **DNS hasn't synced**
   - Your IP changes frequently
   - Wait 5 minutes for DNS to update

3. **Firewall still blocking**
   - Run: `.\fix-firewall.ps1` as Administrator
   - Verify Windows Firewall shows rules

4. **Wrong internal IP**
   - Must be `192.168.1.111` (your static IP)
   - Check: `ipconfig | findstr IPv4`

5. **Access point interfering**
   - If hardwired through access point
   - Configure on main router (192.168.1.1), not AP

### "Works locally but not over internet"

**This confirms**:
- Server works ✓
- Port forwarding NOT working ✗

**Double-check**:
- Internal IP is exactly `192.168.1.111`
- Both TCP and UDP rules exist
- Rules are Enabled ✓
- Router applied changes (wait 5 minutes)

### Test shows "TcpTestSucceeded : False"

**This means**: Port forwarding is not working.

**Fix steps**:
1. Verify both rules exist in router
2. Verify internal IP is `192.168.1.111`
3. Reboot router
4. Wait 5 minutes
5. Test again

---

## Quick Commands

**Test port forwarding**:
```powershell
Test-NetConnection -ComputerName dadlink.ddns.net -Port 9000
```

**Check current IP**:
```powershell
ipconfig | findstr IPv4
# Should show: 192.168.1.111
```

**Check if server is running**:
```powershell
cd C:\dev\VoIP-System
.\test-connection.ps1
```

---

## What Happens After Port Forwarding

**Once configured**:

1. User connects to: `dadlink.ddns.net:9000`
2. DNS resolves to your public IP: `122.150.218.X`
3. Connection hits your router
4. Router sees incoming connection on port 9000
5. Router forwards to: `192.168.1.111:9000` (your computer)
6. Your server receives connection
7. User is connected!

**Without port forwarding**:

1. User connects to: `dadlink.ddns.net:9000`
2. DNS resolves to your public IP
3. Connection hits your router
4. Router rejects connection (no rule) ❌
5. User sees: "Connection timeout"

---

## Summary Checklist

- [ ] Login to router: http://192.168.1.1
- [ ] Navigate to Port Forwarding settings
- [ ] Add TCP rule: Port 9000 → 192.168.1.111
- [ ] Add UDP rule: Port 9001 → 192.168.1.111
- [ ] Verify both rules show "Enabled"
- [ ] Click Apply/Save
- [ ] Wait 2-5 minutes for router to apply
- [ ] Test: `Test-NetConnection -ComputerName dadlink.ddns.net -Port 9000`
- [ ] Verify: `TcpTestSucceeded : True`

**Once port forwarding works, you're done!** Users can connect over the internet using `dadlink.ddns.net:9000`.
