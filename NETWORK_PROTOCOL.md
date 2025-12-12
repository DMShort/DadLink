# Network Protocol Specification
## Gaming-Focused Multi-Channel VoIP System

---

## Protocol Overview

### Transport Layers

| Layer | Protocol | Purpose | Port |
|-------|----------|---------|------|
| **Control** | WebSocket over TLS 1.3 | Authentication, signaling, state sync | 9000 |
| **Voice** | Custom UDP with AES-256-GCM | Low-latency audio packets | 9001 |

---

## Voice Protocol (UDP)

### Packet Structure

```
┌─────────────────────────────────────────────────────────┐
│                    HEADER (28 bytes)                    │
├───────────────────┬─────────────────────────────────────┤
│ Magic Number      │ 4 bytes │ 0x564F4950 ('VOIP')     │
│ Sequence Number   │ 8 bytes │ Monotonic counter       │
│ Timestamp         │ 8 bytes │ Microseconds (Unix)     │
│ Channel ID        │ 4 bytes │ Target channel          │
│ User ID           │ 4 bytes │ Sender ID               │
├─────────────────────────────────────────────────────────┤
│              ENCRYPTED PAYLOAD (variable)               │
├─────────────────────────────────────────────────────────┤
│ Nonce/IV          │ 12 bytes│ AES-GCM nonce           │
│ Opus Frame        │ N bytes │ Encoded audio (40-80)   │
│ Energy Level      │ 1 byte  │ Amplitude 0-255         │
│ Auth Tag          │ 16 bytes│ AES-GCM authentication  │
└─────────────────────────────────────────────────────────┘
```

### Field Descriptions

**Header (Unencrypted):**
- `Magic Number`: Protocol identifier for validation
- `Sequence Number`: Monotonically increasing, detects loss/reordering
- `Timestamp`: Microsecond precision for jitter calculation
- `Channel ID`: Destination channel for routing
- `User ID`: Sender identification

**Payload (Encrypted with AES-256-GCM):**
- `Nonce/IV`: 96-bit nonce for AES-GCM
- `Opus Frame`: Variable length encoded audio (typically 40-80 bytes for 20ms @ 32kbps)
- `Energy Level`: Voice activity indicator (0=silence, 255=max)
- `Auth Tag`: 128-bit authentication tag for integrity

### Encryption

**Algorithm**: AES-256-GCM  
**Key Exchange**: Negotiated during TLS control handshake  
**Key Rotation**: Every 30 minutes (configurable)

**Per-Packet Process:**
1. Generate random 96-bit nonce
2. Encrypt payload (Opus frame + energy level) with AES-GCM
3. Compute authentication tag
4. Prepend nonce, append auth tag
5. Send UDP packet

### Packet Sizes

| Bitrate | Frame Duration | Opus Size | Total Packet Size |
|---------|----------------|-----------|-------------------|
| 24 kbps | 20ms | ~60 bytes | ~117 bytes |
| 32 kbps | 20ms | ~80 bytes | ~137 bytes |
| 64 kbps | 20ms | ~160 bytes| ~217 bytes |

**Typical**: 32 kbps → ~137 bytes per 20ms = 6.85 KB/s = ~55 kbps total (with overhead)

---

## Control Protocol (WebSocket/TLS)

### Message Format

All control messages are JSON over WebSocket.

**General Structure:**
```json
{
  "type": "message_type",
  "id": "optional_request_id",
  ...message_specific_fields
}
```

### Authentication Flow

**1. Client → Server: Hello**
```json
{
  "type": "hello",
  "version": "1.0",
  "client": "voip-client-cpp/0.1.0"
}
```

**2. Server → Client: Challenge**
```json
{
  "type": "challenge",
  "methods": ["password", "token"],
  "server_version": "1.0"
}
```

**3. Client → Server: Authenticate**
```json
{
  "type": "authenticate",
  "method": "password",
  "username": "pilot_1",
  "password": "hashed_password",
  "org_id": 1
}
```

or with token:
```json
{
  "type": "authenticate",
  "method": "token",
  "token": "jwt_token_here"
}
```

**4. Server → Client: Auth Result**
```json
{
  "type": "auth_result",
  "success": true,
  "user_id": 12345,
  "session_token": "jwt_token",
  "voice_key": "base64_aes_key",
  "permissions": ["join", "speak"],
  "orgs": [
    {"id": 1, "name": "My Org", "tag": "ORG"}
  ]
}
```

### Channel Operations

**Join Channel:**
```json
{
  "type": "join_channel",
  "channel_id": 100,
  "password": null
}
```

