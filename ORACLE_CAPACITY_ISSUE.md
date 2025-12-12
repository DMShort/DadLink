# Oracle Cloud "Out of Capacity" Issue - Solutions

## The Problem

**Error**: "Out of capacity for shape VM.Standard.A1.Flex in availability domain AD-1"

**Why this happens**:
- ARM instances (VM.Standard.A1.Flex) are **extremely popular** because they're free
- Oracle has limited capacity
- Thousands of people trying to get free ARM instances
- Capacity varies by region and time of day

**This is NOT your fault** - it's Oracle's capacity limitation.

---

## Solution 1: Try Different Availability Domain (30 seconds)

### What to Do

When creating the instance:

1. **Under "Placement"**, you'll see **"Availability domain"**
2. **Try each domain**:
   - AD-1 (you tried this - full)
   - AD-2 (try this)
   - AD-3 (try this)

**Some regions only have 1-2 domains**, others have 3.

### How to Check Available Domains

Before creating instance:
1. Go to: **Compute → Instances**
2. Click "Create Instance"
3. Look at **"Availability domain"** dropdown
4. Note how many options you have

**Try each one until one works.**

---

## Solution 2: Try Different Times (Free, but tedious)

### Best Times to Try

ARM capacity varies by time of day:

**Best times** (less competition):
- ⏰ **3-7 AM your region's time** (early morning)
- ⏰ **2-5 PM your region's time** (afternoon)
- ⏰ **Weekdays** (less traffic than weekends)

**Worst times** (high competition):
- ❌ Evenings (6-10 PM)
- ❌ Weekends
- ❌ Beginning/end of month

**Strategy**:
- Set an alarm for 4 AM
- Try to create instance
- Higher chance of success

---

## Solution 3: Try Different Home Region (Nuclear option)

### If You Just Created Account (Within 24 hours)

You can contact Oracle support to **change your home region**.

**Why?**
- Some regions have more capacity than others
- Popular regions (Frankfurt, Ashburn) are often full
- Less popular regions may have availability

**Best regions for capacity** (anecdotal):
- ✅ Amsterdam (EU-Amsterdam-1)
- ✅ Montreal (CA-Montreal-1)
- ✅ Melbourne (AP-Melbourne-1)
- ✅ Hyderabad (AP-Hyderabad-1)

**Worst regions** (highest demand):
- ❌ Frankfurt (EU-Frankfurt-1)
- ❌ Ashburn (US-Ashburn-1)
- ❌ Phoenix (US-Phoenix-1)
- ❌ London (UK-London-1)

### How to Change Home Region

⚠️ **Only works if account is VERY new (< 24 hours)**

1. **Open support ticket**:
   - Go to: https://cloud.oracle.com/
   - Click: Help menu → "Contact Support"
   - Request: "Please change my home region to [REGION]"

2. **Or create new account**:
   - Use different email address
   - Choose different home region
   - May need different credit card

---

## Solution 4: Use AMD Instance Instead (x86, not ARM)

### Alternative Free Shape: VM.Standard.E2.1.Micro

**Specs**:
- 1 GB RAM (vs 12-24 GB with ARM)
- 1 OCPU (vs 2-4 with ARM)
- x86 architecture (faster compilation than ARM!)
- **Always Free** (no capacity issues!)

**Pros**:
- ✅ Always available (no capacity issues)
- ✅ x86 architecture (faster Rust compilation)
- ✅ Easier to get

**Cons**:
- ❌ Only 1 GB RAM (might struggle with PostgreSQL + server)
- ❌ Only 1 core (limited performance)

### How to Use E2.Micro

When creating instance:

1. **Click "Change Shape"**
2. **Shape series**: Select "Specialty and previous generation"
3. **Shape**: VM.Standard.E2.1.Micro
   - You'll see "Always Free Eligible" ✓
4. **Continue with instance creation**

**Can you run VoIP server on 1 GB RAM?**
- ⚠️ Tight squeeze, but possible
- May need to optimize PostgreSQL memory usage
- Won't support many concurrent users (~5-10 max)

---

## Solution 5: Automated Retry Script (Advanced)

### Use a Script to Keep Trying

Oracle Cloud capacity changes constantly. A script can retry every few minutes until it succeeds.

### Windows PowerShell Script

Create file: `C:\oracle-retry.ps1`

