# Testing Strategy
## Gaming-Focused Multi-Channel VoIP System

---

## Testing Philosophy

**Principle**: Testing is a **first-class concern**, not an afterthought.

For every non-trivial feature, we MUST define:
1. How we will test it
2. Typical failure modes
3. Logging/instrumentation
4. Regression tests

---

## Testing Pyramid

```
        /\
       /  \
      / E2E\        ← End-to-End Tests (few, slow, critical paths)
     /______\
    /        \
   / Integration\   ← Integration Tests (moderate, subsystems)
  /____________\
 /              \
/   Unit Tests   \  ← Unit Tests (many, fast, logic validation)
/________________\
```

---

## 1. Unit Tests

### Scope
Test pure functions, data structures, algorithms in isolation.

### Examples

**Jitter Buffer Logic:**
```cpp
TEST(JitterBuffer, HandlesOutOfOrderPackets) {
    JitterBuffer buffer;
    buffer.set_target_delay(Duration::from_ms(30));
    
    // Push packets out of order
    buffer.push_packet(create_packet(seq: 2, timestamp: 20ms));
    buffer.push_packet(create_packet(seq: 1, timestamp: 10ms));
    buffer.push_packet(create_packet(seq: 3, timestamp: 30ms));
    
    // Should pop in order
    ASSERT_EQ(buffer.pop_packet().value().seq, 1);
    ASSERT_EQ(buffer.pop_packet().value().seq, 2);
    ASSERT_EQ(buffer.pop_packet().value().seq, 3);
}

TEST(JitterBuffer, DiscardsLatePackets) {
    JitterBuffer buffer;
    buffer.set_target_delay(Duration::from_ms(30));
    
    buffer.push_packet(create_packet(seq: 1, timestamp: 10ms));
    buffer.push_packet(create_packet(seq: 2, timestamp: 20ms));
    buffer.pop_packet();  // Consume seq 1
    buffer.pop_packet();  // Consume seq 2
    
    // Late packet (seq 0) should be discarded
    buffer.push_packet(create_packet(seq: 0, timestamp: 0ms));
    ASSERT_TRUE(buffer.pop_packet().is_none());
}
```

**Permission Checks:**
```rust
#[test]
fn test_user_can_speak_with_permission() {
    let user = create_test_user(roles: vec![Role::Speaker]);
    let channel = create_test_channel(acl: vec![
        ACLEntry { role: Role::Speaker, permissions: Permissions::SPEAK }
    ]);
    
    assert!(user.can_speak_in(&channel));
}

#[test]
fn test_user_cannot_speak_without_permission() {
    let user = create_test_user(roles: vec![Role::Listener]);
    let channel = create_test_channel(acl: vec![
        ACLEntry { role: Role::Listener, permissions: Permissions::JOIN }
    ]);
    
    assert!(!user.can_speak_in(&channel));
}
```

**Opus Encoding/Decoding:**
```cpp
TEST(OpusCodec, EncodeDecode Cycle) {
    const int SAMPLE_RATE = 48000;
    const int FRAME_SIZE = 960;  // 20ms
    
    auto encoder = OpusCodec::create_encoder(SAMPLE_RATE, 1, 32000);
    auto decoder = OpusCodec::create_decoder(SAMPLE_RATE, 1);
    
    // Generate test signal (440 Hz sine wave)
    std::vector<float> input(FRAME_SIZE);
    for (int i = 0; i < FRAME_SIZE; i++) {
        input[i] = sin(2.0 * M_PI * 440.0 * i / SAMPLE_RATE);
    }
    
    // Encode
    auto encoded = encoder->encode(input.data(), FRAME_SIZE);
    ASSERT_TRUE(encoded.is_ok());
    
    // Decode
    std::vector<float> output(FRAME_SIZE);
    auto decoded_size = decoder->decode(
        encoded.value().data.data(),
        encoded.value().data.size(),
        output.data()
    );
    ASSERT_EQ(decoded_size.value(), FRAME_SIZE);
    
    // Check correlation (should be >0.9 for good quality)
    float correlation = compute_correlation(input, output);
    ASSERT_GT(correlation, 0.9);
}
```

### Coverage Target
- **Goal**: >80% code coverage for logic modules
- **Tools**: gcov (C++), cargo-tarpaulin (Rust)

---

## 2. Integration Tests

### Scope
Test interaction between components (audio engine + codec, network + routing).

### Audio Engine + Codec Integration

```cpp
TEST(AudioIntegration, LoopbackTest) {
    // Setup loopback device (or mock)
    AudioEngine engine;
    engine.initialize(AudioConfig{
        sample_rate: 48000,
        frame_size: 480
    });
    
    OpusEncoder encoder = create_test_encoder();
    OpusDecoder decoder = create_test_decoder();
    
    std::vector<float> captured_audio;
    
    // Capture callback: encode and queue
    engine.set_capture_callback([&](const float* pcm, size_t frames) {
        auto encoded = encoder.encode(pcm, frames);
        // Queue for decode
    });
    
    // Playback callback: decode queued packets
    engine.set_playback_callback([&](float* pcm, size_t frames) {
        // Dequeue and decode
        decoder.decode(queued_packet, pcm);
    });
    
    engine.start_capture();
    engine.start_playback();
    
    // Run for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Verify no xruns (buffer under/overruns)
    auto stats = engine.get_stats();
    ASSERT_EQ(stats.xruns, 0);
}
```

