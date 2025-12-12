# Configure Nighthawk Router - Dynamic DNS

Your No-IP hostname **dadlink.ddns.net** is registered, but your router needs to be configured to auto-update it when your public IP changes.

**Current Status**:
- No-IP Hostname: dadlink.ddns.net
- Current Public IP: 122.150.218.107
- DNS Points To: 122.150.218.65 (OLD - needs update)

---

## Step 1: Access Your Nighthawk Router

1. **Open browser**: http://192.168.1.1
2. **Login** with admin credentials
   - Username: admin
   - Password: (your router password)

---

## Step 2: Navigate to Dynamic DNS Settings

**For most Nighthawk routers**:

1. Click: **Advanced** tab (at top)
2. Navigate: **Advanced Setup** → **Dynamic DNS**

**Alternative paths** (if you can't find it):
- Internet → Dynamic DNS
- Settings → Advanced Setup → Dynamic DNS

---

## Step 3: Configure No-IP

**Fill in the form**:

1. **Service Provider**: Select **"No-IP.com"** from dropdown

2. **Enable Dynamic DNS**: ✓ (Check the box)

3. **Host Name**: `dadlink.ddns.net`

4. **User Name / Email**: (Your No-IP account email)

5. **Password / Key**: (Your No-IP account password)

**Click**: **Apply** or **Save**

---

## Step 4: Verify Router DDNS Status

After clicking Apply, you should see a status message:

**Good**:
- "DDNS Status: Registered"
- "Last Update: Success"
- "Last Update Time: [timestamp]"

**Bad**:
- "Update Failed"
- "Authentication Failed"
- "Hostname not found"

---

## Step 5: Force Update (Immediately)

**To update the IP right now**:

1. In the Dynamic DNS settings page
2. Look for: **"Update Now"** button (some models have this)
3. Click it to force immediate update

**Or restart router**:
- Router will update DNS on reboot
- Power cycle or use router's reboot function

---

## Step 6: Verify DNS Updated

**Wait 5 minutes**, then test:

```powershell
# Check DNS resolution
nslookup dadlink.ddns.net
# Should now show: 122.150.218.107 (your current IP)

# Verify it matches your public IP
curl https://api.ipify.org
# Should match the IP from nslookup
```

---

## Troubleshooting

### "I can't find Dynamic DNS settings"

**Nighthawk model-specific locations**:

**R7000 / R7000P**:
- Advanced → Advanced Setup → Dynamic DNS

**RAX40 / RAX50 (AX Series)**:
- Advanced → Advanced Setup → Dynamic DNS

**R8000 / R9000 (X6/X10)**:
- Internet → Dynamic DNS

**Search**: Check router manual for "Dynamic DNS" or "DDNS"

### "Update Failed" or "Authentication Failed"

**Possible causes**:
1. **Wrong credentials**: Double-check No-IP username/password
2. **Hostname not verified**: Check No-IP email - click verification link
3. **Hostname typo**: Must be exactly `dadlink.ddns.net`
4. **Account not activated**: Verify No-IP account is active

**Fix**:
- Login to No-IP website: https://www.noip.com/login
- Verify hostname exists and is active
- Check email for verification link
- Re-enter credentials in router carefully

### "No-IP.com not in dropdown"

**Alternative service providers**:
- DynDNS.org
- ChangeIP.com
- Afraid.org (freedns)

**Or use "Custom" provider** (advanced):
- Server: dynupdate.no-ip.com
- Port: 80
- URL: /nic/update?hostname=dadlink.ddns.net

---

## Alternative: Manual Update Script

**If router doesn't support No-IP**, use Windows auto-updater:

Create file: `C:\no-ip-updater.ps1`

```powershell
# No-IP Auto-Updater for Windows
# Replace these with your actual values
$hostname = "dadlink.ddns.net"
$username = "your-noip-email@example.com"
$password = "your-noip-password"

# Create base64 auth string
$auth = [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes("${username}:${password}"))

while ($true) {
    try {
        $headers = @{
            "Authorization" = "Basic $auth"
        }

        $response = Invoke-WebRequest -Uri "https://dynupdate.no-ip.com/nic/update?hostname=$hostname" -Headers $headers -UseBasicParsing
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

        Write-Host "[$timestamp] No-IP updated: $($response.Content)" -ForegroundColor Green
    } catch {
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$timestamp] Update failed: $_" -ForegroundColor Red
    }

    # Update every 5 minutes
    Start-Sleep -Seconds 300
}
```

**Run on startup** (create scheduled task):

```powershell
$action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-ExecutionPolicy Bypass -WindowStyle Hidden -File C:\no-ip-updater.ps1"
$trigger = New-ScheduledTaskTrigger -AtStartup
Register-ScheduledTask -TaskName "No-IP Updater" -Action $action -Trigger $trigger -RunLevel Highest
```

This keeps your DNS updated even if router doesn't support it.

---

## Summary Checklist

- [ ] Login to router: http://192.168.1.1
- [ ] Navigate to Advanced → Advanced Setup → Dynamic DNS
- [ ] Enable Dynamic DNS
- [ ] Service Provider: No-IP.com
- [ ] Host Name: dadlink.ddns.net
- [ ] Enter No-IP credentials
- [ ] Click Apply
- [ ] Verify status shows "Registered" or "Success"
- [ ] Wait 5 minutes
- [ ] Test: `nslookup dadlink.ddns.net` shows current IP
- [ ] Test client connection: dadlink.ddns.net:9000

---

## Next Steps After Router Configuration

Once DNS is updating correctly:

1. **Test DNS resolution** matches current IP
2. **Update VoIP client** to use `dadlink.ddns.net:9000` instead of IP address
3. **Test connection** from another network (phone hotspot)
4. **Share with users**: They connect to `dadlink.ddns.net:9000`

Your IP can change as much as your ISP wants - No-IP will automatically keep the DNS updated!
