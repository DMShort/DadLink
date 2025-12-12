# Client-Side Voice Encryption Implementation Guide

## Overview

The server now has full DTLS-SRTP voice encryption support. This guide explains how to implement the client-side encryption to enable end-to-end encrypted voice communication.

## Architecture

```
Client                   Server                   Other Clients
  |                        |                            |
  |--[1] Authenticate----->|                            |
  |<--[2] KeyExchangeInit--|                            |
  |--[3] KeyExchangeResp-->|                            |
  |                        |                            |
  |--[4] Encrypted Voice-->|--[5] Re-encrypted Voice--->|
```

### Key Exchange Flow

1. **Authentication**: Client authenticates with username/password or token
2. **Server Initiates**: Server sends `KeyExchangeInit` with its X25519 public key (32 bytes)
3. **Client Responds**: Client generates ephemeral keypair, derives shared secret, sends `KeyExchangeResponse`
4. **Encryption Ready**: Both sides have derived identical SRTP keys using HKDF

### Voice Packet Flow

1. **Client Encrypts**: Client encrypts audio using AES-128-GCM with derived SRTP keys
2. **Server Decrypts**: Server decrypts using sender's SRTP session
3. **Server Re-encrypts**: Server encrypts for each recipient using their individual keys
4. **Recipients Decrypt**: Each client decrypts with their own SRTP session

## Required Dependencies

Add to your client's dependencies (Rust example):

```toml
[dependencies]
aes-gcm = "0.10"
sha2 = "0.10"
hkdf = "0.12"
x25519-dalek = "2.0"
rand = "0.8"
```

For other languages, find equivalent crypto libraries:
- **JavaScript/TypeScript**: `@noble/curves` for X25519, `webcrypto` API for AES-GCM
- **Python**: `cryptography` library
- **Go**: `crypto/aes`, `golang.org/x/crypto/curve25519`

## Implementation Steps

### 1. Handle KeyExchangeInit Message

When you receive the `KeyExchangeInit` message after authentication:

```rust
match message {
    ControlMessage::KeyExchangeInit { public_key } => {
        // Store server's public key
        let server_public_key = public_key; // [u8; 32]

        // Generate ephemeral keypair
        let client_kx = KeyExchange::new();
        let client_public_key = client_kx.public_key_bytes();

        // Derive shared secret and SRTP keys
        let key_material = client_kx.derive_keys(&server_public_key)?;

        // Create SRTP session
        let srtp_session = SrtpSession::new(
            &key_material.master_key,
            &key_material.salt,
            user_id as u64,
        )?;

        // Store for voice encryption/decryption
        self.srtp_session = Some(srtp_session);

        // Send response to server
        send_message(ControlMessage::KeyExchangeResponse {
            public_key: client_public_key,
        })?;
    }
}
```

### 2. Implement SRTP Session (same as server)

You can reuse the server's SRTP implementation or port it to your client language:

**Key components:**
- **Master Key**: 16 bytes (AES-128)
- **Salt**: 14 bytes
- **Nonce Derivation**: XOR salt with (sequence_number || padding)
- **Packet Format**: `[sequence(4 bytes BE) | ciphertext | auth_tag(16 bytes)]`
- **Replay Protection**: 64-packet sliding window bitmap

### 3. Implement Key Exchange (same as server)

**X25519 Diffie-Hellman:**
```rust
// Generate ephemeral secret
let secret = EphemeralSecret::random_from_rng(OsRng);
let public = PublicKey::from(&secret);

// Perform key exchange
let shared_secret = secret.diffie_hellman(&peer_public);

// Derive SRTP keys using HKDF-SHA256
let hk = Hkdf::<Sha256>::new(None, shared_secret.as_bytes());
hk.expand(b"SRTP master key", &mut master_key)?;
hk.expand(b"SRTP master salt", &mut salt)?;
```

### 4. Encrypt Outgoing Voice

Before sending voice packets to the server:

```rust
fn send_voice_packet(&mut self, audio_data: &[u8], channel_id: u32) -> Result<()> {
    // Get SRTP session
    let srtp = self.srtp_session.as_ref()
        .ok_or("No SRTP session established")?;

    // Increment sequence number
    self.sequence_number += 1;

    // Encrypt audio data
    let encrypted_payload = srtp.encrypt(audio_data, self.sequence_number as u32)?;

    // Build voice packet
    let packet = VoicePacket {
        header: VoicePacketHeader {
            magic: VOICE_PACKET_MAGIC,
            sequence: self.sequence_number,
            timestamp: get_timestamp_us(),
            channel_id,
            user_id: self.user_id,
        },
        encrypted_payload,
    };

    // Serialize and send via UDP
    let data = serialize_packet(&packet)?;
    self.udp_socket.send_to(&data, server_voice_addr)?;

    Ok(())
}
```

### 5. Decrypt Incoming Voice

When receiving voice packets from the server:

```rust
fn receive_voice_packet(&mut self, data: &[u8]) -> Result<Vec<u8>> {
    // Parse packet
    let packet = parse_voice_packet(data)?;

    // Get SRTP session
    let srtp = self.srtp_session.as_ref()
        .ok_or("No SRTP session established")?;

    // Decrypt payload (includes replay protection)
    let plaintext_audio = srtp.decrypt(&packet.encrypted_payload).await?;

    // plaintext_audio is now ready for playback
    Ok(plaintext_audio)
}
```

## Message Definitions

Add to your client's control message types:

```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum ControlMessage {
    // ... existing messages ...

    // Server -> Client: Initiate key exchange
    KeyExchangeInit {
        public_key: [u8; 32],
    },

    // Client -> Server: Complete key exchange
    KeyExchangeResponse {
        public_key: [u8; 32],
    },
}
```

## Testing Checklist

- [ ] Key exchange completes successfully after authentication
- [ ] SRTP keys are derived identically on client and server
- [ ] Outgoing voice is encrypted before sending
- [ ] Incoming voice is decrypted successfully
- [ ] Replay attack protection works (duplicate packets rejected)
- [ ] Sequence number wrapping is handled
- [ ] Multiple clients can communicate in same channel
- [ ] Audio quality is preserved (no data corruption)

## Security Considerations

### ✅ What's Protected

- **Voice Data**: All UDP voice packets are encrypted with AES-128-GCM
- **Perfect Forward Secrecy**: New ephemeral keys for each session
- **Replay Protection**: 64-packet sliding window prevents replay attacks
- **Authentication**: GCM provides authenticated encryption (AEAD)

### ⚠️ Current Limitations

1. **Server Can Decrypt**: Server is a trusted intermediary (decrypt-and-re-encrypt model)
2. **No Certificate Validation**: Using self-signed TLS certs (for now)
3. **No Key Rotation**: Keys are per-session, rotate on reconnect
4. **Control Channel**: WebSocket control messages rely on TLS

### 🔒 Future Enhancements

- **E2E Encryption**: Direct client-to-client encryption (server can't decrypt)
- **Key Rotation**: Periodic re-keying during long sessions
- **Certificate Pinning**: Validate server certificates
- **Encrypted Metadata**: Hide channel_id and user_id in voice packets

## Troubleshooting

### "No SRTP session established"

- Ensure you handle `KeyExchangeInit` message
- Verify you send `KeyExchangeResponse` with your public key
- Check server logs for key exchange completion

### "Decryption failed"

- Verify both sides derived same keys (log master_key hash)
- Check sequence number ordering
- Ensure packet format matches: `[seq(4) | ciphertext | tag(16)]`

### "Replay attack detected"

- This is normal for out-of-order or duplicate packets
- UDP can deliver packets out of order
- The 64-packet window handles this

### Audio is garbled

- Decryption likely succeeded but audio codec mismatch
- Verify you're using the same codec (Opus recommended)
- Check sample rate and channel count match

## Example: Complete Client Flow

```rust
impl VoipClient {
    async fn connect(&mut self) -> Result<()> {
        // 1. Connect WebSocket
        self.ws = connect_websocket(&self.server_control_addr).await?;

        // 2. Authenticate
        send_message(ControlMessage::Authenticate {
            method: "password".into(),
            username: Some(self.username.clone()),
            password: Some(self.password.clone()),
            token: None,
        })?;

        // 3. Wait for AuthResult
        let msg = receive_message().await?;
        match msg {
            ControlMessage::AuthResult { success: true, user_id, .. } => {
                self.user_id = user_id.unwrap();
            }
            _ => return Err("Auth failed"),
        }

        // 4. Wait for KeyExchangeInit
        let msg = receive_message().await?;
        match msg {
            ControlMessage::KeyExchangeInit { public_key } => {
                self.complete_key_exchange(public_key)?;
            }
            _ => return Err("Expected key exchange"),
        }

        // 5. Connect UDP socket
        self.udp = bind_udp_socket().await?;

        // 6. Ready to send/receive encrypted voice!
        Ok(())
    }

    fn complete_key_exchange(&mut self, server_pubkey: [u8; 32]) -> Result<()> {
        let kx = KeyExchange::new();
        let client_pubkey = kx.public_key_bytes();

        let key_material = kx.derive_keys(&server_pubkey)?;

        self.srtp_session = Some(SrtpSession::new(
            &key_material.master_key,
            &key_material.salt,
            self.user_id as u64,
        )?);

        send_message(ControlMessage::KeyExchangeResponse {
            public_key: client_pubkey,
        })?;

        Ok(())
    }
}
```

## Reference Implementation

The server implementation can be found in:
- `server/src/crypto/key_exchange.rs` - X25519 + HKDF key exchange
- `server/src/crypto/srtp.rs` - SRTP encryption/decryption
- `server/src/network/tls.rs` - Key exchange protocol handling
- `server/src/network/udp.rs` - Voice packet encryption/decryption

You can port these to your client language or use them as reference for implementation.

## Next Steps

1. Implement the crypto primitives (X25519, HKDF, AES-GCM, SRTP)
2. Add key exchange message handling
3. Integrate encryption into voice send/receive
4. Test with the server
5. Review security documentation

For questions or issues, refer to the security documentation or server implementation.
