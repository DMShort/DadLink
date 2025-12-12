# VPS Hosting Comparison - VoIP Server (2024)

Your home hosting won't work due to CGNAT and rapidly changing IP. Here's a complete comparison of VPS options.

---

## Quick Recommendation

**Best overall**: DigitalOcean ($6/month) - Easiest setup, reliable, great documentation

**Cheapest**: Hetzner ($4.50/month) - Good value, European-focused

**Free (if patient)**: Oracle Cloud (complex, capacity issues)

---

## Detailed Comparison

| Provider | Price/Month | RAM | CPU | Storage | Setup Time | Difficulty | Free Trial |
|----------|-------------|-----|-----|---------|------------|------------|------------|
| **DigitalOcean** | $6 | 1 GB | 1 core | 25 GB SSD | 30 min | Easy | $200 credit (60 days) |
| **Hetzner** | €4.15 (~$4.50) | 2 GB | 1 core | 20 GB SSD | 30 min | Easy | None |
| **Vultr** | $6 | 1 GB | 1 core | 25 GB SSD | 30 min | Easy | $100 credit (14 days) |
| **Linode/Akamai** | $5 | 1 GB | 1 core | 25 GB SSD | 30 min | Easy | $100 credit (60 days) |
| **AWS Lightsail** | $5 | 1 GB | 1 core | 40 GB SSD | 45 min | Moderate | 3 months free |
| **Oracle Cloud** | **FREE** | 1 GB* | 2 cores* | 50 GB | 2+ hours | Very Hard | Forever free |

*Oracle: Up to 24GB RAM / 4 cores if you can get ARM instance (usually out of capacity)

---

## Option 1: DigitalOcean (Recommended)

**Price**: $6/month ($0.009/hour)

**Specs**:
- 1 GB RAM
- 1 CPU core
- 25 GB SSD storage
- 1 TB transfer
- Static public IP included

**Pros**:
- ✅ **Easiest setup** (30 minutes start to finish)
- ✅ **Best documentation** (excellent guides)
- ✅ **Reliable** (99.99% uptime)
- ✅ **$200 free credit** for 60 days (try before buy)
- ✅ **Simple interface** (no confusion)
- ✅ **Great support**

