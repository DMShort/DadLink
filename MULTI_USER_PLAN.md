# Multi-User Support & Authentication Plan

## ✅ What We Already Have (Complete!)

### Server-Side Authentication ✅
**Location**: `server/src/auth.rs`

**Features Implemented:**
- ✅ **Argon2id password hashing** - Industry-standard secure hashing
- ✅ **JWT token generation** - Stateless authentication
- ✅ **JWT token verification** - Validate tokens
- ✅ **All tests passing** - 3/3 auth tests working

**Example:**
```rust
// Hash password
let hash = hash_password("demo123")?;

// Create JWT token
let token = create_jwt(user_id, org_id, roles, secret, 24)?;

// Verify token
let claims = verify_jwt(&token, secret)?;
```

### Database Schema ✅
**Location**: `server/migrations/001_initial_schema.sql`

**Tables Implemented:**
- ✅ **organizations** - Multi-tenant support
- ✅ **users** - User accounts with password hashes
- ✅ **roles** - Role-based permissions
- ✅ **user_roles** - Many-to-many role assignments
- ✅ **channels** - Hierarchical voice channels
- ✅ **channel_acls** - Per-channel permissions
- ✅ **sessions** - Active user sessions
- ✅ **audit_logs** - Security audit trail

### WebSocket Control Server ✅
**Location**: `server/src/network/tls.rs`

**Features:**
- ✅ **WebSocket server** running on port 9000
- ✅ **TLS-ready** (currently using plain WS for dev)
- ✅ **Connection handling**
- ✅ **Message routing** framework

---

## 🔧 What We Need to Implement

### 1. Client-Side Authentication (Next Step!)

#### A. WebSocket Client
**File**: `client/include/network/websocket_client.h`

```cpp
class WebSocketClient {
public:
    // Connect to control server
    Result<void> connect(const std::string& server, uint16_t port);
    
    // Authenticate with credentials
    Result<AuthResponse> authenticate(const std::string& username, 
                                     const std::string& password,
                                     const std::string& org);
    
    // Join/leave channels
    Result<void> join_channel(ChannelId channel_id);
    Result<void> leave_channel(ChannelId channel_id);
    
    // Send control messages
    Result<void> send_message(const ControlMessage& msg);
    
    // Receive callback
    void set_message_callback(MessageCallback callback);
    
private:
    // WebSocket connection
    std::unique_ptr<WebSocket> ws_;
    std::string auth_token_;  // JWT token
    UserId user_id_;
    OrgId org_id_;
};
```

#### B. Control Message Protocol
**File**: `common/include/protocol/control_messages.h`

```cpp
// Client → Server: Login request
struct LoginRequest {
    std::string username;
    std::string password;
    std::string org_tag;
};

// Server → Client: Login response
struct LoginResponse {
    bool success;
    std::string token;        // JWT token
    UserId user_id;
    OrgId org_id;
    std::vector<Channel> channels;  // Available channels
    std::string error_message;
};

// Client → Server: Join channel
struct JoinChannelRequest {
    ChannelId channel_id;
    std::string token;  // JWT auth
};

// Server → Client: User joined
struct UserJoinedNotification {
    ChannelId channel_id;
    UserId user_id;
    std::string username;
};

// Server → Client: User left
struct UserLeftNotification {
    ChannelId channel_id;
    UserId user_id;
};
```

### 2. Server Authentication Handler

#### A. WebSocket Authentication Flow
**File**: `server/src/network/websocket_handler.rs`

