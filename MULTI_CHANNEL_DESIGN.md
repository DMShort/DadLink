# Multi-Channel Design Document

## 🎯 **Objective**
Implement Discord/TeamSpeak-style multi-channel functionality:
- **Listen to multiple channels simultaneously** (channel mixing)
- **Transmit to one active channel** at a time
- **Switch transmit channel** without leaving/rejoining
- **Channel indicators** showing listen/transmit status

---

## 📊 **Current Architecture Analysis**

### **Server-Side (Rust)**

#### ✅ **What Works:**
1. **ChannelManager** (`channel_manager.rs`)
   - Tracks users in channels via WebSocket control
   - Supports join/leave operations
   - Broadcasts notifications to channel members

2. **VoiceRouter** (`routing/voice_router.rs`)
   - Routes UDP voice packets between users
   - Maps UDP addresses to user/channel

#### ❌ **Current Limitations:**
1. **Single channel only** - `leave_all_channels()` called before joining new channel (line 44)
2. **No multi-channel voice routing** - can't broadcast to multiple channels
3. **No transmit channel concept** - user transmits to whichever channel they're "in"

### **Client-Side (C++/Qt)**

#### ✅ **What Works:**
1. **Channel List UI** - Shows "General" and "Gaming" channels
2. **Join/Leave** - Can join one channel via WebSocket
3. **Voice Session** - Hardcoded to channel ID 1

#### ❌ **Current Limitations:**
1. **Single channel mode** - Can only be in one channel at a time
2. **No channel indicators** - Can't see which channels you're listening to
3. **No transmit channel selector** - Can't choose where to transmit
4. **No audio mixing** - Can only hear one channel

---

## 🏗️ **Design: Multi-Channel System**

### **Concept: Listen vs. Transmit**

```
User State:
- Listening Channels: [1, 2, 3]  (hear audio from all)
- Transmit Channel: 2            (speak to this one only)
```

**Example Scenario:**
- User joins "General" (channel 1) → listening + transmit
- User joins "Gaming" (channel 2) → listening + transmit switches to 2
- User joins "Music" (channel 3) → listening + transmit switches to 3
- User sets transmit back to "General" → transmit to 1, still listening to all 3

---

## 🔧 **Implementation Plan**

### **Phase 1: Server Multi-Channel Support**

#### **1.1 - Update VoiceRouter for Multi-Channel**

**File:** `server/src/routing/voice_router.rs`

**Changes:**
```rust
// BEFORE: User in ONE channel
HashMap<ChannelId, HashSet<UserSession>>

// AFTER: User in MULTIPLE channels
channels: HashMap<ChannelId, HashSet<UserSession>>
user_channels: HashMap<SocketAddr, HashSet<ChannelId>>  // NEW: Track all channels per user
user_transmit_channel: HashMap<SocketAddr, ChannelId>   // NEW: Active transmit channel
```

**New Methods:**
- `join_channel_multi()` - Add to channel WITHOUT leaving others
- `leave_channel_multi()` - Remove from specific channel
- `set_transmit_channel()` - Change where user transmits
- `get_listening_channels()` - Get all channels user is in
- `route_voice_multi()` - Route voice to transmit channel only

**Behavior:**
- Voice packets from user → route to `user_transmit_channel` ONLY
- Voice packets to user → mix from ALL channels in `user_channels`

#### **1.2 - Update Control Protocol**

**File:** `server/src/types.rs`

**Add new message:**
```rust
SetTransmitChannel {
    channel_id: ChannelId,
}
```

**Handler in** `server/src/network/tls.rs`:
- Validate user is listening to that channel
- Update VoiceRouter transmit channel
- Send confirmation

---

### **Phase 2: Client Multi-Channel UI**

#### **2.1 - Channel List Enhancements**

**File:** `client/src/ui/main_window.cpp`

**Features:**
- **Visual indicators:** 
  - 🎧 = Listening
  - 🎤 = Transmitting
  - Example: `🎧🎤 # General` (listen + transmit)
  - Example: `🎧 # Gaming` (listen only)

- **Actions:**
  - Double-click → Join/Leave channel (toggle listen)
  - Right-click → Context menu:
    - "Set as Transmit Channel"
    - "Leave Channel"
    - "Mute Channel" (future)

- **Multi-selection:**
  - Track joined channels in `std::set<ChannelId> joinedChannels_`
  - Track transmit channel in `ChannelId transmitChannel_`

#### **2.2 - Channel Controls Widget**

**New widget showing:**
```
Current Transmit: # General
                  [Change Channel ▼]
                  
Listening To: (3 channels)
  🎧🎤 # General
  🎧   # Gaming  
  🎧   # Music
```

---

### **Phase 3: Client Voice Multi-Channel**

#### **3.1 - Voice Session Updates**

**File:** `client/src/session/voice_session.cpp`

