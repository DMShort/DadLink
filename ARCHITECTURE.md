# System Architecture
## Gaming-Focused Multi-Channel VoIP System

---

## Overview

This document describes the high-level system architecture, component relationships, and key design decisions.

---

## System Layers

```
┌─────────────────────────────────────────────────────────────┐
│                   PRESENTATION LAYER                         │
│  • Desktop UI (Qt)                                          │
│  • In-Game Overlay                                          │
│  • Configuration Interface                                  │
└─────────────────────────────────────────────────────────────┘
                           ↕
┌─────────────────────────────────────────────────────────────┐
│                  APPLICATION LAYER                           │
│  • Channel Router & Mixer                                   │
│  • Hotkey Manager                                           │
│  • Config Manager                                           │
│  • Session Manager                                          │
└─────────────────────────────────────────────────────────────┘
                           ↕
┌─────────────────────────────────────────────────────────────┐
│                   AUDIO ENGINE LAYER                         │
│  • Audio Capture (Microphone)                               │
│  • Audio Playback (Speakers)                                │
│  • Device Management                                        │
└─────────────────────────────────────────────────────────────┘
                           ↕
┌─────────────────────────────────────────────────────────────┐
│                  CODEC & NETWORK LAYER                       │
│  • Opus Encoder/Decoder                                     │
│  • UDP Voice Transport                                      │
│  • TLS Control Channel                                      │
│  • Jitter Buffer                                            │
└─────────────────────────────────────────────────────────────┘
```

---

## Client-Server Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                        CLIENTS                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Desktop  │  │  Mobile  │  │   Web    │  │ Overlay  │   │
│  │ (Win/Lin)│  │(iOS/And) │  │(WebRTC)  │  │(In-game) │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
└───────┼─────────────┼─────────────┼─────────────┼──────────┘
        │             │             │             │
        └─────────────┴─────────────┴─────────────┘
                      │
        ┌─────────────┴─────────────┐
        │   NETWORK TRANSPORT       │
        │  ┌────────┐  ┌─────────┐ │
        │  │  UDP   │  │   TLS   │ │
        │  │ Voice  │  │ Control │ │
        │  └────────┘  └─────────┘ │
        └─────────────┬─────────────┘
                      │
┌─────────────────────┴────────────────────────────────────────┐
│                      SERVER CLUSTER                          │
│  ┌────────────────────────────────────────────────────────┐ │
│  │               VOIP SERVER INSTANCES                    │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐            │ │
│  │  │ Server 1 │  │ Server 2 │  │ Server N │            │ │
│  │  └──────────┘  └──────────┘  └──────────┘            │ │
│  └────────────────────────────────────────────────────────┘ │
│                           │                                  │
│  ┌────────────────────────┴─────────────────────────────┐  │
│  │              SHARED PERSISTENCE LAYER                 │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │  │
│  │  │ PostgreSQL   │  │    Redis     │  │   S3/Blob  │ │  │
│  │  │   (Orgs,     │  │   (Cache,    │  │  (Config   │ │  │
│  │  │   Channels)  │  │   Sessions)  │  │   Backup)  │ │  │
│  │  └──────────────┘  └──────────────┘  └────────────┘ │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### Client Components

#### 1. Audio Engine
- **Responsibility**: Hardware audio I/O
- **Technology**: PortAudio (C++) or cpal (Rust)
- **Threading**: Dedicated real-time threads
- **Key Requirement**: <10ms capture/playback latency

#### 2. Codec Layer
- **Responsibility**: Audio compression/decompression
- **Technology**: libopus (20ms frames, 24-64 kbps)
- **Features**: PLC, DTX, FEC
- **Performance**: <2ms encode/decode time

#### 3. Network Transport
- **UDP Voice**: Low-latency, encrypted packets
- **TLS Control**: Reliable signaling via WebSocket
- **Jitter Buffer**: 20-40ms adaptive buffering
- **Encryption**: AES-256-GCM per packet

#### 4. Channel Router & Mixer
- **Multi-Channel Mixing**: Combine up to 10+ simultaneous channels
- **Audio Ducking**: Priority-based volume reduction
- **Per-Channel Controls**: Volume, pan, mute, monitor-only

