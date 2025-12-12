# Voice Encryption Testing Guide

## Overview

This guide provides step-by-step instructions for testing the DTLS-SRTP voice encryption implementation.

## Prerequisites

### Server Setup

1. **PostgreSQL Database Running:**
```bash
# Start PostgreSQL (if using Docker)
docker run --name voip-db -e POSTGRES_PASSWORD=password -p 5432:5432 -d postgres

# Or use system PostgreSQL
sudo systemctl start postgresql
```

2. **Database Initialized:**
```bash
cd server
cargo run -- migrate  # Run migrations
cargo run -- seed     # Optional: seed test data
```

3. **TLS Certificates Generated:**
```bash
cd server
./tools/generate_certs.sh

# Should create:
# - certs/server.crt
# - certs/server.key
```

4. **Environment Variables Set:**
```bash
# server/.env
DATABASE_URL=postgresql://voip:password@localhost/voip_db
JWT_SECRET=$(openssl rand -base64 32)
RUST_LOG=debug
```

### Client Setup

- Implement client-side encryption following `VOICE_ENCRYPTION_CLIENT_GUIDE.md`
- Or use the provided test client (if available)

## Test Plan

### Phase 1: Server Startup and Crypto Initialization

#### Test 1.1: Server Starts Successfully

**Steps:**
```bash
cd server
cargo build --release
cargo run
```

**Expected Output:**
```
🎤 VoIP Server v0.1.0
⚙️  Configuration loaded
   Control port: 8443
   Voice port: 9000
   Database: postgresql://****@localhost/voip_db
🔌 Initializing database...
✅ Database ready
✅ Repositories initialized
✅ Channel manager initialized
✅ Voice router initialized
✅ Permission checker initialized
✅ SRTP session manager initialized
✅ UDP voice server started
✅ TLS certificates loaded successfully
✅ WebSocket control server started
🚀 Server initialization complete

📡 Voice: UDP port 9000
🔐 Control: WebSocket port 8443
💾 Database: PostgreSQL connected
```

**Pass Criteria:**
- [ ] No errors during startup
- [ ] "SRTP session manager initialized" message appears
- [ ] Both UDP and WebSocket servers start

#### Test 1.2: Crypto Module Loads

**Verification:**
```bash
# Check server logs for crypto initialization
grep "SRTP" server.log
```

**Expected:**
- SRTP session manager initialized successfully
- No crypto-related errors

### Phase 2: Authentication and Key Exchange

#### Test 2.1: User Registration

**Client Request:**
```json
{
  "type": "register",
  "username": "alice",
  "password": "test123",
  "email": "alice@example.com"
}
```

**Server Response:**
```json
{
  "type": "register_result",
  "success": true,
  "user_id": 1,
  "message": "Registration successful! You can now login as 'alice'"
}
```

**Server Logs:**
```
✅ User alice registered successfully (ID: 1)
```

**Pass Criteria:**
- [ ] Registration succeeds
- [ ] Password is hashed with Argon2id (check database)
- [ ] User ID is returned

**Database Verification:**
```sql
SELECT id, username, length(password_hash) FROM users WHERE username = 'alice';
-- password_hash should be ~90 characters (Argon2id hash)
```

#### Test 2.2: User Authentication

**Client Request:**
```json
{
  "type": "authenticate",
  "method": "password",
  "username": "alice",
  "password": "test123"
}
```

**Server Response:**
```json
{
  "type": "auth_result",
  "success": true,
  "user_id": 1,
  "session_token": "eyJ0eXAiOiJKV1QiLCJhbGc...",
  "voice_key": "demo_voice_key",
  "message": "Authentication successful"
}
```

**Server Logs:**
```
✅ User alice (ID: 1) authenticated and registered
```

**Pass Criteria:**
- [ ] Authentication succeeds with correct password
- [ ] Authentication fails with incorrect password
- [ ] JWT token is returned

#### Test 2.3: Key Exchange Initialization

