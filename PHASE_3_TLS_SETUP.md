# Phase 3: TLS/HTTPS - Setup Guide

**Created:** November 27, 2025
**Status:** ✅ Implementation Complete - Ready for Testing

---

## 🎯 Quick Start (5 Minutes)

### 1. Generate TLS Certificates

**Windows PowerShell:**
```powershell
cd c:\dev\VoIP-System\server
.\tools\generate_certs.ps1
```

**Expected Output:**
```
🔐 Generating TLS certificates for development...
   Domain: localhost
   Valid for: 365 days

📝 Using OpenSSL to generate certificates...
   Generating private key...
   Generating self-signed certificate...

✅ Certificate generation complete!

📁 Files created:
   Private key: certs\key.pem
   Certificate: certs\cert.pem

⚠️  WARNING: These are self-signed certificates for DEVELOPMENT ONLY
   Clients will show security warnings when connecting
   For production, use Let's Encrypt or a proper certificate authority
```

### 2. Verify Certificate Files

```powershell
ls certs/
```

**Should show:**
```
cert.pem  (Certificate)
key.pem   (Private key)
```

### 3. Start Server with TLS

```powershell
cd c:\dev\VoIP-System\server
cargo run --release
```

**Look for these logs:**
```
✅ TLS certificates loaded successfully
Starting SECURE WebSocket server (WSS) on 0.0.0.0:9000
✅ WebSocket control server started
```

### 4. Connect Client with TLS

1. Start client application
2. Enter server: `localhost`
3. Enter port: `9000`
4. **✅ CHECK "Use TLS/SSL (wss://)" checkbox**
5. Enter credentials: `demo` / `demo123`
6. Click "Login"

**Client console should show:**
```
Connecting to WebSocket: wss://localhost:9000
```

---

## 🔒 Certificate Details

### What Was Generated

**Certificate:**
- Type: X.509 self-signed
- Algorithm: RSA 2048-bit
- Validity: 365 days
- Domains: localhost, *.localhost, 127.0.0.1

**Private Key:**
- Type: RSA 2048-bit
- Format: PKCS#1 PEM
- Encryption: None (for development)
- Permissions: 600 (owner read/write only)

### View Certificate Information

```powershell
# Windows (if OpenSSL installed)
openssl x509 -in certs/cert.pem -text -noout

# Output shows:
# - Issuer: CN=localhost
# - Subject: CN=localhost
# - Validity: Not Before/After
# - Subject Alt Names: DNS:localhost, IP:127.0.0.1
```

---

## 🛠️ Configuration Options

### Default Paths

The server looks for certificates at:
```
server/certs/cert.pem  (Certificate)
server/certs/key.pem   (Private key)
```

### Custom Paths

Create `server/config/server.yaml`:
```yaml
security:
  tls_cert: "path/to/your/cert.pem"
  tls_key: "path/to/your/key.pem"
  jwt_secret: "your-secret-key"
```

Or use environment variables:
```powershell
$env:VOIP_SECURITY__TLS_CERT="path/to/cert.pem"
$env:VOIP_SECURITY__TLS_KEY="path/to/key.pem"
```

---

## 🧪 Testing Scenarios

### Test 1: Secure Connection (WSS)

1. Ensure certificates are in `certs/` folder
2. Start server: `cargo run --release`
3. Client: **Check TLS checkbox**
4. Connect and login

**Expected:**
- Server log: "Starting SECURE WebSocket server (WSS)"
- Client log: "Connecting to WebSocket: wss://..."
- ✅ Connection succeeds

### Test 2: Insecure Connection (WS)

1. Ensure certificates are in `certs/` folder
2. Start server: `cargo run --release`
3. Client: **Uncheck TLS checkbox**
4. Connect and login

**Expected:**
- Server log: "Starting SECURE WebSocket server (WSS)" (server is running WSS)
- Client log: "Connecting to WebSocket: ws://..."
- ❌ Connection might fail (client using WS, server expecting WSS)

### Test 3: Fallback to Insecure Mode

1. **Delete or rename** `certs/` folder
2. Start server: `cargo run --release`
3. Client: **Uncheck TLS checkbox**
4. Connect and login

**Expected:**
- Server log: "⚠️ Failed to load TLS certificates"
- Server log: "Starting INSECURE WebSocket server (WS)"
- Client log: "Connecting to WebSocket: ws://..."
- ✅ Connection succeeds

### Test 4: Certificate Warnings

1. Start server with self-signed certificates
2. Connect from **different machine** (not localhost)
3. Client: Check TLS checkbox
4. Enter server IP address

**Expected:**
- Browser may show "Certificate not trusted" warning
- Client may show certificate validation error
- This is **normal** for self-signed certificates

**To Accept:**
- Click "Advanced" → "Accept Risk"
- Or import certificate into system trust store

---

## 🐛 Troubleshooting

### "OpenSSL not found" (Windows)

**Solution 1: Install OpenSSL**
```powershell
# Using Chocolatey
choco install openssl

# Or download from:
# https://slproweb.com/products/Win32OpenSSL.html
```

**Solution 2: Use .NET Certificate**
The PowerShell script will automatically fallback to .NET certificate generation if OpenSSL is not available.

### "Certificate file not found"

**Check:**
```powershell
ls certs/
```

**If missing, regenerate:**
```powershell
.\tools\generate_certs.ps1
```