#### 5. Hotkey Manager
- **Global Hotkeys**: OS-level key interception
- **Per-Channel PTT**: Independent push-to-talk per channel
- **Simulcast**: Multi-channel transmission

#### 6. UI Layer
- **Radio Stack View**: Visual channel list with active speakers
- **Settings**: Device, hotkey, server configuration
- **Overlay**: Minimal in-game display

### Server Components

#### 1. Network Frontend
- **TLS Listener**: Control connection acceptance
- **UDP Socket**: Voice packet receiver
- **Rate Limiting**: DDoS protection
- **Connection Pool**: Manage thousands of concurrent connections

#### 2. Org/Tenant Manager
- **Multi-Tenancy**: Isolated organizations per server
- **Resource Quotas**: Per-org user/channel limits
- **User Management**: Authentication, roles, permissions

#### 3. Channel Manager
- **Hierarchical Channels**: Tree structure with parent-child relationships
- **ACL System**: Role-based permissions (join, speak, manage)
- **Operation Presets**: Pre-configured channel layouts

#### 4. Routing Engine
- **Packet Forwarding**: Pure forwarding (no decoding/mixing)
- **Permission Validation**: Check ACLs per packet
- **Membership Tracking**: In-memory cache for fast lookups
- **Performance Target**: <5ms routing latency

#### 5. Admin API
- **REST/gRPC**: HTTP-based management interface
- **CRUD Operations**: Orgs, channels, roles, users
- **Monitoring**: Session stats, channel occupancy
- **Authentication**: JWT tokens, role-based access

#### 6. Persistence Layer
- **PostgreSQL**: Org structure, users, channels, roles
- **Redis**: Session cache, channel membership
- **File Storage**: Operation presets, configuration backups

---

## Data Flow Diagrams

### Voice Transmission Flow

```
[User Presses PTT]
       ↓
[Hotkey Manager] → Triggers capture for channel
       ↓
[Audio Engine] → Captures 10ms of audio (480 samples @ 48kHz)
       ↓
[Lock-Free Queue] → Passes PCM to encoding thread
       ↓
[Opus Encoder] → Encodes to ~40-80 bytes (1-2ms)
       ↓
[Packet Builder] → Adds header (seq, timestamp, channel, user)
       ↓
[AES Encryptor] → Encrypts payload
       ↓
[UDP Socket] → Sends packet to server
       ↓
═══════════════════════════════════════════════════════════
       ↓
[SERVER: UDP Receive] → Packet arrives
       ↓
[Permission Check] → Validate user can speak in channel
       ↓
[Routing Engine] → Get all channel members
       ↓
[UDP Send (broadcast)] → Forward to all members except sender
       ↓
═══════════════════════════════════════════════════════════
       ↓
[RECEIVER: UDP Receive] → Packet arrives
       ↓
[AES Decryptor] → Decrypt payload
       ↓
[Jitter Buffer] → Reorder and buffer (20-40ms)
       ↓
[Opus Decoder] → Decode to PCM (1-2ms)
       ↓
[Channel Mixer] → Mix with other active channels
       ↓
[Audio Engine] → Output to speakers
       ↓
[User Hears Audio]
```

**Total Latency: ~70-110ms** (within <150ms target)

### Control Message Flow

```
[User Action] (e.g., Join Channel)
       ↓
[UI Event] → Button click or menu selection
       ↓
[Session Manager] → Creates JOIN_CHANNEL message
       ↓
[WebSocket] → Sends JSON over TLS
       ↓
═══════════════════════════════════════════════════════════
       ↓
[SERVER: WebSocket Handler] → Receives message
       ↓
[Authentication] → Validate JWT token
       ↓
[Channel Manager] → Check channel exists and permissions
       ↓
[Membership Update] → Add user to channel member list
       ↓
[Event Broadcast] → Send USER_JOINED to all channel members
       ↓
[Response] → Send CHANNEL_STATE to joining user
       ↓
═══════════════════════════════════════════════════════════
       ↓
[CLIENT: WebSocket Handler] → Receives CHANNEL_STATE
       ↓
[Session Manager] → Updates local channel state
       ↓
[UI Update] → Display users in channel
       ↓
[User Sees Update]
```