**Server Message (automatically sent after auth):**
```json
{
  "type": "key_exchange_init",
  "public_key": [1, 2, 3, ..., 32]  // 32-byte array
}
```

**Server Logs:**
```
🔑 Sent key exchange init to user 1
```

**Pass Criteria:**
- [ ] KeyExchangeInit message sent immediately after auth
- [ ] Public key is 32 bytes (X25519)
- [ ] Public key is different each time (ephemeral)

**Verification:**
```bash
# Server logs should show:
grep "key exchange init" server.log
```

#### Test 2.4: Key Exchange Completion

**Client Response:**
```json
{
  "type": "key_exchange_response",
  "public_key": [32, 31, 30, ..., 1]  // 32-byte array
}
```

**Server Logs:**
```
🔑 Received key exchange response from user 1
✅ SRTP session established for user 1
🔐 SRTP session established for user 1
```

**Pass Criteria:**
- [ ] Server receives client's public key
- [ ] Shared secret derived successfully
- [ ] SRTP session created
- [ ] No crypto errors in logs

**Database Verification:**
```bash
# Check that pending key exchange was removed
# (in-memory only, not in database)
```

### Phase 3: Voice Encryption

#### Test 3.1: Encrypt Voice Packet (Client Side)

**Client Code:**
```rust
let audio_data = vec![0u8; 960];  // 20ms of Opus audio at 48kHz
let encrypted = srtp_session.encrypt(&audio_data, sequence_number)?;

// Verify packet structure
assert_eq!(&encrypted[0..4], sequence_number.to_be_bytes());
assert_eq!(encrypted.len(), 4 + 960 + 16);  // seq + audio + GCM tag
```

**Pass Criteria:**
- [ ] Encryption succeeds
- [ ] Packet format is `[seq(4) | ciphertext(960) | tag(16)]`
- [ ] Sequence number is in big-endian
- [ ] Ciphertext is different from plaintext

#### Test 3.2: Send Encrypted Voice Packet

**Client:**
```rust
let packet = VoicePacket {
    header: VoicePacketHeader {
        magic: VOICE_PACKET_MAGIC,
        sequence: 1,
        timestamp: get_timestamp(),
        channel_id: 1,
        user_id: 1,
    },
    encrypted_payload,
};

udp_socket.send_to(&serialize_packet(&packet), server_voice_addr)?;
```

**Server Logs:**
```
📦 Voice packet: seq=1, ch=1, user=1, payload=980B
🔓 Decrypted 960 bytes from user 1
```

**Pass Criteria:**
- [ ] Packet received by server
- [ ] Packet parsed successfully
- [ ] Decryption succeeds
- [ ] No "No SRTP session" errors

#### Test 3.3: Decrypt Voice Packet (Server Side)

**Server Process:**
1. Receive UDP packet
2. Parse header and encrypted payload
3. Get SRTP session for user
4. Decrypt payload
5. Verify plaintext matches original audio

**Wireshark Capture:**
```bash
# Capture UDP packets on port 9000
sudo tcpdump -i lo -w voice.pcap 'port 9000'

# Analyze in Wireshark
wireshark voice.pcap
```

**Verify:**
- Payload after header is NOT plaintext
- Payload is encrypted (random-looking bytes)

**Pass Criteria:**
- [ ] Server decrypts packet successfully
- [ ] Plaintext audio recovered
- [ ] No decryption errors

#### Test 3.4: Re-encrypt for Recipients

**Setup:** Two clients (Alice and Bob) in same channel

**Flow:**
```
Alice                 Server                  Bob
  |                     |                      |
  |--Encrypted Audio--->|                      |
  |                     |--Decrypt(Alice key)->|
  |                     |                      |
  |                     |--Encrypt(Bob key)--->|
  |                     |                   Decrypt
  |                     |                   Playback
```

**Server Logs:**
```
📦 Voice packet: seq=1, ch=1, user=1, payload=980B
🔓 Decrypted 960 bytes from user 1
🔊 Routed encrypted voice from user 1 to 1 recipients in channel 1
```

