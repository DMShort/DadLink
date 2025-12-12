# VoIP System Security Documentation

## Overview

This document describes the security architecture of the VoIP system, covering encryption, authentication, authorization, and threat model.

## Security Architecture

### 1. Transport Layer Security

#### Control Channel (WebSocket over TLS)

- **Protocol**: WebSocket Secure (WSS) over TLS 1.3
- **Port**: 8443 (configurable)
- **Certificate**: TLS certificate and private key
- **Purpose**: Protects control messages (auth, channel join/leave, permissions)

**Setup:**
```bash
cd server && ./tools/generate_certs.sh
```

This generates:
- `certs/server.crt` - Self-signed certificate
- `certs/server.key` - Private key

**Production:** Replace with certificates from a trusted CA (Let's Encrypt, etc.)

#### Voice Channel (UDP with SRTP)

- **Protocol**: SRTP (Secure Real-time Transport Protocol)
- **Port**: 9000 (configurable)
- **Encryption**: AES-128-GCM
- **Key Exchange**: X25519 Elliptic Curve Diffie-Hellman
- **Key Derivation**: HKDF-SHA256
- **Purpose**: Encrypts voice packets end-to-end

### 2. Authentication

#### Password-Based Authentication

```
Client                          Server
  |                               |
  |---Authenticate(user,pass)---->|
  |                               |--[Argon2id hash verification]
  |<-----AuthResult + JWT---------|
```

**Password Storage:**
- Hashing: Argon2id (OWASP recommended)
- Salt: Unique per user (generated during registration)
- Parameters: Memory=64MB, Iterations=3, Parallelism=1

**Code:**
```rust
// server/src/auth.rs
pub fn hash_password(password: &str) -> Result<String> {
    let salt = SaltString::generate(&mut OsRng);
    let argon2 = Argon2::default();
    let password_hash = argon2.hash_password(password.as_bytes(), &salt)?
        .to_string();
    Ok(password_hash)
}
```

#### JWT Token Authentication

**Token Structure:**
```json
{
  "sub": 123,          // User ID
  "org": 1,            // Organization ID
  "roles": [1, 2],     // Role IDs
  "exp": 1234567890,   // Expiration timestamp
  "iat": 1234564290,   // Issued at
  "jti": "uuid"        // Unique token ID
}
```

**Signing:** HMAC-SHA256 with server secret
**Expiration:** 1 hour (configurable)
**Storage:** Client stores in memory (not localStorage for security)

### 3. Voice Encryption (DTLS-SRTP)

#### Key Exchange Protocol

**Step 1: Ephemeral Key Generation**
```rust
// Both client and server generate ephemeral X25519 keypairs
let secret = EphemeralSecret::random_from_rng(OsRng);  // 32 bytes
let public = PublicKey::from(&secret);                  // 32 bytes
```

**Step 2: Public Key Exchange**
```
Client                                    Server
  |                                         |
  |<-------KeyExchangeInit(Ps)--------------|  Server sends public key
  |                                         |
  |--------KeyExchangeResponse(Pc)--------->|  Client sends public key
  |                                         |
```

**Step 3: Shared Secret Derivation**
```rust
// X25519 Diffie-Hellman
let shared_secret = secret.diffie_hellman(&peer_public);  // 32 bytes

// Both sides compute the SAME shared secret!
// Client: shared_secret = secret_c * public_s
// Server: shared_secret = secret_s * public_c
// Due to ECDH math: secret_c * public_s == secret_s * public_c
```

**Step 4: SRTP Key Derivation (HKDF-SHA256)**
```rust
let hk = Hkdf::<Sha256>::new(None, shared_secret.as_bytes());

// Derive master key (16 bytes for AES-128)
let mut master_key = [0u8; 16];
hk.expand(b"SRTP master key", &mut master_key)?;

// Derive salt (14 bytes)
let mut salt = [0u8; 14];
hk.expand(b"SRTP master salt", &mut salt)?;
```

**Why HKDF?**
- Expands shared secret into multiple cryptographic keys
- Provides cryptographically independent keys for different purposes
- Follows RFC 5764 (DTLS-SRTP) standards

#### SRTP Encryption

**Packet Structure:**
```
┌─────────────┬──────────────┬──────────────┐
│ Sequence    │  Ciphertext  │  Auth Tag    │
│ (4 bytes)   │  (N bytes)   │  (16 bytes)  │
└─────────────┴──────────────┴──────────────┘
     ↑              ↑              ↑
   Plaintext    Encrypted    GCM Tag (MAC)
```

**Nonce Derivation:**
```
Nonce (12 bytes) = Salt (14 bytes truncated to 12) XOR (Sequence || Padding)

┌────────────────────────┐
│ Sequence (4 bytes BE)  │
│ Padding  (8 bytes 0x00)│
└────────────────────────┘
            XOR
┌────────────────────────┐
│ Salt (first 12 bytes)  │
└────────────────────────┘
            =
┌────────────────────────┐
│ Unique Nonce           │
└────────────────────────┘
```

**Why unique nonces?**
AES-GCM requires a unique nonce for each encryption with the same key. Reusing nonces catastrophically breaks security.

**Encryption Process:**
```rust
pub fn encrypt(&self, plaintext: &[u8], sequence: u32) -> Result<Vec<u8>> {
    // 1. Derive unique nonce from sequence number
    let nonce = self.derive_nonce(sequence);

    // 2. Encrypt with AES-128-GCM
    let ciphertext = self.cipher.encrypt(&nonce, plaintext)?;

    // 3. Prepend sequence number (unencrypted)
    let mut result = sequence.to_be_bytes().to_vec();
    result.extend_from_slice(&ciphertext);

    Ok(result)  // [seq(4) | ciphertext | tag(16)]
}
```

**Decryption Process:**
```rust
pub async fn decrypt(&self, encrypted: &[u8]) -> Result<Vec<u8>> {
    // 1. Extract sequence number
    let sequence = u32::from_be_bytes(encrypted[0..4].try_into()?);

    // 2. Check for replay attacks
    if !self.check_replay(sequence).await {
        return Err(VoipError::ReplayAttack);
    }

    // 3. Derive nonce
    let nonce = self.derive_nonce(sequence);

    // 4. Decrypt and verify auth tag
    let plaintext = self.cipher.decrypt(&nonce, &encrypted[4..])?;

    Ok(plaintext)
}
```

#### Replay Protection

**Sliding Window Bitmap (64 packets):**
```
max_seq = 1000

┌─────────────────────────────────────┐
│ Bitmap (64 bits)                    │
│ 1 = received, 0 = not received      │
├─────────────────────────────────────┤
│ Covers: max_seq - 63 to max_seq     │
│ Range:  937 to 1000                 │
└─────────────────────────────────────┘
```

**Algorithm:**
```rust
fn check_replay(&mut self, sequence: u64) -> bool {
    // Reject packets older than window
    if sequence + 64 <= self.max_seq {
        return false;  // Too old
    }

    // Accept new packets (advance window)
    if sequence > self.max_seq {
        let shift = sequence - self.max_seq;
        self.bitmap <<= shift;
        self.max_seq = sequence;
        return true;
    }

    // Check if packet already received
    let offset = self.max_seq - sequence;
    let mask = 1u64 << offset;

    if self.bitmap & mask != 0 {
        return false;  // Duplicate!
    }

    // Mark as received
    self.bitmap |= mask;
    true
}
```

### 4. Authorization (Permissions & ACL)

#### Role-Based Access Control (RBAC)

**Hierarchy:**
```
Organization
  └─ Roles (Admin, Moderator, Member, Guest)
       └─ Permissions (JOIN, SPEAK, WHISPER, MANAGE, KICK, BAN)
            └─ Users
                 └─ Channels (with optional channel-specific ACL)
```

**Permission Flags (Bitflags):**
```rust
bitflags! {
    pub struct Permissions: u32 {
        const JOIN    = 0b0001;      // Can join channel
        const SPEAK   = 0b0010;      // Can transmit voice
        const WHISPER = 0b0100;      // Can use whisper mode
        const MANAGE  = 0b1000;      // Can modify channel settings
        const KICK    = 0b00010000;  // Can kick users
        const BAN     = 0b00100000;  // Can ban users
    }
}
```

**Permission Checking:**
```rust
// 1. Get user's roles
let roles = role_repo.get_user_roles(user_id).await?;

// 2. Get channel ACL
let channel_acl = role_repo.get_channel_acl(channel_id).await?;

// 3. Combine permissions
let mut effective_perms = Permissions::empty();
for role in roles {
    if let Some(acl) = channel_acl.find(|a| a.role_id == role.id) {
        effective_perms |= acl.permissions;  // Channel-specific
    } else {
        effective_perms |= role.permissions;  // Organization-level
    }
}

// 4. Check permission
if !effective_perms.contains(Permissions::SPEAK) {
    return Err(VoipError::PermissionDenied);
}
```

#### Channel Password Protection

**Password Hashing:**
- Same as user passwords (Argon2id)
- Stored in `channels.password_hash` column
- Optional (NULL = no password)

**Verification:**
```rust
// Check if channel has password
if let Some(hash) = channel.password_hash {
    let provided = password.ok_or("Password required")?;
    auth::verify_password(provided, &hash)?;
}
```

### 5. Database Security

#### PostgreSQL Configuration

**Connection:**
```env
DATABASE_URL=postgresql://voip:secure_password@localhost/voip_db
```

**Best Practices:**
- Strong database password (generate with `openssl rand -base64 32`)
- Restrict network access (listen only on localhost)
- Use connection pooling (sqlx PgPool)
- Prepared statements prevent SQL injection

#### Password Storage

**Users Table:**
```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(255) UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,  -- Argon2id hash
    ...
);
```

**Never store plaintext passwords!**

#### SQL Injection Prevention

All queries use parameterized statements:
```rust
// ✅ SAFE: Uses $1 parameter binding
sqlx::query!("SELECT * FROM users WHERE username = $1", username)
    .fetch_one(&pool).await?;

// ❌ UNSAFE: String concatenation
// let query = format!("SELECT * FROM users WHERE username = '{}'", username);
```

### 6. Threat Model

#### In Scope

✅ **Passive Network Eavesdropping**
- **Threat**: Attacker intercepts network traffic
- **Mitigation**: TLS for control channel, SRTP for voice
- **Status**: Protected

✅ **Replay Attacks**
- **Threat**: Attacker captures and replays old voice packets
- **Mitigation**: Sliding window bitmap (64 packets)
- **Status**: Protected

✅ **Man-in-the-Middle (MITM)**
- **Threat**: Attacker intercepts and modifies traffic
- **Mitigation**: TLS for control, authenticated encryption (GCM) for voice
- **Status**: Partially protected (requires proper cert validation)

✅ **Brute Force Password Attacks**
- **Threat**: Attacker tries many passwords
- **Mitigation**: Argon2id slow hashing, account lockout (TODO)
- **Status**: Partially protected

✅ **Unauthorized Channel Access**
- **Threat**: User joins channel without permission
- **Mitigation**: Permission system + optional password
- **Status**: Protected

#### Out of Scope (Current Limitations)

⚠️ **Server Compromise**
- Server can decrypt all voice traffic (trusted intermediary model)
- Future: Consider true E2E encryption where server can't decrypt

⚠️ **Certificate Validation**
- Self-signed certificates not validated by clients
- Future: Implement certificate pinning or use trusted CA

⚠️ **DoS Attacks**
- No rate limiting on UDP voice packets
- No connection limits on WebSocket
- Future: Implement rate limiting and connection quotas

⚠️ **Key Rotation**
- SRTP keys are per-session (rotate on reconnect only)
- Future: Implement periodic re-keying for long sessions

⚠️ **Metadata Protection**
- Voice packet headers (user_id, channel_id) are unencrypted
- Future: Encrypt metadata or use obfuscation

### 7. Security Checklist

#### Deployment

- [ ] Generate strong TLS certificates (not self-signed for production)
- [ ] Use strong JWT secret (min 32 bytes random)
- [ ] Use strong database password
- [ ] Restrict database to localhost or VPN
- [ ] Enable firewall rules (allow only necessary ports)
- [ ] Use HTTPS/WSS in production (no plain HTTP/WS)
- [ ] Regularly update dependencies (check for CVEs)
- [ ] Monitor logs for suspicious activity
- [ ] Implement rate limiting
- [ ] Use reverse proxy (nginx) with additional security headers

#### Development

- [ ] Never commit secrets to git (.env in .gitignore)
- [ ] Use environment variables for secrets
- [ ] Validate all user input
- [ ] Use parameterized SQL queries
- [ ] Keep crypto dependencies updated
- [ ] Review security patches
- [ ] Run security audits (cargo audit)
- [ ] Test encryption/decryption thoroughly
- [ ] Test permission system edge cases

### 8. Cryptographic Standards

#### Algorithms

| Purpose | Algorithm | Key Size | Standard |
|---------|-----------|----------|----------|
| Password Hashing | Argon2id | N/A | RFC 9106 |
| TLS | TLS 1.3 | 2048-bit RSA or 256-bit ECDSA | RFC 8446 |
| Key Exchange | X25519 | 256-bit | RFC 7748 |
| Key Derivation | HKDF-SHA256 | 256-bit | RFC 5869 |
| Voice Encryption | AES-128-GCM | 128-bit | NIST SP 800-38D |
| JWT Signing | HMAC-SHA256 | 256-bit | RFC 7519 |

#### Random Number Generation

All cryptographic randomness uses OS-provided CSPRNG:
```rust
use rand::rngs::OsRng;
let secret = EphemeralSecret::random_from_rng(OsRng);
```

**Never use:**
- `rand::thread_rng()` for cryptographic keys
- Predictable seeds
- Custom PRNG implementations

### 9. Compliance

#### Data Protection

- **GDPR**: User data stored in PostgreSQL, deletable on request
- **Encryption at Rest**: Database encryption recommended for production
- **Right to Erasure**: Implement user deletion endpoint

#### Audit Logging

Currently logs:
- Authentication attempts (success/failure)
- Channel joins/leaves
- Permission changes
- SRTP session establishment

Logs contain:
- Timestamp
- User ID
- Action
- Result (success/error)

**Do NOT log:**
- Passwords (plaintext or hashed)
- Decrypted voice data
- JWT secrets
- SRTP keys

### 10. Incident Response

#### Security Breach Procedure

1. **Detect**: Monitor logs for suspicious activity
2. **Contain**: Revoke compromised JWT tokens, terminate sessions
3. **Investigate**: Review logs, identify attack vector
4. **Remediate**: Patch vulnerability, rotate secrets
5. **Notify**: Inform affected users
6. **Review**: Post-mortem, update security practices

#### Key Rotation

**JWT Secret:**
```bash
# Generate new secret
openssl rand -base64 32 > new_secret.txt

# Update .env
JWT_SECRET=<new_secret>

# Restart server (invalidates all existing tokens)
```

**TLS Certificates:**
```bash
# Generate new certificates
cd server && ./tools/generate_certs.sh

# Restart server
# Clients will need to accept new certificate
```

**SRTP Keys:**
- Automatically rotated on each new session
- No manual intervention required
- Future: Implement mid-session re-keying

### 11. References

- [RFC 5764: DTLS-SRTP](https://datatracker.ietf.org/doc/html/rfc5764)
- [RFC 7748: X25519 Elliptic Curves](https://datatracker.ietf.org/doc/html/rfc7748)
- [RFC 5869: HKDF](https://datatracker.ietf.org/doc/html/rfc5869)
- [RFC 9106: Argon2](https://datatracker.ietf.org/doc/html/rfc9106)
- [NIST SP 800-38D: GCM](https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf)
- [OWASP Password Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html)

### 12. Security Contacts

For security issues:
1. **Do NOT** create public GitHub issues
2. Email security concerns to: [your-security-email]
3. Include: Description, impact, reproduction steps
4. Allow 90 days for patch before public disclosure

---

**Last Updated**: 2025-01-XX
**Version**: 1.0
**Maintainer**: VoIP Security Team