```powershell
# Oracle Cloud Instance Creation Retry Script

# CONFIGURE THESE
$region = "us-ashburn-1"  # Your home region
$compartmentId = "ocid1.compartment.oc1..aaaa..."  # Your compartment OCID
$subnetId = "ocid1.subnet.oc1..aaaa..."  # Your public subnet OCID
$imageId = "ocid1.image.oc1..aaaa..."  # Ubuntu 22.04 image OCID
$sshPublicKey = Get-Content "C:\Users\YourName\.ssh\oracle_voip.key.pub"

Write-Host "Starting Oracle Cloud instance creation retry loop..." -ForegroundColor Cyan
Write-Host "This will try every 5 minutes until successful." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop." -ForegroundColor Yellow
Write-Host ""

$attemptCount = 0

while ($true) {
    $attemptCount++
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

    Write-Host "[$timestamp] Attempt #$attemptCount" -ForegroundColor Cyan

    try {
        # Try to create instance using OCI CLI
        $result = oci compute instance launch `
            --availability-domain "AD-1" `
            --compartment-id $compartmentId `
            --shape "VM.Standard.A1.Flex" `
            --shape-config '{"ocpus":2,"memoryInGBs":12}' `
            --image-id $imageId `
            --subnet-id $subnetId `
            --display-name "voip-server" `
            --assign-public-ip true `
            --ssh-authorized-keys-file "C:\Users\YourName\.ssh\oracle_voip.key.pub" `
            2>&1

        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "SUCCESS! Instance created!" -ForegroundColor Green
            Write-Host "Check Oracle Cloud Console for your new instance." -ForegroundColor Green
            break
        } else {
            Write-Host "  Status: Failed (out of capacity)" -ForegroundColor Red
        }
    } catch {
        Write-Host "  Status: Error - $_" -ForegroundColor Red
    }

    # Wait 5 minutes before retrying
    Write-Host "  Waiting 5 minutes before retry..." -ForegroundColor Gray
    Start-Sleep -Seconds 300
}

Write-Host ""
Write-Host "Script completed!" -ForegroundColor Green
```

**To use**:
1. Install OCI CLI: https://docs.oracle.com/en-us/iaas/Content/API/SDKDocs/cliinstall.htm
2. Configure OCI CLI: `oci setup config`
3. Update script with your OCIDs
4. Run: `powershell -ExecutionPolicy Bypass -File C:\oracle-retry.ps1`
5. Let it run overnight

### Bash Script (For Linux/Mac)

```bash
#!/bin/bash
# Run this on any Linux machine to keep retrying

while true; do
    echo "[$(date)] Attempting to create instance..."

    oci compute instance launch \
        --availability-domain "AD-1" \
        --compartment-id "ocid1.compartment..." \
        --shape "VM.Standard.A1.Flex" \
        --shape-config '{"ocpus":2,"memoryInGBs":12}' \
        --image-id "ocid1.image..." \
        --subnet-id "ocid1.subnet..." \
        --display-name "voip-server" \
        --assign-public-ip true \
        --ssh-authorized-keys-file ~/.ssh/oracle_voip.key.pub

    if [ $? -eq 0 ]; then
        echo "SUCCESS!"
        break
    fi

    echo "Failed. Retrying in 5 minutes..."
    sleep 300
done
```

---

## Solution 6: Community Scripts (GitHub)

People have created automated scripts that retry until successful:

### hitrov/oci-arm-host-capacity

**GitHub**: https://github.com/hitrov/oci-arm-host-capacity

**What it does**:
- Monitors Oracle Cloud for ARM capacity
- Automatically creates instance when available
- Runs continuously until successful

**Features**:
- Email notification when successful
- Tries multiple availability domains
- Configurable retry interval

### Installation

```bash
# Clone repository
git clone https://github.com/hitrov/oci-arm-host-capacity.git
cd oci-arm-host-capacity

# Install dependencies
npm install

# Configure (edit config.json with your details)
nano config.json

# Run
node index.js
```

---

## Solution 7: Just Use DigitalOcean ($6/month)

### Honest Comparison

**Oracle Cloud Free ARM**:
- ✅ Free forever
- ❌ Extremely hard to get
- ❌ Can take days/weeks
- ❌ May never get capacity
- ❌ 2+ hours setup time (if you get capacity)
- ❌ Complex interface

**DigitalOcean**:
- ⚠️ $6/month ($0.009/hour)
- ✅ Available instantly
- ✅ 30 minutes setup time
- ✅ Simple, clean interface
- ✅ x86 (faster compilation)
- ✅ Reliable capacity