**Verify paths:**
```powershell
# Should exist:
certs\cert.pem
certs\key.pem
```

### "Server still starts in WS mode"

**Check server logs:**
```
⚠️  Failed to load TLS certificates: <error message>
```

**Common causes:**
1. Certificates don't exist → Regenerate
2. Wrong permissions → `chmod 600 certs/key.pem; chmod 644 certs/cert.pem`
3. Corrupted files → Regenerate
4. Wrong paths in config → Check `security.tls_cert` and `security.tls_key`

### "Client can't connect (wss://)"

**Checklist:**
1. Server started with TLS? (check logs for "WSS")
2. TLS checkbox is checked?
3. Correct server address?
4. Firewall allows port 9000?
5. Certificate is valid? (not expired)

**Debug:**
```powershell
# Try without TLS first
# Uncheck TLS checkbox in client
# Should work if basic connectivity is fine
```

### "Certificate expired"

**Regenerate:**
```powershell
rm certs\*
.\tools\generate_certs.ps1
```

**Restart server:**
```powershell
cargo run --release
```

---

## 🚀 Production Deployment

### Option 1: Let's Encrypt (Recommended)

**Free, trusted, auto-renewed certificates**

1. **Install Certbot:**
```bash
# Ubuntu/Debian
sudo apt install certbot

# Windows
choco install certbot
```

2. **Generate Certificate:**
```bash
sudo certbot certonly --standalone -d yourdomain.com
```

3. **Update Config:**
```yaml
security:
  tls_cert: "/etc/letsencrypt/live/yourdomain.com/fullchain.pem"
  tls_key: "/etc/letsencrypt/live/yourdomain.com/privkey.pem"
```

4. **Setup Auto-Renewal:**
```bash
# Test renewal
sudo certbot renew --dry-run

# Add to crontab (runs twice daily)
0 0,12 * * * certbot renew --quiet
```

### Option 2: Commercial Certificate

1. Generate CSR
2. Purchase from CA (Digicert, GlobalSign, etc.)
3. Download certificate files
4. Update config with paths
5. Restart server

### Option 3: Cloud Provider Certificates

**AWS Certificate Manager:**
- Free SSL certificates
- Auto-renewal
- Integration with load balancers

**Azure App Service:**
- Free SSL for custom domains
- Auto-managed certificates

**Cloudflare:**
- Free SSL/TLS
- Automatic certificate management
- Global CDN

---

## 📊 Security Best Practices

### Development

✅ **DO:**
- Use self-signed certificates for local testing
- Accept certificate warnings (you generated them)
- Test both WS and WSS modes
- Keep private keys out of version control

❌ **DON'T:**
- Commit certificates to git
- Use development certificates in production
- Disable certificate validation permanently
- Share private keys

### Production

✅ **DO:**
- Use Let's Encrypt or commercial CA
- Enable auto-renewal
- Monitor certificate expiration
- Use strong key sizes (2048-bit minimum)
- Keep private keys secure (600 permissions)
- Rotate certificates annually

❌ **DON'T:**
- Use self-signed certificates
- Hardcode certificate paths
- Ignore certificate warnings
- Use weak encryption
- Skip certificate validation

---

## 🔍 Verification Commands

### Check Certificate Validity

```bash
# View certificate details
openssl x509 -in certs/cert.pem -text -noout

# Check expiration date
openssl x509 -in certs/cert.pem -noout -dates

# Verify private key matches certificate
openssl x509 -noout -modulus -in certs/cert.pem | openssl md5
openssl rsa -noout -modulus -in certs/key.pem | openssl md5
# Hashes should match
```

### Test TLS Connection

```bash
# Test server TLS handshake
openssl s_client -connect localhost:9000 -servername localhost

# Should show:
# - Certificate chain
# - SSL handshake details
# - Connected successfully
```

### Monitor Certificate Expiration

```bash
# Days until expiration
openssl x509 -in certs/cert.pem -noout -checkend 2592000
# Returns 0 if valid for next 30 days
```

---

## 📚 Additional Resources

### Documentation
- [Rustls Documentation](https://docs.rs/rustls/latest/rustls/)
- [Let's Encrypt Getting Started](https://letsencrypt.org/getting-started/)
- [Qt WebSockets SSL/TLS](https://doc.qt.io/qt-6/qwebsocket.html)

### Tools
- [SSL Labs Server Test](https://www.ssllabs.com/ssltest/)
- [Certificate Decoder](https://www.sslshopper.com/certificate-decoder.html)
- [OpenSSL Commands](https://www.openssl.org/docs/man1.1.1/man1/openssl.html)

---

## ✅ Quick Reference

### Generate Certificates
```powershell
cd server
.\tools\generate_certs.ps1
```

### Start Server (TLS)
```powershell
cargo run --release
# Look for: "Starting SECURE WebSocket server (WSS)"
```

### Start Server (No TLS)
```powershell
# Delete certs/ folder
cargo run --release
# Look for: "Starting INSECURE WebSocket server (WS)"
```

### Connect Client (Secure)
- Check "Use TLS/SSL (wss://)" ✅
- Connect to server

### Connect Client (Insecure)
- Uncheck "Use TLS/SSL (wss://)" ☐
- Connect to server

---

**Phase 3 setup complete! Your VoIP server now supports secure WebSocket connections.** 🔐

Next: Test the implementation, then proceed to **Phase 5: Permissions/ACL**.
