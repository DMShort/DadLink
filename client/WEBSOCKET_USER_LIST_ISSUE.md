# WebSocket User List - Server Architecture Issue

## 🚨 **Problem Summary**

**The user list doesn't populate because the Rust server's WebSocket implementation is incomplete.**

### Current Behavior:
- ✅ Voice communication works perfectly (UDP routing functional)
- ✅ WebSocket authentication works
- ✅ WebSocket channel join works
- ❌ **User list is always empty**
- ❌ **No notifications when other users join/leave**

---

## 🔍 **Root Cause**

### Server Code Analysis

**File**: `server/src/network/tls.rs` (lines 254-259)

```rust
ControlMessage::JoinChannel { channel_id, password: _ } => {
    // ... authentication check ...
    
    let response = ControlMessage::ChannelJoined {
        channel_id,
        channel_name: format!("Channel {}", channel_id),
        users: vec![],  // ← HARD-CODED EMPTY!
    };
    send_message(socket, &response).await?;
    
    Ok(true)
}
```

**Issues:**
1. **No shared state** between WebSocket connections
2. **No user tracking** - server doesn't know who's in which channel
3. **No broadcasting** - can't notify other users when someone joins
4. **Empty user list** - always returns `users: vec![]`

---

## 🏗️ **Architecture Comparison**

### Voice (UDP) - Works Perfect ✅
```
Client A ──UDP packet──> VoiceRouter (shared state)
                              │
                              ├──> Client A (loopback)
                              └──> Client B (broadcast)

✓ VoiceRouter maintains channel membership
✓ Broadcasts packets to all users in channel
✓ Shared state via Arc<Mutex<...>>
```

### Control (WebSocket) - Incomplete ❌
```
Client A ──login──> WebSocket Handler A (isolated)
Client B ──login──> WebSocket Handler B (isolated)
                          │
                          NO COMMUNICATION!

✗ Each WebSocket connection independent
✗ No shared channel state
✗ No way to notify other users
```

---

## 🔧 **What The Server Needs**

### 1. Shared Channel Manager
```rust
pub struct ChannelManager {
    // Track users in each channel
    channels: Arc<Mutex<HashMap<ChannelId, ChannelState>>>,
    
    // Map user ID to WebSocket sender (for broadcasting)
    user_sockets: Arc<Mutex<HashMap<UserId, mpsc::Sender<ControlMessage>>>>,
}

pub struct ChannelState {
    id: ChannelId,
    name: String,
    users: Vec<UserInfo>,
}
```

### 2. User Join Logic
```rust
async fn handle_join_channel(
    channel_id: ChannelId,
    user: UserInfo,
    channel_manager: &ChannelManager,
) -> Result<()> {
    // 1. Add user to channel state
    let users = channel_manager.add_user(channel_id, user.clone()).await?;
    
    // 2. Send channel_joined with REAL user list
    send_message(socket, &ControlMessage::ChannelJoined {
        channel_id,
        channel_name: "General",
        users: users.clone(),  // ← Actual users!
    }).await?;
    
    // 3. Broadcast to ALL OTHER users in channel
    channel_manager.broadcast_to_channel(
        channel_id,
        &ControlMessage::ChannelState {
            channel_id,
            event: "user_joined".to_string(),
            user: Some(user),
        },
        Some(user.id),  // Exclude sender
    ).await?;
    
    Ok(())
}
```

### 3. Broadcast Mechanism
```rust
async fn broadcast_to_channel(
    &self,
    channel_id: ChannelId,
    message: &ControlMessage,
    exclude_user: Option<UserId>,
) -> Result<()> {
    let users = self.get_channel_users(channel_id).await?;
    let sockets = self.user_sockets.lock().await;
    
    for user in users {
        if Some(user.id) == exclude_user {
            continue;
        }
        
        if let Some(sender) = sockets.get(&user.id) {
            sender.send(message.clone()).await?;
        }
    }
    
    Ok(())
}
```

---

## 🎯 **Recommended Solutions**