**Changes:**
```cpp
// BEFORE
ChannelId channel_id_;  // Single channel

// AFTER
ChannelId transmit_channel_id_;           // Where we send
std::set<ChannelId> listening_channels_;  // Where we receive from
std::map<ChannelId, JitterBuffer> channel_buffers_;  // Separate buffer per channel
```

**Audio Capture (Transmission):**
- Use `transmit_channel_id_` in voice packet header
- No change to encoding logic

**Audio Playback (Reception):**
- Accept packets from ANY channel in `listening_channels_`
- Route to appropriate `channel_buffers_[channel_id]`
- **Mix audio** from all channels before playback

#### **3.2 - Audio Mixing**

**Mixing Strategy:**
```cpp
void mix_channels(float* output, size_t frames) {
    memset(output, 0, frames * sizeof(float));
    
    for (auto& [channel_id, buffer] : channel_buffers_) {
        float temp[960];
        if (buffer.get_frame(temp, frames)) {
            // Simple additive mixing with clipping
            for (size_t i = 0; i < frames; i++) {
                output[i] += temp[i];
                output[i] = std::clamp(output[i], -1.0f, 1.0f);
            }
        }
    }
}
```

**Optimization:**
- Weight channels by user count (busy channels quieter)
- Optional: Ducking (reduce music channels when voice active)
- Optional: Per-channel volume controls

---

### **Phase 4: WebSocket Protocol Updates**

#### **4.1 - Client Methods**

**File:** `client/include/network/websocket_client.h`

**Add:**
```cpp
Result<void> set_transmit_channel(ChannelId channel_id);
std::set<ChannelId> get_joined_channels() const;
ChannelId get_transmit_channel() const;
```

#### **4.2 - Server Handlers**

**File:** `server/src/network/tls.rs`

**Update JoinChannel:**
- Don't call `leave_all_channels()`
- Add to listening channels
- If first channel, set as transmit
- Broadcast join notification

**Add SetTransmitChannel:**
- Verify user is in that channel
- Update VoiceRouter
- Send confirmation

---

## 🧪 **Testing Scenarios**

### **Test 1: Multi-Channel Listen**
1. User A joins Channel 1 and 2
2. User B in Channel 1 speaks → A hears it
3. User C in Channel 2 speaks → A hears it
4. User A hears BOTH simultaneously (mixed)

### **Test 2: Transmit Channel Switching**
1. User A joins Channel 1 and 2 (transmit = 2)
2. User A speaks → only Channel 2 users hear
3. User A switches transmit to Channel 1
4. User A speaks → only Channel 1 users hear
5. User A still HEARS both channels

### **Test 3: Dynamic Join/Leave**
1. User A in Channels 1, 2, 3
2. User A leaves Channel 2 → stops hearing Channel 2
3. User A rejoins Channel 2 → hears it again
4. Transmit channel preserved throughout

---

## 📋 **Implementation Checklist**

### **Server**
- [ ] Modify VoiceRouter to support multiple channels per user
- [ ] Add user_channels and user_transmit_channel tracking
- [ ] Implement join_channel_multi() (don't leave others)
- [ ] Implement set_transmit_channel()
- [ ] Update voice routing to use transmit channel
- [ ] Add SetTransmitChannel control message
- [ ] Update JoinChannel handler (no auto-leave)

### **Client - Backend**
- [ ] Track joined channels set in MainWindow
- [ ] Track transmit channel in MainWindow
- [ ] Add set_transmit_channel() to WebSocketClient
- [ ] Update VoiceSession for multi-channel
- [ ] Implement audio mixing from multiple channels
- [ ] Create per-channel jitter buffers

### **Client - UI**
- [ ] Add channel indicators (🎧 listen, 🎤 transmit)
- [ ] Update channel list items dynamically
- [ ] Add right-click context menu
- [ ] Add transmit channel selector widget
- [ ] Show joined channels list
- [ ] Visual feedback on transmit channel changes

### **Testing**
- [ ] Test 2-user, 2-channel scenario
- [ ] Test 3-user, 3-channel mixing
- [ ] Test transmit channel switching
- [ ] Test audio quality with mixing
- [ ] Test dynamic join/leave

---

## 🎯 **Success Criteria**

✅ **User can listen to 3+ channels simultaneously**  
✅ **Audio from all channels mixed clearly**  
✅ **User can switch transmit channel without leaving**  
✅ **UI clearly shows listen/transmit status**  
✅ **No audio dropouts during channel switches**  
✅ **Multiple users in multiple channels work smoothly**

---

## 🚀 **Future Enhancements**

- **Per-channel volume controls**
- **Channel muting** (stop hearing specific channel)
- **Audio ducking** (reduce music when voice active)
- **Priority channels** (always hear admin channel)
- **Channel groups** (categories like Discord)
- **Quick channel hotkeys** (press 1-9 to switch transmit)

---

**Status:** 📝 Design Complete - Ready for Implementation  
**Date:** November 23, 2025  
**Estimated Time:** 4-6 hours full implementation