**Pass Criteria:**
- [ ] Server decrypts Alice's packet
- [ ] Server re-encrypts for Bob (different ciphertext)
- [ ] Bob receives and decrypts successfully
- [ ] Audio matches original

**Verification:**
```bash
# Capture packets and verify ciphertext is different
# Alice's packet: [seq | ciphertext_A | tag_A]
# Bob's packet:   [seq | ciphertext_B | tag_B]
# ciphertext_A != ciphertext_B (different recipient keys)
```

### Phase 4: Security Tests

#### Test 4.1: Replay Attack Protection

**Attack:** Send duplicate packet with same sequence number

**Client:**
```rust
// Send same packet twice
let packet = create_voice_packet(seq=100, audio);
udp_socket.send_to(&packet, server)?;
udp_socket.send_to(&packet, server)?;  // Duplicate
```

**Server Logs:**
```
📦 Voice packet: seq=100, ch=1, user=1, payload=980B
🔓 Decrypted 960 bytes from user 1
📦 Voice packet: seq=100, ch=1, user=1, payload=980B
⚠️  Replay attack detected for user 1, sequence 100
```

**Pass Criteria:**
- [ ] First packet is accepted
- [ ] Second packet is rejected (replay detected)
- [ ] Warning logged

#### Test 4.2: Out-of-Order Packets

**Client:** Send packets out of order (within 64-packet window)

```rust
send_packet(seq=100);
send_packet(seq=102);
send_packet(seq=101);  // Out of order but within window
```

**Server:**
- Should accept all three packets
- No replay attack warnings

**Pass Criteria:**
- [ ] All packets accepted (within window)
- [ ] Bitmap correctly tracks received packets

#### Test 4.3: Old Packet Rejection

**Client:** Send very old packet (beyond 64-packet window)

```rust
send_packet(seq=100);
send_packet(seq=200);
send_packet(seq=50);  // Too old (200 - 50 > 64)
```

**Server Logs:**
```
⚠️  Replay attack detected (packet too old)
```

**Pass Criteria:**
- [ ] Old packet rejected
- [ ] No decryption attempted (fail fast)

#### Test 4.4: Tampered Packet Detection

**Attack:** Modify ciphertext

**Client:**
```rust
let mut encrypted = srtp.encrypt(&audio, seq);
encrypted[10] ^= 0xFF;  // Flip bits in ciphertext
send_packet_with_payload(encrypted);
```

**Server Logs:**
```
⚠️  Failed to decrypt packet from user 1: authentication failed
```

**Pass Criteria:**
- [ ] GCM authentication fails
- [ ] Packet rejected
- [ ] Error logged

#### Test 4.5: Wrong Key Attack

**Attack:** Client uses wrong key to encrypt

**Setup:**
- Client A has key_material_A
- Client B tries to decrypt with key_material_B

**Result:**
- Decryption fails (different keys)
- GCM tag verification fails

**Pass Criteria:**
- [ ] Decryption fails
- [ ] No plaintext leaked

### Phase 5: Integration Tests

#### Test 5.1: Complete Communication Flow

**Scenario:** Alice and Bob have a voice conversation in channel 1

**Steps:**
1. Alice registers and authenticates
2. Alice completes key exchange
3. Bob registers and authenticates
4. Bob completes key exchange
5. Alice joins channel 1
6. Bob joins channel 1
7. Alice sends voice packets
8. Bob receives and decrypts voice packets
9. Bob sends voice packets
10. Alice receives and decrypts voice packets

**Validation:**
- [ ] Both users establish SRTP sessions
- [ ] Voice packets encrypted end-to-end
- [ ] Audio quality preserved (no corruption)
- [ ] No decryption errors
- [ ] Sequence numbers increment correctly

#### Test 5.2: Multi-User Channel

**Scenario:** 4 users (Alice, Bob, Charlie, Dave) in same channel

