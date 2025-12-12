# Security Architecture
## Gaming-Focused Multi-Channel VoIP System

---

## Security Principles

1. **Defense in Depth**: Multiple security layers
2. **Least Privilege**: Minimal permissions by default
3. **Zero Trust**: Verify every request
4. **Encryption Everywhere**: In transit and at rest
5. **Audit Everything**: Comprehensive logging

---

## Threat Model

### Assets
- **High Value**: Voice communications, credentials, session tokens
- **Medium Value**: User metadata, channel membership
- **Low Value**: Public org info, server stats

### Threats
1. Network eavesdropping
2. Man-in-the-middle attacks
3. DDoS attacks
4. Credential theft
5. Permission escalation
6. Packet injection
7. Session hijacking

---

## Authentication

### Password Security
- **Algorithm**: Argon2id (64 MB memory, 3 iterations)
- **Salt**: 16 bytes random per password
- **TOTP**: Optional two-factor authentication

### Session Management
- **JWT Tokens**: 1 hour access token, 7 day refresh token
- **Token Storage**: Refresh tokens in Redis
- **Revocation**: Blacklist in Redis with TTL

---

## Encryption

### Control Channel
- **Protocol**: TLS 1.3 only
- **Ciphers**: AES-256-GCM, ChaCha20-Poly1305
- **Certificates**: Let's Encrypt or custom CA

### Voice Channel
- **Algorithm**: AES-256-GCM per-packet
- **Key Exchange**: During TLS handshake
- **Key Rotation**: Every 30 minutes
- **Nonce**: 96-bit random per packet
- **Auth Tag**: 128-bit for integrity

---

## Authorization

### Role-Based Access Control

**Permissions:**
- JOIN: Can join channel
- SPEAK: Can transmit audio
- WHISPER: Can private message
- MANAGE: Can edit channel
- KICK/BAN: Moderation powers

**ACL Evaluation:**
```rust
pub fn check_permission(user: &User, channel: &Channel, perm: Permission) -> Result<()> {
    if user.is_admin { return Ok(()); }
    
    for acl in &channel.acl {
        if user.has_role(acl.role_id) && acl.permissions.contains(perm) {
            return Ok(());
        }
    }
    
    Err(Error::PermissionDenied)
}
```

### Packet Validation
1. Verify user ID matches session
2. Verify user in channel
3. Check speak permission
4. Validate sequence number (anti-replay)
5. Check timestamp (reject old packets >5s)

---

## DDoS Protection

### Rate Limiting
- **Connection Rate**: 10 per minute per IP
- **Voice Packets**: 50 per second per user
- **Control Messages**: 10 per second per user

### Connection Limits
- Max 1000 total connections per server
- Max 5 connections per IP
- Max 1000 users per org
- Max 100 channels per org

### IP Blacklisting
- Automatic after repeated violations
- Manual admin blacklist
- Stored in Redis, synced across servers

---

## Audit Logging

### Events to Log
**Security Events:**
- Authentication (success/failure)
- Permission denials
- Session creation/revocation
- Admin actions

**Operational Events:**
- Channel join/leave
- Org/channel creation
- Role changes
- Configuration changes

### Log Format
```json
{
  "timestamp": "2025-11-20T14:30:00Z",
  "event": "auth_failure",
  "user": "pilot_1",
  "ip": "192.168.1.100",
  "reason": "invalid_password"
}
```

### Retention
- Security logs: 90 days
- Audit logs: 365 days (compliance)
- Operational logs: 30 days

---

## Secrets Management

### Environment Variables
- `JWT_SECRET`: Random 256-bit key
- `DATABASE_PASSWORD`: Strong password
- `TLS_CERT_PATH`: Certificate location
- `ENCRYPTION_KEY`: Voice encryption key

### Best Practices
- Never commit secrets to version control
- Use `.env` files (git-ignored)
- Rotate secrets periodically
- Use secret management systems (Vault) in production

---

## Security Checklist

### Pre-Deployment
- [ ] TLS certificates valid and properly configured
- [ ] Strong JWT secret generated
- [ ] Database credentials secured
- [ ] Firewall rules configured
- [ ] Rate limiting enabled
- [ ] HTTPS enforced for admin API
- [ ] Security headers configured

### Post-Deployment
- [ ] Monitor authentication failures
- [ ] Review audit logs regularly
- [ ] Test DDoS resilience
- [ ] Conduct penetration testing
- [ ] Keep dependencies updated
- [ ] Rotate secrets quarterly

---

## Incident Response

### Compromise Procedures
1. **Immediate**: Revoke all sessions, rotate keys
2. **Investigation**: Review audit logs, identify breach
3. **Containment**: Block malicious IPs, patch vulnerabilities
4. **Communication**: Notify affected users
5. **Recovery**: Restore from backups if needed
6. **Post-Mortem**: Document incident, improve defenses

---

See `ARCHITECTURE.md` for system overview and `DEPLOYMENT_GUIDE.md` for secure deployment practices.