---

## Threading Model

### Client Threads

| Thread | Priority | Purpose | RT Constraints |
|--------|----------|---------|----------------|
| **Main/UI** | Normal | Qt event loop, UI updates | None |
| **Audio Capture** | Real-Time | Microphone input | NO allocation, NO blocking |
| **Audio Playback** | Real-Time | Speaker output | NO allocation, NO blocking |
| **Encoding** | High | Opus encoding, packet sending | Minimal allocation |
| **Decoding Pool** | High | Packet receive, Opus decoding | Minimal allocation |
| **Network I/O** | Normal | TLS control messages | Standard I/O |
| **Mixer** | High | Multi-channel audio mixing | Pre-allocated buffers |

**Thread Communication:**
- Lock-free ring buffers (SPSC queues) between audio threads
- Standard mutexes acceptable for non-RT threads
- Atomic flags for state coordination

### Server Threads

| Component | Threading Model | Concurrency |
|-----------|----------------|-------------|
| **UDP Voice** | Tokio async (thread pool) | Thousands of connections |
| **TLS Control** | Tokio async (WebSocket) | Per-connection tasks |
| **Routing Engine** | Lock-free message passing | Shared-nothing per channel |
| **Database** | Connection pool (pgpool) | Configurable pool size |
| **Admin API** | Axum handlers (async) | Request-based tasks |

---

## Scalability Design

### Horizontal Scaling

**Load Balancing:**
```
                     ┌─────────────┐
                     │ Load        │
   Clients ────────→ │ Balancer    │
                     │ (HAProxy)   │
                     └──────┬──────┘
                            │
        ┌───────────────────┼───────────────────┐
        ↓                   ↓                   ↓
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│ VoIP Server  │    │ VoIP Server  │    │ VoIP Server  │
│   Instance   │    │   Instance   │    │   Instance   │
│      #1      │    │      #2      │    │      #3      │
└──────┬───────┘    └──────┬───────┘    └──────┬───────┘
       │                   │                   │
       └───────────────────┴───────────────────┘
                           │
                  ┌────────┴─────────┐
                  │   PostgreSQL     │
                  │   (Primary +     │
                  │   Replicas)      │
                  └──────────────────┘
```

**Org-Based Sharding:**
- Each org assigned to specific server instance
- Inter-server communication for cross-org features (future)
- Session affinity via load balancer

**Resource Limits Per Server:**
- Target: 500-1000 concurrent users
- Expected: ~50-100 active channels
- Network: ~5-10 Mbps per 100 users (voice only)
- CPU: ~2-4 cores for routing + encoding overhead
- Memory: ~4-8 GB for 1000 users

### Vertical Scaling

**Single Server Optimization:**
- Zero-copy packet forwarding where possible
- In-memory channel membership cache
- Connection pooling for database
- Async I/O for all network operations
- SIMD for any audio processing on server (if needed)

---

## Fault Tolerance

### Client Resilience

**Network Interruption:**
- Automatic reconnection with exponential backoff
- Maintain local state during disconnection
- Rejoin previous channels on reconnect

**Audio Device Failure:**
- Graceful fallback to system default device
- User notification of device loss
- Hot-plug detection and recovery

**Packet Loss:**
- Opus PLC (Packet Loss Concealment)
- Jitter buffer adaptation
- FEC (Forward Error Correction) when enabled

### Server Resilience

**Process Crashes:**
- Systemd/Docker auto-restart
- Health checks and monitoring
- Graceful shutdown (finish in-flight packets)

**Database Unavailability:**
- Redis cache continues serving session data
- Reject new connections until DB restored
- Existing sessions continue (cached membership)

**Network Partitions:**
- Detect via keepalive timeouts
- Clean up stale sessions
- Client auto-reconnect handles recovery

---

## Security Architecture

### Threat Model

**Threats:**
1. Eavesdropping on voice communications
2. Unauthorized access to channels
3. DDoS attacks on server
4. Injection of malicious packets
5. Session hijacking

**Mitigations:**
1. End-to-end encryption (TLS + AES-GCM)
2. Role-based access control (ACL system)
3. Rate limiting, IP blacklisting
4. Packet authentication (HMAC)
5. JWT with short expiration + refresh tokens