**Cons**:
- ❌ Slightly more expensive than Hetzner
- ❌ Only 1 GB RAM (vs Hetzner's 2 GB)

**Best for**: First-time VPS users, want reliability and ease

**Setup guide**: Already created - [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)

---

## Option 2: Hetzner Cloud (Best Value)

**Price**: €4.15/month (~$4.50 USD)

**Specs**:
- 2 GB RAM (double DigitalOcean!)
- 1 CPU core (AMD/Intel)
- 20 GB SSD storage
- 20 TB transfer
- Static public IP included

**Pros**:
- ✅ **Cheapest paid option**
- ✅ **2 GB RAM** (great for PostgreSQL + server)
- ✅ **European data centers** (good for EU/UK users)
- ✅ **Fast SSD**
- ✅ **Excellent performance**

**Cons**:
- ❌ No free trial/credit
- ❌ Primarily European (higher latency for US/Asia)
- ❌ Requires credit card immediately

**Best for**: Budget-conscious, European users, need more RAM

**Locations**: Germany, Finland, USA (Ashburn)

---

## Option 3: Vultr (DigitalOcean Alternative)

**Price**: $6/month

**Specs**:
- 1 GB RAM
- 1 CPU core
- 25 GB SSD storage
- 1 TB transfer

**Pros**:
- ✅ $100 free credit (14 days)
- ✅ More global locations than DO
- ✅ Similar pricing to DigitalOcean
- ✅ Good performance

**Cons**:
- ❌ Interface less polished than DO
- ❌ Documentation not as good
- ❌ Shorter trial period

**Best for**: Need specific location not offered by DO

---

## Option 4: Linode (now Akamai)

**Price**: $5/month

**Specs**:
- 1 GB RAM
- 1 CPU core
- 25 GB SSD storage
- 1 TB transfer

**Pros**:
- ✅ $100 free credit (60 days)
- ✅ $5/month (cheaper than DO)
- ✅ Backed by Akamai (CDN company)
- ✅ Good performance

**Cons**:
- ❌ Recent acquisition (Akamai bought Linode)
- ❌ Interface in transition
- ❌ Less beginner-friendly

**Best for**: Experienced users, want $1 savings vs DO

---

## Option 5: AWS Lightsail (Amazon)

**Price**: $5/month

**Specs**:
- 1 GB RAM
- 1 CPU core
- 40 GB SSD storage
- 2 TB transfer

**Pros**:
- ✅ 3 months free trial
- ✅ More storage (40 GB vs 25 GB)
- ✅ Amazon reliability
- ✅ Easy to upgrade to full AWS later

**Cons**:
- ❌ **AWS interface complexity** (overwhelming)
- ❌ Easy to accidentally use paid services
- ❌ Billing surprises possible
- ❌ Harder to understand pricing

**Best for**: Already familiar with AWS, plan to use other AWS services

---

## Option 6: Oracle Cloud (Free Forever)

**Price**: **FREE** (forever)

**Specs (if you get ARM instance)**:
- Up to 24 GB RAM
- Up to 4 CPU cores
- 50 GB storage
- 10 TB transfer

**Specs (AMD E2.Micro - always available)**:
- 1 GB RAM
- 1 CPU core
- 50 GB storage

**Pros**:
- ✅ **FREE FOREVER** (no trial, actually free)
- ✅ Generous specs (if ARM available)
- ✅ Good for learning

**Cons**:
- ❌ **ARM instances out of capacity** (weeks to get)
- ❌ **Extremely complex setup** (2+ hours, easy to mess up)
- ❌ **Confusing interface** (compartments, VCNs, security lists)
- ❌ **AMD instance only 1 GB RAM** (tight for PostgreSQL)
- ❌ **Poor documentation**

**Best for**: Patient users, willing to spend days retrying for capacity, want free hosting

**Reality**: Most people try Oracle, get frustrated, switch to DigitalOcean.

See guides:
- [ORACLE_CLOUD_SETUP_V2.md](./ORACLE_CLOUD_SETUP_V2.md)
- [ORACLE_CAPACITY_ISSUE.md](./ORACLE_CAPACITY_ISSUE.md)

---

## Feature Comparison

### Global Locations

**Best global coverage**: Vultr (25+ locations)

| Provider | North America | Europe | Asia | Australia |
|----------|---------------|--------|------|-----------|
| DigitalOcean | 5 | 3 | 2 | 1 |
| Hetzner | 1 | 3 | 0 | 0 |
| Vultr | 8 | 5 | 7 | 1 |
| Linode | 4 | 3 | 4 | 1 |
| AWS Lightsail | Many | Many | Many | Many |
| Oracle | 8 | 5 | 7 | 2 |

### Support Quality

**Ranked**:
1. DigitalOcean - Excellent documentation, good support
2. AWS Lightsail - Extensive AWS docs (overwhelming)
3. Linode - Good community
4. Vultr - Decent support
5. Hetzner - Good docs, slower support
6. Oracle - Poor support, confusing docs

### Ease of Setup

**Ranked (easiest to hardest)**:
1. DigitalOcean - 30 min
2. Hetzner - 30 min
3. Vultr - 30 min
4. Linode - 40 min
5. AWS Lightsail - 45 min (navigating AWS console)
6. Oracle - 2+ hours (complex, error-prone)

---

## Cost Calculator (1 Year)

| Provider | Monthly | Yearly Total | Free Trial Value | Net First Year |
|----------|---------|--------------|------------------|----------------|
| DigitalOcean | $6 | $72 | $200 credit (60 days) | **FREE** for 2 months, then $60 |
| Hetzner | €4.15 | €49.80 (~$54) | None | $54 |
| Vultr | $6 | $72 | $100 credit (14 days) | ~$66 |
| Linode | $5 | $60 | $100 credit (60 days) | $30 |
| AWS Lightsail | $5 | $60 | 3 months free | $45 |
| Oracle | $0 | $0 | N/A | **$0** |

**Best value first year**: Oracle (if you get capacity) or DigitalOcean (with free trial)

**Best value long-term**: Oracle (free) or Hetzner ($4.50/month)

---

## Performance Comparison

**Benchmarks** (approximate):

| Provider | CPU Performance | Network Speed | Disk I/O |
|----------|----------------|---------------|----------|
| DigitalOcean | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Hetzner | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Vultr | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Linode | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| AWS Lightsail | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| Oracle (ARM) | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |

**Note**: Oracle ARM is fast but Rust compilation is slower on ARM.

---

## Recommended Choice: DigitalOcean

**Why DigitalOcean**:

1. **$200 free credit** - Try for 60 days completely free
2. **Easiest setup** - 30 minutes to online
3. **Best documentation** - Step-by-step guides for everything
4. **Reliable** - 99.99% uptime
5. **Already have setup guide** - [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)

**After free trial ends**: Pay $6/month or switch to Hetzner for $4.50/month

---

## Alternative: Hetzner (If Budget is Tight)

**Why Hetzner**:

1. **Cheapest** - $4.50/month
2. **More RAM** - 2 GB vs 1 GB (better for PostgreSQL)
3. **Good performance** - Actually better CPU than DO
4. **No trial needed** - Just $4.50 to start

**Downside**: Pay from day 1, no free trial

---

## Sign Up Links

**DigitalOcean**: https://www.digitalocean.com/
- Use referral code for $200 credit: Available via signup promotions

**Hetzner**: https://www.hetzner.com/cloud

**Vultr**: https://www.vultr.com/
- $100 credit via promotions

**Linode**: https://www.linode.com/
- $100 credit via promotions

**AWS Lightsail**: https://aws.amazon.com/lightsail/

**Oracle Cloud**: https://cloud.oracle.com/
- See [ORACLE_CLOUD_SETUP_V2.md](./ORACLE_CLOUD_SETUP_V2.md)

---

## Setup Time Estimate

**DigitalOcean**:
1. Sign up - 5 min
2. Create droplet - 2 min
3. SSH setup - 3 min
4. Install dependencies - 10 min
5. Deploy VoIP server - 10 min
6. Test - 5 min

**Total**: 30-40 minutes

**Oracle Cloud**:
1. Sign up - 15 min
2. Navigate confusing interface - 30 min
3. Create VCN (network) - 20 min
4. Set up security lists - 20 min
5. Create instance (if capacity available) - 10 min
6. Configure everything - 30 min
7. Troubleshoot issues - 1-2 hours

**Total**: 2-4 hours (if lucky with capacity)

---

## My Recommendation

### For Immediate Use

**Go with DigitalOcean**:
- Sign up with $200 credit
- Follow [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)
- Be online in 30 minutes
- Use free for 60 days
- After trial: Either pay $6/month or migrate to Hetzner

### For Long-term Budget Use

**Go with Hetzner**:
- €4.15/month (~$4.50)
- 2 GB RAM (better than DO)
- Pay from day 1, but cheapest option
- Great performance

### If You Have Patience

**Try Oracle Cloud**:
- Set up retry script for ARM instance
- While waiting, use DigitalOcean (free trial)
- If you get Oracle ARM: Migrate and save $72/year
- If no Oracle capacity after 2 weeks: Stay with DO or switch to Hetzner

---

## Quick Decision Matrix

**Choose DigitalOcean if**:
- ✅ Want to be online TODAY
- ✅ First-time VPS user
- ✅ Want easiest setup
- ✅ Okay with $6/month after trial

**Choose Hetzner if**:
- ✅ Want cheapest long-term option
- ✅ Need 2 GB RAM
- ✅ Okay paying from day 1
- ✅ European users (lower latency)

**Choose Oracle if**:
- ✅ Free is absolute requirement
- ✅ Patient (may take weeks for capacity)
- ✅ Willing to deal with complexity
- ✅ Have 2-4 hours for setup

---

## Next Steps

**Option 1: DigitalOcean** (Recommended)

1. Go to: https://www.digitalocean.com/
2. Sign up (get $200 credit)
3. Follow: [DIGITALOCEAN_SETUP.md](./DIGITALOCEAN_SETUP.md)
4. Online in 30 minutes

**Option 2: Hetzner**

1. Go to: https://www.hetzner.com/cloud
2. Sign up
3. I'll create a setup guide for you

**Option 3: Oracle Cloud**

1. Follow: [ORACLE_CLOUD_SETUP_V2.md](./ORACLE_CLOUD_SETUP_V2.md)
2. Be patient with capacity issues
3. See: [ORACLE_CAPACITY_ISSUE.md](./ORACLE_CAPACITY_ISSUE.md)

**Which do you want to use?**
