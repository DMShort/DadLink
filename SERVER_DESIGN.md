# Server Design Specification
## Gaming-Focused Multi-Channel VoIP System

---

## Server Architecture

```
┌──────────────────────────────────────────┐
│        NETWORK FRONTEND                  │
│  TLS Listener | UDP Voice Socket         │
└──────────────┬───────────────────────────┘
               ↓
┌──────────────┴───────────────────────────┐
│      ROUTING & VALIDATION                │
│  Permission Check | Member Lookup        │
└──────────────┬───────────────────────────┘
               ↓
┌──────────────┴───────────────────────────┐
│        ORG/TENANT MANAGER                │
│  Orgs | Users | Roles | Channels         │
└──────────────┬───────────────────────────┘
               ↓
┌──────────────┴───────────────────────────┐
│        PERSISTENCE LAYER                 │
│  PostgreSQL | Redis | Config Files       │
└──────────────────────────────────────────┘
```

---

## Core Modules

### 1. Network Frontend

**Responsibilities:**
- Accept TLS control connections
- Receive/send UDP voice packets
- Rate limiting and DDoS protection

```rust
pub struct NetworkFrontend {
    pub async fn start(config: NetworkConfig) -> Result<()>;
    pub async fn accept_control_connection() -> Result<ControlStream>;
    pub async fn receive_voice_packet() -> Result<VoicePacket>;
}
```

### 2. Org/Tenant Manager

**Data Models:**

```rust
pub struct Organization {
    pub id: OrgId,
    pub name: String,
    pub tag: String,
    pub owner_id: UserId,
    pub max_users: u32,
    pub max_channels: u32,
}

pub struct User {
    pub id: UserId,
    pub org_id: OrgId,
    pub username: String,
    pub roles: Vec<RoleId>,
}
```

### 3. Channel & Role Manager

```rust
pub struct Channel {
    pub id: ChannelId,
    pub org_id: OrgId,
    pub parent_id: Option<ChannelId>,
    pub name: String,
    pub acl: Vec<ACLEntry>,
    pub max_users: u32,
}

pub struct Role {
    pub id: RoleId,
    pub name: String,
    pub permissions: Permissions,
}

bitflags! {
    pub struct Permissions: u32 {
        const JOIN = 0b0001;
        const SPEAK = 0b0010;
        const WHISPER = 0b0100;
        const MANAGE = 0b1000;
    }
}
```

### 4. Routing Engine

**Core Algorithm:**

```rust
pub async fn route_voice_packet(&self, packet: VoicePacket) -> Result<()> {
    // 1. Validate sender
    let sender = self.validate_sender(packet.user_id, packet.channel_id).await?;
    
    // 2. Check speak permission
    if !sender.can_speak_in(packet.channel_id) {
        return Err(Error::PermissionDenied);
    }
    
    // 3. Get channel members (from cache)
    let members = self.get_channel_members(packet.channel_id).await?;
    
    // 4. Forward to all except sender
    for member in members {
        if member.id != packet.user_id {
            self.send_voice_packet(member.connection, packet.clone()).await?;
        }
    }
    
    Ok(())
}
```

**Performance:**
- In-memory channel membership cache
- Lock-free packet forwarding
- Target: <5ms routing latency

### 5. Admin API

**Endpoints:**

```
POST   /api/v1/orgs                    Create org
GET    /api/v1/orgs/:id                Get org
PUT    /api/v1/orgs/:id                Update org
DELETE /api/v1/orgs/:id                Delete org

POST   /api/v1/orgs/:id/channels       Create channel
GET    /api/v1/orgs/:id/channels       List channels
PUT    /api/v1/channels/:id            Update channel
DELETE /api/v1/channels/:id            Delete channel

POST   /api/v1/orgs/:id/users          Add user
DELETE /api/v1/orgs/:id/users/:user_id Remove user

GET    /api/v1/orgs/:id/sessions       Active sessions
GET    /api/v1/channels/:id/users      Users in channel
```