### Network + Routing Integration

```rust
#[tokio::test]
async fn test_voice_packet_routing() {
    let server = start_test_server().await;
    
    // Create test org and channel
    let org = server.create_org("TestOrg").await.unwrap();
    let channel = server.create_channel(org.id, "Command").await.unwrap();
    
    // Connect two test clients
    let client1 = connect_test_client(&server, "user1", org.id).await;
    let client2 = connect_test_client(&server, "user2", org.id).await;
    
    // Both join channel
    client1.join_channel(channel.id).await.unwrap();
    client2.join_channel(channel.id).await.unwrap();
    
    // Client1 sends voice packet
    let packet = create_test_voice_packet(
        user_id: client1.id,
        channel_id: channel.id,
        payload: vec![0x01, 0x02, 0x03]
    );
    client1.send_voice(packet.clone()).await.unwrap();
    
    // Client2 should receive it
    let received = client2.receive_voice().await.unwrap();
    assert_eq!(received.user_id, client1.id);
    assert_eq!(received.payload, packet.payload);
}
```

---

## 3. Network Condition Tests

### Scope
Simulate real-world network conditions (jitter, loss, reordering).

### Test Harness

```rust
struct NetworkSimulator {
    packet_loss_rate: f32,      // 0.0-1.0
    jitter_ms: u32,              // ±N ms
    reorder_probability: f32,    // 0.0-1.0
}

impl NetworkSimulator {
    fn send_packet(&mut self, packet: VoicePacket) {
        // Simulate packet loss
        if random::<f32>() < self.packet_loss_rate {
            return;  // Drop packet
        }
        
        // Simulate jitter
        let delay = Duration::from_ms(
            (random::<f32>() * 2.0 - 1.0) * self.jitter_ms as f32
        );
        tokio::time::sleep(delay).await;
        
        // Simulate reordering
        if random::<f32>() < self.reorder_probability {
            // Delay this packet more
            tokio::time::sleep(Duration::from_ms(50)).await;
        }
        
        // Actually send
        self.real_socket.send(packet).await;
    }
}
```

### Test Scenarios

**Scenario 1: Moderate Packet Loss**
```rust
#[tokio::test]
async fn test_5_percent_packet_loss() {
    let mut sim = NetworkSimulator {
        packet_loss_rate: 0.05,
        jitter_ms: 10,
        reorder_probability: 0.0,
    };
    
    // Send 100 packets
    for i in 0..100 {
        sim.send_packet(create_test_packet(seq: i));
    }
    
    // Verify PLC (packet loss concealment) is used
    // Audio quality should still be acceptable
}
```

**Scenario 2: High Jitter**
```rust
#[tokio::test]
async fn test_high_jitter() {
    let mut sim = NetworkSimulator {
        packet_loss_rate: 0.0,
        jitter_ms: 50,  // ±50ms
        reorder_probability: 0.1,
    };
    
    // Jitter buffer should adapt
    // No packets should be discarded as "too late"
    // (unless jitter exceeds buffer capacity)
}
```

---

## 4. Load & Stress Tests

### Scope
Verify system handles high user counts and concurrent operations.

### Scenarios

**Test 1: 100 Concurrent Users**
```rust
#[tokio::test]
async fn test_100_users_single_channel() {
    let server = start_test_server().await;
    let channel = setup_test_channel(&server).await;
    
    // Connect 100 clients
    let mut clients = vec![];
    for i in 0..100 {
        let client = connect_test_client(&server, &format!("user{}", i)).await;
        client.join_channel(channel.id).await.unwrap();
        clients.push(client);
    }
    
    // 10 clients transmit simultaneously
    let mut tasks = vec![];
    for i in 0..10 {
        let client = clients[i].clone();
        tasks.push(tokio::spawn(async move {
            client.transmit_for_duration(Duration::from_secs(5)).await;
        }));
    }
    
    // Wait for completion
    futures::future::join_all(tasks).await;
    
    // Measure metrics
    let metrics = server.get_metrics();
    assert!(metrics.routing_latency_p95 < Duration::from_ms(10));
    assert!(metrics.cpu_usage < 0.8);  // <80% CPU
}
```

**Test 2: Many Channels**
```rust
#[tokio::test]
async fn test_50_channels_100_users() {
    let server = start_test_server().await;
    
    // Create 50 channels
    let channels = (0..50)
        .map(|i| server.create_channel(&format!("Channel{}", i)))
        .collect::<Vec<_>>();
    
    // 100 users, each in 3-5 random channels
    for i in 0..100 {
        let client = connect_test_client(&server, &format!("user{}", i)).await;
        let random_channels = choose_random_channels(&channels, 3..=5);
        for ch in random_channels {
            client.join_channel(ch.id).await;
        }
    }
    
    // Measure memory usage
    let metrics = server.get_metrics();
    assert!(metrics.memory_mb < 2000);  // <2GB
}
```