```rust
async fn handle_login(
    msg: LoginRequest,
    db: &Database,
) -> Result<LoginResponse> {
    // 1. Query user from database
    let user = db.get_user_by_username(&msg.org_tag, &msg.username).await?;
    
    // 2. Verify password
    if !verify_password(&msg.password, &user.password_hash)? {
        return Ok(LoginResponse {
            success: false,
            error_message: "Invalid credentials".to_string(),
            ..Default::default()
        });
    }
    
    // 3. Get user roles
    let roles = db.get_user_roles(user.id).await?;
    
    // 4. Create JWT token
    let token = create_jwt(
        user.id,
        user.org_id,
        roles,
        &CONFIG.security.jwt_secret,
        24, // 24 hours
    )?;
    
    // 5. Create session
    db.create_session(user.id, &token).await?;
    
    // 6. Get available channels
    let channels = db.get_channels_for_org(user.org_id).await?;
    
    // 7. Update last login
    db.update_last_login(user.id).await?;
    
    Ok(LoginResponse {
        success: true,
        token,
        user_id: user.id,
        org_id: user.org_id,
        channels,
        error_message: String::new(),
    })
}
```

#### B. Channel Join Authorization
```rust
async fn handle_join_channel(
    msg: JoinChannelRequest,
    db: &Database,
) -> Result<()> {
    // 1. Verify JWT token
    let claims = verify_jwt(&msg.token, &CONFIG.security.jwt_secret)?;
    
    // 2. Check channel exists
    let channel = db.get_channel(msg.channel_id).await?;
    
    // 3. Check user has permission
    let has_permission = db.check_channel_permission(
        claims.sub,  // user_id
        msg.channel_id,
        Permission::SPEAK,
    ).await?;
    
    if !has_permission {
        return Err(VoipError::PermissionDenied(
            "No permission to join this channel".to_string()
        ));
    }
    
    // 4. Add user to channel
    db.add_user_to_channel(claims.sub, msg.channel_id).await?;
    
    // 5. Notify other users
    broadcast_user_joined(msg.channel_id, claims.sub).await?;
    
    Ok(())
}
```

### 3. Voice Packet Routing (Multi-User)

#### Current: Echo Mode ✅
```rust
// Currently echoes back to sender
socket.send_to(&packet_data, sender_addr).await?;
```

#### Need: Multi-User Routing
**File**: `server/src/routing/voice_router.rs`

```rust
pub struct VoiceRouter {
    // Channel ID → Set of (User ID, Socket Address)
    channels: Arc<RwLock<HashMap<ChannelId, HashSet<(UserId, SocketAddr)>>>>,
}

impl VoiceRouter {
    pub async fn route_packet(
        &self,
        packet: VoicePacket,
        sender_addr: SocketAddr,
    ) -> Result<()> {
        let channel_id = packet.header.channel_id;
        let sender_user_id = packet.header.user_id;
        
        // Get all users in the channel
        let channels = self.channels.read().await;
        let users = channels.get(&channel_id)
            .ok_or(VoipError::ChannelNotFound(channel_id))?;
        
        // Serialize packet once
        let packet_data = serialize_packet(&packet)?;
        
        // Send to all users in channel EXCEPT sender
        for (user_id, addr) in users.iter() {
            if *user_id != sender_user_id {
                // Send to this user
                socket.send_to(&packet_data, addr).await?;
            }
        }
        
        Ok(())
    }
    
    pub async fn add_user_to_channel(
        &self,
        channel_id: ChannelId,
        user_id: UserId,
        addr: SocketAddr,
    ) {
        let mut channels = self.channels.write().await;
        channels.entry(channel_id)
            .or_insert_with(HashSet::new)
            .insert((user_id, addr));
    }
    
    pub async fn remove_user_from_channel(
        &self,
        channel_id: ChannelId,
        user_id: UserId,
    ) {
        let mut channels = self.channels.write().await;
        if let Some(users) = channels.get_mut(&channel_id) {
            users.retain(|(id, _)| *id != user_id);
        }
    }
}
```

### 4. Integration: VoiceSession + Authentication

**File**: `client/src/session/authenticated_voice_session.h`