### **Option A: Fix The Server** (Proper Solution)
**Pros:**
- Fixes the root cause
- Enables all WebSocket features (user list, typing indicators, etc.)
- Scalable architecture

**Cons:**
- Requires significant server refactoring
- Needs testing with multiple users
- Takes time to implement

**Implementation:**
1. Create `ChannelManager` struct
2. Add to server state (`Arc<ChannelManager>`)
3. Pass to WebSocket handlers
4. Implement user tracking and broadcasting
5. Update join/leave handlers

**Effort**: ~2-4 hours

---

### **Option B: Client-Side User Discovery** (Quick Workaround)
**Pros:**
- No server changes needed
- Works immediately
- Uses existing UDP voice packets

**Cons:**
- Hacky solution
- Users must speak to be discovered
- No notification when users leave (until timeout)

**Implementation:**
```cpp
// In VoiceSession::on_packet_received()
void VoiceSession::on_packet_received(const VoicePacket& packet) {
    // ... existing decode logic ...
    
    // Auto-discover users from voice packets
    if (packet.user_id != my_user_id_) {
        emit userDiscoveredFromVoice(packet.user_id, packet.channel_id);
    }
}

// In MainWindow, wire up signal:
connect(voiceSession, &VoiceSession::userDiscoveredFromVoice,
        this, &MainWindow::onVoiceUserDiscovered);

void MainWindow::onVoiceUserDiscovered(uint32_t userId, uint32_t channelId) {
    // Check if user already in list
    for (int i = 0; i < userList_->count(); ++i) {
        if (userList_->item(i)->data(Qt::UserRole).toUInt() == userId) {
            return;  // Already have them
        }
    }
    
    // Add new user (use User ID as name until we get real name)
    auto* item = new QListWidgetItem(QString("User %1").arg(userId));
    item->setData(Qt::UserRole, userId);
    userList_->addItem(item);
    addLogMessage(QString("👤 Discovered User %1 from voice").arg(userId));
}
```

**Effort**: ~30 minutes

---

### **Option C: Hybrid Approach** (Recommended)
**Short-term**: Use Option B (voice-based discovery) to unblock testing  
**Long-term**: Implement Option A (proper server architecture)

---

## 📊 **Current Status**

| Component | Status | Notes |
|-----------|--------|-------|
| **WebSocket Connection** | ✅ Working | Auth succeeds with demo/demo123 |
| **Voice UDP** | ✅ Working | Users can hear each other |
| **VoiceRouter** | ✅ Working | Has shared state, broadcasts |
| **WebSocket Shared State** | ❌ Missing | No ChannelManager |
| **User List from Server** | ❌ Broken | Always returns empty vec![] |
| **User Join Notifications** | ❌ Missing | No broadcast mechanism |
| **User Leave Notifications** | ❌ Missing | No broadcast mechanism |

---

## 🚀 **Next Steps**

### Immediate (To Unblock Testing):
1. Implement Option B (voice-based user discovery)
2. Test with 2-3 clients
3. Verify users appear after speaking

### Short-Term:
1. Document server architecture requirements
2. Create GitHub issue for server improvements
3. Propose ChannelManager design

### Long-Term:
1. Implement proper server architecture (Option A)
2. Add database-backed user tracking
3. Add presence system (online/offline/away)
4. Add typing indicators, user status, etc.

---

## 💻 **Testing The Current System**

**What Works:**
```
1. Client A joins channel 1
2. Client B joins channel 1
3. Client A speaks → Client B hears ✅
4. Client B speaks → Client A hears ✅
```

**What Doesn't Work:**
```
1. Client A joins channel 1
   → Sees only themselves in user list
2. Client B joins channel 1
   → Client A doesn't see Client B ❌
   → Client B sees only themselves ❌
```

**Workaround:**
```
1. Client A joins channel 1
2. Client B joins channel 1
3. Client B speaks (sends voice packet)
   → Client A's VoiceSession receives packet
   → Client A discovers Client B from packet ✅
```

---

**Status**: Server architecture gap identified  
**Impact**: High (blocks user list feature)  
**Recommendation**: Implement Option C (Hybrid - workaround now, fix later)