### Encryption

**Control Channel:**
- TLS 1.3 for all WebSocket connections
- Certificate pinning (optional)
- JWT tokens (HS256 or RS256)

**Voice Channel:**
- Per-packet AES-256-GCM encryption
- Shared key negotiated during control handshake
- Key rotation every N minutes (configurable)

**At-Rest:**
- Password hashing: Argon2id
- Config encryption: AES-256 for sensitive fields
- Database TLS connections

---

## Configuration Management

### Client Configuration

**Files:**
- `config.json` - Main configuration
- `servers.json` - Saved server list
- `operations/*.json` - Operation presets

**Hot Reload:**
- Audio settings require restart
- Hotkeys can be updated live
- Channel settings update immediately

### Server Configuration

**Files:**
- `server.yaml` - Main server config
- `orgs/*.yaml` - Per-org configurations (optional)

**Example `server.yaml`:**
```yaml
server:
  bind_address: "0.0.0.0"
  control_port: 9000
  voice_port: 9001
  max_connections: 1000

database:
  url: "postgresql://user:pass@localhost/voip"
  max_connections: 20

security:
  tls_cert: "/path/to/cert.pem"
  tls_key: "/path/to/key.pem"
  jwt_secret: "change_me_in_production"

audio:
  max_bitrate: 128000  # 128 kbps
  default_bitrate: 32000  # 32 kbps

limits:
  max_channels_per_org: 100
  max_users_per_org: 1000
  max_users_per_channel: 50
```

---

## Monitoring & Observability

### Client Metrics

- Audio device latency and buffer underruns
- Packet loss rate per channel
- Network RTT (round-trip time)
- CPU usage by component
- Memory usage

### Server Metrics

- Active connections
- Packets forwarded per second
- Routing latency (p50, p95, p99)
- Database query performance
- Channel occupancy
- Per-org resource usage

### Logging

**Client Logs:**
- Audio device events
- Connection state changes
- Channel join/leave
- Errors and warnings

**Server Logs:**
- Connection accept/close
- Authentication failures
- Permission denials
- Routing errors
- Admin API access

**Log Levels:**
- ERROR: Serious issues requiring attention
- WARN: Potential problems
- INFO: Normal operation events
- DEBUG: Detailed diagnostic info
- TRACE: Very verbose (audio callbacks, packet flow)

---

## Technology Stack Summary

### Client Stack (Recommended: C++)

| Component | Technology | Rationale |
|-----------|-----------|-----------|
| Language | C++ | Low-level control, real-time performance |
| Audio I/O | PortAudio | Cross-platform, mature, low-latency |
| Codec | libopus | Industry standard, excellent quality |
| UI | Qt 6 | Mature, cross-platform desktop framework |
| Networking | Asio | Async I/O, good performance |
| Crypto | OpenSSL | TLS, AES encryption |
| Config | nlohmann/json | Modern C++ JSON library |

**Alternative:** Rust core + C++ UI (better memory safety, steeper learning curve)

### Server Stack (Recommended: Rust)

| Component | Technology | Rationale |
|-----------|-----------|-----------|
| Language | Rust | Memory safety, performance, async ecosystem |
| Runtime | Tokio | Mature async runtime, excellent network performance |
| Web Framework | Axum | Type-safe, built on Tokio |
| Database | PostgreSQL | Robust, feature-rich, good performance |
| ORM | SQLx | Compile-time checked queries |
| Cache | Redis | Fast in-memory cache for sessions |
| Config | config-rs | YAML/TOML support |

**Alternative:** Go (simpler, faster development, slightly lower performance)

---

## Next Steps

1. Review and approve architecture decisions
2. Set up development environment
3. Create detailed design docs for each module
4. Implement MVP core components
5. Establish CI/CD pipeline
6. Begin iterative development

See related documents:
- `CLIENT_DESIGN.md` - Client implementation details
- `SERVER_DESIGN.md` - Server implementation details
- `NETWORK_PROTOCOL.md` - Protocol specifications
- `TESTING_STRATEGY.md` - Testing approach