**Server Response:**
```json
{
  "type": "channel_joined",
  "channel_id": 100,
  "channel_name": "Command",
  "users": [
    {"id": 1, "name": "User1", "speaking": false},
    {"id": 2, "name": "User2", "speaking": true}
  ]
}
```

**Leave Channel:**
```json
{
  "type": "leave_channel",
  "channel_id": 100
}
```

**Channel State Update (Broadcast to all members):**
```json
{
  "type": "channel_state",
  "channel_id": 100,
  "event": "user_joined",
  "user": {"id": 3, "name": "User3"}
}
```

### User State Updates

**User Speaking (Broadcast):**
```json
{
  "type": "user_speaking",
  "channel_id": 100,
  "user_id": 2,
  "speaking": true
}
```

**User Left (Broadcast):**
```json
{
  "type": "user_left",
  "channel_id": 100,
  "user_id": 2
}
```

### Server-Initiated Messages

**Kick/Ban:**
```json
{
  "type": "kicked",
  "channel_id": 100,
  "reason": "Administrator action"
}
```

**Server Shutdown:**
```json
{
  "type": "server_shutdown",
  "message": "Server restarting in 60 seconds",
  "grace_period_seconds": 60
}
```

---

## Error Handling

### Error Response Format

```json
{
  "type": "error",
  "code": "PERMISSION_DENIED",
  "message": "You do not have permission to speak in this channel",
  "request_id": "abc123"
}
```

### Error Codes

| Code | Meaning | Recovery |
|------|---------|----------|
| `INVALID_AUTH` | Authentication failed | Re-authenticate |
| `PERMISSION_DENIED` | Insufficient permissions | User notification |
| `CHANNEL_NOT_FOUND` | Channel doesn't exist | Remove from local state |
| `CHANNEL_FULL` | Channel at capacity | User notification |
| `RATE_LIMITED` | Too many requests | Backoff and retry |
| `SERVER_ERROR` | Internal server error | Retry with backoff |

---

## Connection Management

### Keepalive

**Client → Server (every 30 seconds):**
```json
{
  "type": "ping",
  "timestamp": 1700000000
}
```

**Server → Client:**
```json
{
  "type": "pong",
  "timestamp": 1700000000,
  "server_time": 1700000001
}
```

### Reconnection

**On Disconnect:**
1. Client attempts reconnection with exponential backoff (1s, 2s, 4s, 8s, max 30s)
2. Re-authenticate with stored session token
3. Rejoin all previously subscribed channels
4. Sync state with server

---

## Quality of Service

### DSCP Markings

**Voice Packets (UDP):**
- DSCP: EF (Expedited Forwarding) = 0x2E
- Priority: Highest
- Use: Voice audio packets

**Control Packets (TLS/TCP):**
- DSCP: AF41 (Assured Forwarding) = 0x22
- Priority: High
- Use: Signaling and control

### Jitter Buffer Configuration

**Client-Side:**
- Target Delay: 20-40ms (configurable)
- Adaptive: Yes (adjust based on observed jitter)
- Maximum Delay: 100ms (discard older packets)

---

## Security Considerations

### Threat Mitigation

**Replay Attacks:**
- Monotonic sequence numbers
- Timestamp validation (discard packets >5s old)

**Man-in-the-Middle:**
- TLS 1.3 for control channel
- Certificate pinning (optional)
- AES-GCM authentication tags for voice

**DDoS:**
- Rate limiting per IP
- Connection limits
- IP blacklisting

**Packet Injection:**
- Authentication tags (AES-GCM)
- Sequence number validation
- User ID validation against session

---

## Protocol Versioning

### Version Negotiation

**Client advertises supported versions:**
```json
{
  "type": "hello",
  "versions": ["1.0", "1.1"]
}
```

**Server selects highest common version:**
```json
{
  "type": "challenge",
  "version": "1.0"
}
```

### Compatibility

- **Breaking changes** require major version increment
- **Backward compatible features** use minor version increment
- Servers MUST support at least previous major version
- Clients SHOULD fall back to older versions

---

## Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| Packet Loss Tolerance | <5% | Opus PLC handles gracefully |
| End-to-End Latency | <150ms | Capture to playback |
| Jitter Tolerance | ±50ms | Adaptive jitter buffer |
| Voice Packet Rate | 50 pps | 20ms frames |
| Control Latency | <100ms | User action to server response |

---

See `ARCHITECTURE.md` for system overview and `CLIENT_DESIGN.md`/`SERVER_DESIGN.md` for implementation details.