**Server Load:**
- Each user sends 50 packets/second (20ms audio)
- Server processes: 4 * 50 = 200 packets/sec
- Server sends: 200 * 3 (recipients) = 600 packets/sec

**Metrics:**
- Latency: < 50ms (target)
- Packet loss: < 1%
- CPU usage: < 50%

**Pass Criteria:**
- [ ] All users can hear each other
- [ ] No encryption/decryption errors
- [ ] Acceptable latency
- [ ] No packet drops under normal load

#### Test 5.3: Session Reconnection

**Scenario:** Client disconnects and reconnects

**Steps:**
1. Client authenticates and establishes SRTP session
2. Client disconnects (close WebSocket)
3. Client reconnects (new WebSocket)
4. Client re-authenticates
5. New key exchange performed
6. New SRTP session established

**Pass Criteria:**
- [ ] Old SRTP session is cleaned up
- [ ] New session uses different keys (ephemeral)
- [ ] No key reuse
- [ ] Voice works with new session

### Phase 6: Performance Tests

#### Test 6.1: Encryption Performance

**Benchmark:** Time to encrypt 1000 packets

```rust
let audio = vec![0u8; 960];
let start = Instant::now();

for seq in 0..1000 {
    srtp.encrypt(&audio, seq)?;
}

let duration = start.elapsed();
println!("Encrypted 1000 packets in {:?}", duration);
```

**Target:** < 10ms total (100μs per packet)

**Pass Criteria:**
- [ ] Encryption is fast enough for real-time
- [ ] No performance degradation over time

#### Test 6.2: Decryption Performance

**Benchmark:** Time to decrypt 1000 packets

```rust
let encrypted_packets: Vec<_> = (0..1000)
    .map(|seq| srtp.encrypt(&audio, seq).unwrap())
    .collect();

let start = Instant::now();

for packet in &encrypted_packets {
    srtp.decrypt(packet).await?;
}

let duration = start.elapsed();
```

**Target:** < 10ms total (100μs per packet)

**Pass Criteria:**
- [ ] Decryption is fast enough
- [ ] No memory leaks

#### Test 6.3: Key Exchange Performance

**Benchmark:** Time to complete key exchange

```rust
let start = Instant::now();

let client_kx = KeyExchange::new();
let server_kx = KeyExchange::new();

let client_keys = client_kx.derive_keys(&server_kx.public_key_bytes())?;
let server_keys = server_kx.derive_keys(&client_kx.public_key_bytes())?;

let duration = start.elapsed();
```

**Target:** < 1ms

**Pass Criteria:**
- [ ] Key exchange completes quickly
- [ ] Both sides derive identical keys

### Phase 7: Stress Tests

#### Test 7.1: High Packet Rate

**Load:** 100 packets/second per user, 10 users

**Duration:** 5 minutes

**Metrics:**
- Packet loss rate
- Decryption error rate
- Memory usage
- CPU usage

**Pass Criteria:**
- [ ] < 1% packet loss
- [ ] 0 decryption errors
- [ ] Stable memory usage
- [ ] CPU < 80%

#### Test 7.2: Long Session

**Duration:** 1 hour continuous voice

**Metrics:**
- Sequence number handling (should wrap at u32::MAX)
- Memory leaks
- Session stability

**Pass Criteria:**
- [ ] No crashes
- [ ] Sequence wrapping handled correctly
- [ ] No memory leaks

#### Test 7.3: Rapid Reconnections

**Scenario:** Client connects and disconnects 100 times

**Metrics:**
- SRTP session cleanup
- Memory leaks
- Key exchange stability

**Pass Criteria:**
- [ ] All sessions cleaned up
- [ ] No memory leaks
- [ ] All key exchanges succeed

## Test Tools

### Manual Testing with Rust