```cpp
class AuthenticatedVoiceSession {
public:
    struct Config {
        std::string server_address;
        uint16_t control_port = 9000;  // WebSocket
        uint16_t voice_port = 9001;    // UDP
        
        // Auth credentials
        std::string username;
        std::string password;
        std::string org_tag;
        
        // Audio config
        AudioConfig audio;
        OpusConfig opus;
    };
    
    // Connect and authenticate
    Result<void> connect(const Config& config);
    
    // Join a channel (starts voice transmission)
    Result<void> join_channel(ChannelId channel_id);
    
    // Leave channel (stops voice transmission)
    void leave_channel();
    
    // Get list of available channels
    std::vector<Channel> get_channels() const;
    
    // Get users in current channel
    std::vector<UserInfo> get_channel_users() const;
    
private:
    std::unique_ptr<WebSocketClient> control_;
    std::unique_ptr<VoiceSession> voice_;
    
    std::string auth_token_;
    UserId user_id_;
    OrgId org_id_;
    ChannelId current_channel_;
};
```

---

## 📋 Implementation Roadmap

### Phase 1: Authentication (1-2 hours)
- [ ] Implement WebSocketClient (C++)
- [ ] Define control message protocol
- [ ] Implement login flow (client + server)
- [ ] Test authentication end-to-end

### Phase 2: Channel Management (1 hour)
- [ ] Implement join/leave channel (client)
- [ ] Implement server-side channel authorization
- [ ] Add channel user tracking
- [ ] Test channel switching

### Phase 3: Multi-User Routing (1 hour)
- [ ] Replace echo with VoiceRouter
- [ ] Implement channel-based routing
- [ ] Handle user join/leave events
- [ ] Test with 2-3 clients

### Phase 4: Integration (1 hour)
- [ ] Create AuthenticatedVoiceSession
- [ ] Wire WebSocket + Voice together
- [ ] Add UI hooks for channel list
- [ ] Test complete multi-user flow

### Phase 5: Polish (1 hour)
- [ ] Add error handling
- [ ] Add reconnection logic
- [ ] Add presence indicators
- [ ] Add voice activity detection

**Total Estimated Time: 5-6 hours**

---

## 🎯 Testing Strategy

### Test 1: Single User Authentication
```
1. Client connects to control server
2. Client sends LoginRequest
3. Server validates credentials
4. Server returns JWT token
5. Client stores token
✅ PASS if token received
```

### Test 2: Channel Join
```
1. Authenticated user joins channel 1
2. Server adds user to channel
3. Server sends confirmation
✅ PASS if user in channel
```

### Test 3: Two Users Same Channel
```
1. User A joins channel 1
2. User B joins channel 1
3. User A speaks
4. User B hears User A
5. User B speaks
6. User A hears User B
✅ PASS if both hear each other
```

### Test 4: Multiple Channels
```
1. User A in channel 1
2. User B in channel 2
3. User A speaks
4. User B should NOT hear
5. User B joins channel 1
6. User A speaks
7. User B SHOULD hear
✅ PASS if isolation works
```

---

## 💾 Database Queries Needed

```sql
-- Get user by username
SELECT * FROM users 
WHERE org_id = (SELECT id FROM organizations WHERE tag = $1)
AND username = $2;

-- Get user roles
SELECT r.* FROM roles r
JOIN user_roles ur ON r.id = ur.role_id
WHERE ur.user_id = $1;

-- Get channels for org
SELECT * FROM channels
WHERE org_id = $1
ORDER BY position;

-- Check channel permission
SELECT COUNT(*) FROM channel_acls
WHERE channel_id = $1
AND (user_id = $2 OR role_id IN ($3...))
AND (permissions & $4) = $4;

-- Add user to channel session
INSERT INTO channel_sessions (channel_id, user_id, joined_at)
VALUES ($1, $2, NOW());
```

---

## 🔒 Security Considerations

### 1. Password Security ✅
- Argon2id hashing (already implemented)
- No plaintext passwords stored
- Salted hashes

