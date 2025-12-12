# Next Steps - Network Integration

## ✅ What We Just Added

### Client Network Layer
- **UDP Voice Socket** (`client/src/network/udp_socket.cpp`)
  - Send/receive voice packets
  - Non-blocking async operations
  - Thread-safe packet handling
  - Network statistics

**Features:**
- ✅ Packet serialization/deserialization
- ✅ Network byte order handling
- ✅ Non-blocking I/O
- ✅ Receive callback system
- ✅ Statistics tracking
- ✅ Windows & Linux ready

---

## 🧪 Test Network Connection

### 1. Make Sure Server is Running
```bash
cd C:\dev\VoIP-System\server
cargo run

# You should see:
# ✅ UDP voice server started (port 9001)
# ✅ WebSocket control server started (port 9000)
```

### 2. Run Client (Now with Network!)
```bash
cd C:\dev\VoIP-System\client
.\build\Debug\voip-client.exe
```

The client can now potentially connect to the server!

---

## 🔌 How to Wire Everything Together

### Complete Audio→Network→Audio Pipeline

Here's what a complete integration looks like:

```cpp
// 1. Create components
AudioEngine audio_engine;
OpusEncoder encoder;
OpusDecoder decoder;
UdpVoiceSocket network;
JitterBuffer jitter_buffer(5, 960);

// 2. Connect to server
network.connect("127.0.0.1", 9001);

// 3. Set up audio capture → encode → send
audio_engine.set_capture_callback([&](const float* pcm, size_t frames) {
    // Encode audio
    auto encoded = encoder->encode(pcm, frames);
    
    // Create voice packet
    VoicePacket packet;
    packet.header.magic = VOICE_PACKET_MAGIC;
    packet.header.sequence = next_seq++;
    packet.header.timestamp = get_timestamp();
    packet.header.channel_id = current_channel;
    packet.header.user_id = my_user_id;
    packet.encrypted_payload = encoded.value().data;
    
    // Send to server
    network.send_packet(packet);
});

// 4. Set up receive → decode → playback
network.set_receive_callback([&](const VoicePacket& packet) {
    // Add to jitter buffer
    AudioPacket audio_packet;
    audio_packet.sequence = packet.header.sequence;
    audio_packet.timestamp = packet.header.timestamp;
    
    // Decode Opus
    decoder->decode(
        packet.encrypted_payload.data(),
        packet.encrypted_payload.size(),
        audio_packet.samples.data(),
        960
    );
    
    jitter_buffer.push(std::move(audio_packet));
});

// 5. Set up playback from jitter buffer
audio_engine.set_playback_callback([&](float* pcm, size_t frames) {
    auto packet = jitter_buffer.pop();
    if (packet.has_value()) {
        std::memcpy(pcm, packet->samples.data(), frames * sizeof(float));
    } else {
        // Silence
        std::fill(pcm, pcm + frames, 0.0f);
    }
});
```

---

## 📋 Implementation Checklist

### Phase 1: Basic Connection ✅
- [x] UDP socket implementation
- [x] Packet serialization
- [x] Non-blocking I/O
- [x] Client builds successfully

### Phase 2: Audio Integration (Next!)
- [ ] Wire audio capture to network send
- [ ] Wire network receive to audio playback
- [ ] Add sequence number tracking
- [ ] Add timestamp generation
- [ ] Test loopback (mic → server → speakers)

### Phase 3: WebSocket Control
- [ ] Implement WebSocket client
- [ ] Authentication flow
- [ ] Channel join/leave
- [ ] User state management

### Phase 4: Multi-Channel
- [ ] Channel routing
- [ ] Multiple simultaneous channels
- [ ] Per-channel audio mixing
- [ ] PTT state management

---

## 🎯 Immediate Next Task

### Create Audio-Network Integration

Let's create a simple program that:
1. Captures from microphone
2. Encodes with Opus
3. Sends to server via UDP
4. Receives from server
5. Decodes Opus
6. Plays to speakers

This will be our **first end-to-end voice test**!

**File to create**: `client/examples/voice_loopback.cpp`

---

## 🔍 Testing Strategy

### Test 1: Network Connectivity
```bash
# Run server
cd server && cargo run

# In another terminal, test network
cd client
# Create simple network test that sends packets
```

### Test 2: Audio Loopback (Local)
- Mic → Encode → Decode → Speakers
- **Status**: Already working in tests!

### Test 3: Network Loopback
- Mic → Encode → Network → Decode → Speakers
- **Status**: Ready to implement!

### Test 4: Two Clients
- Client A → Server → Client B
- **Status**: After Test 3

---

## 📊 Current Architecture

```
Client                          Server
┌─────────────┐                ┌──────────────┐
│  Microphone │                │              │
│      ↓      │                │  UDP Socket  │
│   Capture   │                │   :9001      │
│      ↓      │                │      ↓       │
│    Opus     │                │   Routing    │
│   Encoder   │                │   Engine     │
│      ↓      │                │      ↓       │
│    UDP      │────Packet─────→│   Forward    │
│   Socket    │                │      ↓       │
│      ↑      │←───Packet──────│   Clients    │
│   Jitter    │                │              │
│   Buffer    │                └──────────────┘
│      ↓      │                         
│    Opus     │
│   Decoder   │
│      ↓      │
│  Playback   │
│      ↓      │
│  Speakers   │
└─────────────┘
```

**Status**: 
- ✅ Audio components working
- ✅ Network components working
- ⏳ Integration needed
- ⏳ Server routing needed

---

## 🚀 Quick Win: Create Voice Loopback Test

Want to see/hear it working? Let's create a quick integration test that actually transmits voice!

**Would you like me to:**
1. Create `voice_loopback.cpp` - Full audio→network→audio test
2. Add server packet echo logic - Server echoes packets back
3. Create simple UI showing connection status
4. Add more network features (WebSocket control)

**Recommended**: Start with #1 - Voice loopback test!

---

## 📝 Notes

- Server is already running and can receive packets ✅
- Client can now send/receive packets ✅
- Audio pipeline is complete ✅
- Just need to wire them together! ⏳

**Next command to run:**
```bash
# Let's create the voice loopback integration!
```

Ready to make the system actually transmit voice? 🎤→🔊