```rust
#[cfg(test)]
mod integration_tests {
    use super::*;

    #[tokio::test]
    async fn test_complete_encryption_flow() {
        // 1. Create SRTP sessions
        let (key, salt) = SrtpSession::generate_key_material();
        let alice_session = SrtpSession::new(&key, &salt, 1).unwrap();
        let bob_session = SrtpSession::new(&key, &salt, 2).unwrap();

        // 2. Encrypt (Alice)
        let audio = vec![0x42; 960];
        let encrypted = alice_session.encrypt(&audio, 1).unwrap();

        // 3. Decrypt (Bob - using same keys)
        let decrypted = bob_session.decrypt(&encrypted).await.unwrap();

        // 4. Verify
        assert_eq!(audio, decrypted);
    }
}
```

### Wireshark Analysis

**Capture UDP voice packets:**
```bash
sudo tcpdump -i any -w voice.pcap 'udp port 9000'
```

**Analyze:**
- Open in Wireshark
- Filter: `udp.port == 9000`
- Verify packets are encrypted (random data after header)

### Logging Configuration

**Enable debug logging:**
```bash
# server/.env
RUST_LOG=voip_server=debug,voip_server::crypto=trace
```

**Key log messages:**
- `SRTP session established`
- `Decrypted X bytes`
- `Routed encrypted voice`
- `Replay attack detected`

## Common Issues and Debugging

### Issue: "No SRTP session established"

**Cause:** Client didn't complete key exchange

**Debug:**
1. Check client received `KeyExchangeInit`
2. Check client sent `KeyExchangeResponse`
3. Check server logs for "SRTP session established"

**Fix:** Ensure client implements key exchange handler

### Issue: "Decryption failed"

**Causes:**
1. Keys don't match (different shared secret)
2. Packet corrupted in transit
3. Wrong packet format

**Debug:**
```rust
// Log keys (only in debug mode!)
println!("Master key: {:?}", key_material.master_key);
println!("Salt: {:?}", key_material.salt);
// Compare client and server - should be identical
```

**Fix:**
- Verify both sides use same HKDF inputs
- Check packet format matches spec
- Ensure nonce derivation is identical

### Issue: "Replay attack detected" (false positive)

**Cause:** Packets arriving very out of order (> 64 packets)

**Debug:**
- Check network stability
- Reduce packet send rate
- Increase replay window size (not recommended)

**Fix:**
- Use UDP only on reliable networks
- Or accept some packet loss

### Issue: Audio is garbled

**Causes:**
1. Decryption succeeded but wrong codec
2. Sample rate mismatch
3. Endianness issue

**Debug:**
- Verify plaintext bytes match original audio
- Check codec settings match
- Verify byte order is correct

## Success Criteria

### Minimum Requirements

- [ ] Key exchange completes successfully
- [ ] Voice packets are encrypted
- [ ] Voice packets are decrypted successfully
- [ ] Replay protection works
- [ ] Multiple users can communicate
- [ ] No cleartext audio in network capture

### Production Ready

- [ ] All tests pass
- [ ] Performance meets targets
- [ ] No memory leaks
- [ ] Stress tests pass
- [ ] Security tests pass
- [ ] Documentation complete

## Next Steps

After testing is complete:
1. Update client implementation guide with lessons learned
2. Document any bugs found and fixes applied
3. Create production deployment guide
4. Plan for future enhancements (E2E encryption, key rotation)

## References

- `VOICE_ENCRYPTION_CLIENT_GUIDE.md` - Client implementation
- `SECURITY.md` - Security architecture
- `server/src/crypto/` - Server implementation reference

---

**Testing Checklist:**
- [ ] Phase 1: Server Startup (2/2 tests)
- [ ] Phase 2: Authentication & Key Exchange (4/4 tests)
- [ ] Phase 3: Voice Encryption (4/4 tests)
- [ ] Phase 4: Security Tests (5/5 tests)
- [ ] Phase 5: Integration Tests (3/3 tests)
- [ ] Phase 6: Performance Tests (3/3 tests)
- [ ] Phase 7: Stress Tests (3/3 tests)

**Total: 24 tests**

Good luck testing! 🔐🎤