### 2. Token Security ✅
- JWT with expiration
- Signed tokens (HMAC-SHA256)
- 24-hour expiry

### 3. Channel Security
- ACL-based permissions
- Role hierarchy
- Per-channel passwords (optional)

### 4. Voice Security (Future)
- AES-256-GCM encryption
- Perfect forward secrecy
- DTLS key exchange

---

## 🎮 Example Usage (After Implementation)

```cpp
// Connect and authenticate
AuthenticatedVoiceSession session;
AuthenticatedVoiceSession::Config config;
config.server_address = "voip.example.com";
config.username = "alice";
config.password = "secure_password";
config.org_tag = "ACME";

auto result = session.connect(config);
if (!result.is_ok()) {
    std::cerr << "Failed to connect: " << result.error() << "\n";
    return;
}

// Get available channels
auto channels = session.get_channels();
std::cout << "Available channels:\n";
for (const auto& channel : channels) {
    std::cout << "  " << channel.name << " (" << channel.id << ")\n";
}

// Join a channel (starts voice transmission)
session.join_channel(1);  // Join channel ID 1

// Now speak - your voice goes to all users in channel 1!
// Others in channel 1 will hear you!

// Leave channel
session.leave_channel();
```

---

## 📊 Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                         Client A                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  WebSocket   │  │     Voice    │  │    Audio     │     │
│  │   Control    │  │   Session    │  │   Engine     │     │
│  │  (Auth/Join) │  │  (UDP Voice) │  │ (Mic/Speak)  │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                 │                  │              │
└─────────┼─────────────────┼──────────────────┼──────────────┘
          │                 │                  │
          │ :9000           │ :9001            │
          │ (WebSocket)     │ (UDP)            │
          ▼                 ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                          Server                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  WebSocket   │  │     Voice    │  │   Voice      │     │
│  │   Handler    │  │   Receiver   │  │   Router     │     │
│  │   (Auth)     │──│   (Parse)    │──│  (Channel)   │     │
│  └──────┬───────┘  └──────────────┘  └──────┬───────┘     │
│         │                                    │              │
│  ┌──────▼───────┐                     ┌─────▼────────┐     │
│  │  PostgreSQL  │                     │   Channel    │     │
│  │  (Users/ACL) │                     │  Membership  │     │
│  └──────────────┘                     └──────┬───────┘     │
│                                              │              │
└──────────────────────────────────────────────┼──────────────┘
                                               │
          ┌────────────────────────────────────┘
          │
          │ (Forward to all in channel)
          │
          ▼
┌─────────────────────────────────────────────────────────────┐
│                         Client B                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  WebSocket   │  │     Voice    │  │    Audio     │     │
│  │   Control    │  │   Session    │  │   Engine     │     │
│  │  (Receive)   │  │  (Receive)   │  │  (Playback)  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start (After Implementation)

```bash
# 1. Start server with DB
cd server
docker-compose up -d postgres  # Start database
cargo run

# 2. Create test user
psql -h localhost -U voip -d voip
INSERT INTO organizations (name, tag) VALUES ('Test Org', 'TEST');
INSERT INTO users (org_id, username, password_hash) 
VALUES (1, 'alice', '[argon2_hash]');

# 3. Run client A
cd client
./build/Debug/voip-client
# Login: alice / password123 / TEST
# Join: Channel 1

# 4. Run client B (different terminal)
./build/Debug/voip-client
# Login: bob / password456 / TEST
# Join: Channel 1

# 5. Speak! Alice and Bob can now talk!
```

---

## Summary

**We have:**
- ✅ Complete authentication system (server)
- ✅ Complete database schema
- ✅ Working voice transmission (single user)

**We need:**
- ⏳ Client-side WebSocket connection
- ⏳ Login/authentication flow
- ⏳ Multi-user voice routing
- ⏳ Channel management

**Estimated time: 5-6 hours to complete multi-user support!**

Want to start? Let's begin with the WebSocket client! 🚀