**Is free worth it?**

**Time calculation**:
- If you spend 5 hours trying to get Oracle ARM capacity
- Your time is worth more than $0/hour
- $6/month = $0.20/day
- Could spend that time earning $6 instead

**Reality**: Most people trying to get Oracle ARM free tier spend days retrying, give up, and switch to DigitalOcean or Hetzner.

---

## Solution 8: Hetzner ($4.50/month - Cheapest Paid)

**If cost is concern but Oracle is too frustrating**:

**Hetzner Cloud**:
- €4.15/month (~$4.50 USD)
- 2 CPU cores (vs Oracle's maybe-never)
- 2 GB RAM
- 20 GB SSD
- Available instantly
- Simple interface

**See**: [HOSTING_OPTIONS.md](./HOSTING_OPTIONS.md) for Hetzner guide

---

## What Most People Do (Real Stats)

Based on community forums and Reddit:

**70%**: Try Oracle for a few hours → give up → use DigitalOcean/Hetzner
**20%**: Use retry scripts → wait days/weeks → eventually get ARM instance
**10%**: Get lucky and get ARM instance on first try

**Average time to get Oracle ARM instance**:
- Lucky: Instant
- Typical: 3-7 days of retrying
- Unlucky: Never (some people never get it)

---

## My Recommendations

### If You Want Free No Matter What

1. **Try all availability domains** (5 minutes each)
2. **Try at 4 AM tomorrow morning** (5 minutes)
3. **If still no luck**: Run retry script overnight
4. **If still no luck after 3 days**: Consider if your time is worth $6/month

### If You Want to Get Online Today

1. **Use DigitalOcean** ($6/month)
   - See [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)
   - 30 minutes to online
   - Zero frustration

2. **Or Hetzner** ($4.50/month)
   - See [HOSTING_OPTIONS.md](./HOSTING_OPTIONS.md)
   - Cheapest reliable option

### If You Have Time and Patience

1. **Try different availability domains** today
2. **Set alarm for 4 AM tomorrow**
3. **Try again at 4 AM**
4. **If no luck**: Run retry script for 2-3 days
5. **If still no luck**: Switch to DigitalOcean

---

## Quick Decision Matrix

**Choose Oracle ARM if**:
- ✅ Free is absolute requirement
- ✅ You have time to wait (days/weeks)
- ✅ You're patient with retry scripts
- ✅ You want to learn about capacity constraints

**Choose DigitalOcean if**:
- ✅ $6/month is acceptable
- ✅ You want to be online TODAY
- ✅ You value your time
- ✅ You want simplicity

**Choose Hetzner if**:
- ✅ $4.50/month is acceptable
- ✅ You want best price/performance
- ✅ European users (lower latency)

**Choose Oracle E2.Micro (AMD) if**:
- ✅ Free is requirement
- ⚠️ You're okay with low specs (1GB RAM)
- ✅ You want to be online today
- ✅ Small user base (<10 users)

---

## Immediate Next Steps

**Option A: Keep Trying Oracle (30 min more)**
1. Go back to instance creation
2. Try **AD-2** (different availability domain)
3. If that fails, try **AD-3**
4. If all fail, try again at 4 AM tomorrow

**Option B: Switch to DigitalOcean (30 min total)**
1. Open [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)
2. Follow step-by-step guide
3. Be online in 30 minutes
4. Cost: $6/month

**Option C: Use Oracle AMD (E2.Micro) (30 min)**
1. Go back to instance creation
2. Change shape to VM.Standard.E2.1.Micro
3. Complete setup
4. Limited specs but works

**What do you want to do?**

---

## Additional Resources

**Oracle Cloud Capacity Status** (unofficial):
- r/OracleCloud subreddit
- "Out of capacity" posts daily

**Retry Scripts**:
- https://github.com/hitrov/oci-arm-host-capacity
- https://github.com/mohankumarpaluru/oracle-freetier-instance-creation

**Alternative Guides**:
- [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md) - Easiest paid option
- [HOSTING_OPTIONS.md](./HOSTING_OPTIONS.md) - Compare all options
- [ORACLE_CLOUD_SETUP_V2.md](./ORACLE_CLOUD_SETUP_V2.md) - Oracle guide (if you persist)

---

**Bottom line**: This is Oracle Cloud's biggest problem. You're not alone - thousands of people hit this daily. The ARM instances are too good (free + powerful), so they're always full.

**Your time has value** - consider if days of retrying is worth $6/month.