**Authentication:**
- JWT tokens
- Admin role required
- Org owners have full control

---

## Database Schema

### PostgreSQL Tables

```sql
CREATE TABLE organizations (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    tag VARCHAR(16) UNIQUE NOT NULL,
    owner_id INTEGER,
    created_at TIMESTAMP DEFAULT NOW(),
    max_users INTEGER DEFAULT 100,
    max_channels INTEGER DEFAULT 50
);

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    org_id INTEGER REFERENCES organizations(id),
    username VARCHAR(64) NOT NULL,
    email VARCHAR(255),
    password_hash VARCHAR(255),
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(org_id, username)
);

CREATE TABLE channels (
    id SERIAL PRIMARY KEY,
    org_id INTEGER REFERENCES organizations(id),
    parent_id INTEGER REFERENCES channels(id),
    name VARCHAR(255) NOT NULL,
    description TEXT,
    password_hash VARCHAR(255),
    max_users INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE roles (
    id SERIAL PRIMARY KEY,
    org_id INTEGER REFERENCES organizations(id),
    name VARCHAR(64) NOT NULL,
    permissions INTEGER NOT NULL,
    UNIQUE(org_id, name)
);

CREATE TABLE user_roles (
    user_id INTEGER REFERENCES users(id),
    role_id INTEGER REFERENCES roles(id),
    PRIMARY KEY (user_id, role_id)
);

CREATE TABLE channel_acl (
    channel_id INTEGER REFERENCES channels(id),
    role_id INTEGER REFERENCES roles(id),
    permissions INTEGER NOT NULL,
    PRIMARY KEY (channel_id, role_id)
);
```

### Redis Cache

```
Key Pattern                      | Purpose
---------------------------------|---------------------------
session:{user_id}                | User session data
channel_members:{channel_id}     | Set of user IDs in channel
org_channels:{org_id}            | Set of channel IDs
user_channels:{user_id}          | Set of subscribed channels
```

---

## Configuration

### server.yaml

```yaml
server:
  bind_address: "0.0.0.0"
  control_port: 9000
  voice_port: 9001
  max_connections: 1000

database:
  url: "postgresql://user:pass@localhost/voip"
  max_connections: 20

redis:
  url: "redis://localhost:6379"
  
security:
  tls_cert: "/path/to/cert.pem"
  tls_key: "/path/to/key.pem"
  jwt_secret: "change_in_production"

audio:
  max_bitrate: 128000
  default_bitrate: 32000

limits:
  max_channels_per_org: 100
  max_users_per_org: 1000
  max_users_per_channel: 50
```

---

## Scalability

### Resource Estimates

**Per 100 Concurrent Users:**
- CPU: ~1 core (routing + overhead)
- Memory: ~500 MB (connections + cache)
- Network: ~5 Mbps (voice only, 32 kbps per user average)

**Target Capacity Per Server:**
- 500-1000 concurrent users
- 50-100 active channels
- ~2-4 cores
- ~4-8 GB RAM

### Horizontal Scaling

```
Load Balancer (HAProxy)
         ↓
    ┌────┴────┐
    ↓         ↓
Server 1   Server 2
    ↓         ↓
    └────┬────┘
         ↓
   PostgreSQL
```

**Org-Based Sharding:**
- Assign orgs to specific servers
- Session affinity via load balancer
- Future: inter-server linking

---

## Monitoring

### Key Metrics

**Server Health:**
- Active connections
- Packets/second
- Routing latency (p50, p95, p99)
- CPU/Memory usage
- Database query time

**Per-Org Metrics:**
- Active users
- Channel occupancy
- Bandwidth usage
- Error rates

### Logging

```
ERROR: Serious issues
WARN:  Potential problems
INFO:  Normal events
DEBUG: Diagnostic info
TRACE: Packet-level (very verbose)
```

---

See `ARCHITECTURE.md` for system overview and `NETWORK_PROTOCOL.md` for packet formats.