---

## 5. End-to-End Scenario Tests

### Scope
Scripted real-world operation scenarios.

### Scenario: Star Citizen Fleet Operation

```rust
#[tokio::test]
async fn test_fleet_operation_scenario() {
    let server = start_test_server().await;
    let org = server.create_org("TestOrg").await.unwrap();
    
    // Create channel structure
    let command = server.create_channel(org.id, "Command").await.unwrap();
    let flight = server.create_channel(org.id, "Flight Ops").await.unwrap();
    let alpha = server.create_channel_with_parent(
        org.id, "Alpha Wing", flight.id
    ).await.unwrap();
    let beta = server.create_channel_with_parent(
        org.id, "Beta Wing", flight.id
    ).await.unwrap();
    
    // Fleet commander
    let commander = connect_test_client(&server, "commander", org.id).await;
    commander.join_channel(command.id).await;
    commander.join_channel(flight.id).await;  // Monitor
    
    // Squadron leaders
    let alpha_lead = connect_test_client(&server, "alpha_lead", org.id).await;
    alpha_lead.join_channel(command.id).await;
    alpha_lead.join_channel(alpha.id).await;
    
    // Pilots
    let alpha_pilot1 = connect_test_client(&server, "alpha_1", org.id).await;
    alpha_pilot1.join_channel(alpha.id).await;
    
    // Scenario: Commander issues orders
    commander.transmit_to_channel(
        command.id,
        "All wings, prepare for jump"
    ).await;
    
    // Verify all leaders received
    assert!(alpha_lead.received_transmission_from(commander.id));
    
    // Squadron leader relays to squadron
    alpha_lead.transmit_to_channel(
        alpha.id,
        "Alpha wing, form up"
    ).await;
    
    // Verify pilots received
    assert!(alpha_pilot1.received_transmission_from(alpha_lead.id));
    
    // Check audio ducking applied correctly
    // (Command channel should duck Flight/Squadron channels)
}
```

---

## 6. Cross-Platform Tests

### Scope
Verify compatibility across operating systems.

### Test Matrix

| OS | Audio API | Test Coverage |
|-----------|-----------|---------------|
| Windows 10/11 | WASAPI (PortAudio) | Full |
| Linux (Ubuntu) | ALSA/PulseAudio | Full (Phase 2) |
| macOS | CoreAudio | Future |

### Platform-Specific Tests
- Device enumeration accuracy
- Hotkey registration (OS APIs differ)
- Overlay compatibility (DX11/Vulkan/OpenGL)
- Audio latency measurements

---

## 7. Bug Triage & Fix Workflow

### Standard Process

**1. Clarify Symptom**
- Expected behavior
- Observed behavior
- Reproduction steps
- Logs/error messages

**2. Locate Layer**
Determine if bug is in:
- Audio I/O
- Codec
- Network
- Jitter buffer
- Routing
- ACL/permissions
- UI/hotkeys

**3. Add Instrumentation**
- Log sequence numbers and timestamps
- Log jitter buffer levels
- Log per-channel packet counts
- Enable TRACE logging

**4. Form Hypothesis**
- Specific root cause theory
- Why this explains the symptom

**5. Implement Fix**
- Minimal change targeting root cause
- Add regression test

**6. Verify**
- Run new regression test
- Run full test suite
- Manual verification

---

## Testing Tools

### Client Testing
- **C++ Unit Tests**: Google Test
- **Mock Audio**: Virtual audio cable or loopback
- **Packet Capture**: Wireshark with custom dissector

### Server Testing
- **Rust Unit Tests**: cargo test
- **Integration**: tokio-test
- **Load Testing**: custom load generator or k6
- **Monitoring**: Prometheus + Grafana

### Network Simulation
- **tc** (Linux traffic control)
- **Custom UDP proxy** with configurable loss/jitter
- **netem** (network emulator)

---

## Continuous Integration

### CI Pipeline

```yaml
# .github/workflows/ci.yml
on: [push, pull_request]

jobs:
  test-client:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build
        run: cmake --build build
      - name: Run Unit Tests
        run: ctest --output-on-failure
      - name: Code Coverage
        run: gcovr --xml coverage.xml
  
  test-server:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run Tests
        run: cargo test --all-features
      - name: Code Coverage
        run: cargo tarpaulin --out Xml
```

---

## Performance Benchmarks

### Regular Benchmarks

**Audio Engine:**
- Capture latency: <10ms target
- Playback latency: <10ms target
- CPU usage: <5% per stream

**Codec:**
- Encode time: <2ms per frame
- Decode time: <2ms per frame

**Network:**
- Routing latency: <5ms (p95)
- Throughput: >10,000 packets/sec

### Regression Detection
- Run benchmarks in CI
- Alert on >10% performance regression

---

See `ARCHITECTURE.md` for system overview and component details.
