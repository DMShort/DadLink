use crate::error::{Result, VoipError};
use crate::types::{VoicePacket, VoicePacketHeader, VOICE_PACKET_MAGIC, VOICE_PACKET_HEADER_SIZE};
use crate::routing::VoiceRouter;
use crate::channel_manager::ChannelManager;
use crate::crypto::SrtpSessionManager;
use tokio::net::UdpSocket;
use std::net::SocketAddr;
use std::sync::Arc;
use tracing::{debug, warn, error, info};

/// UDP Voice Server
///
/// Handles incoming voice packets and routes them to channel members
pub struct UdpVoiceServer {
    socket: Arc<UdpSocket>,
    router: Arc<VoiceRouter>,
    channel_manager: Arc<ChannelManager>,
    srtp_sessions: Arc<SrtpSessionManager>,
}

impl UdpVoiceServer {
    /// Create and bind UDP voice server
    pub async fn bind(
        addr: SocketAddr,
        router: Arc<VoiceRouter>,
        channel_manager: Arc<ChannelManager>,
        srtp_sessions: Arc<SrtpSessionManager>,
    ) -> Result<Self> {
        let socket = UdpSocket::bind(addr).await?;
        info!("🎤 UDP voice server listening on {}", socket.local_addr()?);

        // Set socket options for optimal voice traffic
        socket.set_broadcast(false)?;

        Ok(Self {
            socket: Arc::new(socket),
            router,
            channel_manager,
            srtp_sessions,
        })
    }
    
    /// Start receiving packets (spawns background task)
    pub fn start(self: Arc<Self>) -> tokio::task::JoinHandle<()> {
        tokio::spawn(async move {
            self.receive_loop().await;
        })
    }
    
    /// Main receive loop
    async fn receive_loop(&self) {
        let mut buffer = vec![0u8; 2048]; // Max UDP packet size
        
        loop {
            match self.socket.recv_from(&mut buffer).await {
                Ok((len, peer_addr)) => {
                    debug!("Received {} bytes from {}", len, peer_addr);
                    
                    // Parse packet
                    match Self::parse_voice_packet(&buffer[..len]) {
                        Ok(packet) => {
                            // Copy packed struct fields to avoid alignment issues
                            let seq = packet.header.sequence;
                            let ch = packet.header.channel_id;
                            let user = packet.header.user_id;
                            let payload_len = packet.encrypted_payload.len();

                            debug!("📦 Voice packet: seq={}, ch={}, user={}, payload={}B",
                                  seq, ch, user, payload_len);

                            // Register this UDP address for the user
                            self.router.register_udp_address(user, peer_addr).await;

                            // Decrypt the payload if user has an SRTP session
                            let plaintext_audio = match self.srtp_sessions.get_session(user).await {
                                Some(srtp_session) => {
                                    // Decrypt the encrypted payload using SRTP
                                    match srtp_session.decrypt(&packet.encrypted_payload).await {
                                        Ok(plaintext) => {
                                            debug!("🔓 Decrypted {} bytes from user {}", plaintext.len(), user);
                                            plaintext
                                        }
                                        Err(e) => {
                                            warn!("Failed to decrypt packet from user {}: {:?}", user, e);
                                            continue; // Skip this packet
                                        }
                                    }
                                }
                                None => {
                                    // No SRTP session - allow unencrypted for development
                                    debug!("⚠️ No SRTP session for user {}, using unencrypted audio (development mode)", user);
                                    packet.encrypted_payload.clone() // Treat as plaintext
                                }
                            };

                            // Get all users in the channel from ChannelManager
                            let channel_users = self.channel_manager.get_channel_users(ch).await;

                            // Build list of users to send to (exclude sender)
                            let mut sent_count = 0;
                            for channel_user in &channel_users {
                                if channel_user.id != user {
                                    // Get SRTP session for recipient
                                    match self.srtp_sessions.get_session(channel_user.id).await {
                                        Some(recipient_srtp) => {
                                            // Re-encrypt for this recipient
                                            match recipient_srtp.encrypt(&plaintext_audio, seq as u32) {
                                                Ok(encrypted_for_recipient) => {
                                                    // Create new packet for recipient
                                                    let recipient_packet = VoicePacket {
                                                        header: packet.header,
                                                        encrypted_payload: encrypted_for_recipient,
                                                    };

                                                    // Look up UDP address for this user
                                                    if let Some(udp_addr) = self.router.get_udp_address(channel_user.id).await {
                                                        if let Ok(data) = Self::serialize_packet(&recipient_packet) {
                                                            match self.socket.send_to(&data, udp_addr).await {
                                                                Ok(_) => {
                                                                    sent_count += 1;
                                                                }
                                                                Err(e) => {
                                                                    warn!("Failed to send to {}: {:?}", udp_addr, e);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                Err(e) => {
                                                    warn!("Failed to encrypt for user {}: {:?}", channel_user.id, e);
                                                }
                                            }
                                        }
                                        None => {
                                            // No SRTP session for recipient - send unencrypted (development mode)
                                            debug!("⚠️ No SRTP session for recipient user {}, sending unencrypted", channel_user.id);

                                            // Send plaintext packet to recipient
                                            let recipient_packet = VoicePacket {
                                                header: packet.header,
                                                encrypted_payload: plaintext_audio.clone(),
                                            };

                                            if let Some(udp_addr) = self.router.get_udp_address(channel_user.id).await {
                                                if let Ok(data) = Self::serialize_packet(&recipient_packet) {
                                                    match self.socket.send_to(&data, udp_addr).await {
                                                        Ok(_) => {
                                                            sent_count += 1;
                                                        }
                                                        Err(e) => {
                                                            warn!("Failed to send to {}: {:?}", udp_addr, e);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            if sent_count == 0 {
                                debug!("👤 User {} alone in channel {} (or others not ready)", user, ch);
                            } else {
                                info!("🔊 Routed encrypted voice from user {} to {} recipients in channel {}",
                                      user, sent_count, ch);
                            }
                        }
                        Err(e) => {
                            warn!("Invalid packet from {}: {:?}", peer_addr, e);
                        }
                    }
                }
                Err(e) => {
                    error!("UDP receive error: {:?}", e);
                }
            }
        }
    }
    
    /// Parse raw bytes into VoicePacket
    fn parse_voice_packet(data: &[u8]) -> Result<VoicePacket> {
        if data.len() < VOICE_PACKET_HEADER_SIZE {
            return Err(VoipError::InvalidPacket(
                format!("Packet too small: {} bytes", data.len())
            ));
        }
        
        // Parse header (unsafe pointer cast for performance)
        // In production, use proper deserialization
        let header = unsafe {
            std::ptr::read_unaligned(data.as_ptr() as *const VoicePacketHeader)
        };
        
        // Verify magic number
        if u32::from_be(header.magic) != VOICE_PACKET_MAGIC {
            return Err(VoipError::InvalidPacket(
                format!("Invalid magic: 0x{:08x}", u32::from_be(header.magic))
            ));
        }
        
        // Convert header fields from network byte order
        let header = VoicePacketHeader {
            magic: u32::from_be(header.magic),
            sequence: u64::from_be(header.sequence),
            timestamp: u64::from_be(header.timestamp),
            channel_id: u32::from_be(header.channel_id),
            user_id: u32::from_be(header.user_id),
        };
        
        // Extract encrypted payload
        let encrypted_payload = data[VOICE_PACKET_HEADER_SIZE..].to_vec();
        
        if encrypted_payload.is_empty() {
            return Err(VoipError::InvalidPacket("No payload".to_string()));
        }
        
        Ok(VoicePacket {
            header,
            encrypted_payload,
        })
    }
    
    /// Send voice packet to peer
    pub async fn send_to(&self, packet: &VoicePacket, addr: SocketAddr) -> Result<()> {
        let data = Self::serialize_packet(packet)?;
        
        match self.socket.send_to(&data, addr).await {
            Ok(sent) => {
                debug!("Sent {} bytes to {}", sent, addr);
                Ok(())
            }
            Err(e) => {
                Err(VoipError::IoError(e))
            }
        }
    }
    
    /// Serialize voice packet to bytes
    fn serialize_packet(packet: &VoicePacket) -> Result<Vec<u8>> {
        let mut data = Vec::with_capacity(VOICE_PACKET_HEADER_SIZE + packet.encrypted_payload.len());
        
        // Convert header to network byte order
        let header = VoicePacketHeader {
            magic: packet.header.magic.to_be(),
            sequence: packet.header.sequence.to_be(),
            timestamp: packet.header.timestamp.to_be(),
            channel_id: packet.header.channel_id.to_be(),
            user_id: packet.header.user_id.to_be(),
        };
        
        // Serialize header (unsafe for performance)
        let header_bytes = unsafe {
            std::slice::from_raw_parts(
                &header as *const VoicePacketHeader as *const u8,
                VOICE_PACKET_HEADER_SIZE
            )
        };
        
        data.extend_from_slice(header_bytes);
        data.extend_from_slice(&packet.encrypted_payload);
        
        Ok(data)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_parse_valid_packet() {
        let mut data = vec![0u8; 128];
        
        // Create header
        let header = VoicePacketHeader {
            magic: VOICE_PACKET_MAGIC.to_be(),
            sequence: 42u64.to_be(),
            timestamp: 1000000u64.to_be(),
            channel_id: 1u32.to_be(),
            user_id: 100u32.to_be(),
        };
        
        // Write header
        unsafe {
            let header_ptr = &header as *const VoicePacketHeader as *const u8;
            let header_slice = std::slice::from_raw_parts(header_ptr, VOICE_PACKET_HEADER_SIZE);
            data[..VOICE_PACKET_HEADER_SIZE].copy_from_slice(header_slice);
        }
        
        // Add payload
        data[VOICE_PACKET_HEADER_SIZE..].fill(0xFF);
        
        let packet = UdpVoiceServer::parse_voice_packet(&data).unwrap();
        
        // Copy fields to avoid packed struct alignment issues
        let magic = packet.header.magic;
        let sequence = packet.header.sequence;
        let timestamp = packet.header.timestamp;
        let channel_id = packet.header.channel_id;
        let user_id = packet.header.user_id;
        
        assert_eq!(magic, VOICE_PACKET_MAGIC);
        assert_eq!(sequence, 42);
        assert_eq!(timestamp, 1000000);
        assert_eq!(channel_id, 1);
        assert_eq!(user_id, 100);
        assert_eq!(packet.encrypted_payload.len(), 128 - VOICE_PACKET_HEADER_SIZE);
    }
    
    #[test]
    fn test_parse_invalid_magic() {
        let mut data = vec![0u8; 128];
        data[0..4].copy_from_slice(&[0xDE, 0xAD, 0xBE, 0xEF]); // Wrong magic
        
        let result = UdpVoiceServer::parse_voice_packet(&data);
        assert!(result.is_err());
    }
    
    #[test]
    fn test_parse_too_small() {
        let data = vec![0u8; 10]; // Too small
        
        let result = UdpVoiceServer::parse_voice_packet(&data);
        assert!(result.is_err());
    }
    
    #[tokio::test]
    async fn test_serialize_deserialize() {
        let packet = VoicePacket {
            header: VoicePacketHeader {
                magic: VOICE_PACKET_MAGIC,
                sequence: 123,
                timestamp: 999999,
                channel_id: 5,
                user_id: 42,
            },
            encrypted_payload: vec![1, 2, 3, 4, 5],
        };
        
        let data = UdpVoiceServer::serialize_packet(&packet).unwrap();
        let parsed = UdpVoiceServer::parse_voice_packet(&data).unwrap();
        
        // Copy fields to avoid packed struct alignment issues
        let magic = parsed.header.magic;
        let sequence = parsed.header.sequence;
        let timestamp = parsed.header.timestamp;
        let channel_id = parsed.header.channel_id;
        let user_id = parsed.header.user_id;
        
        assert_eq!(magic, VOICE_PACKET_MAGIC);
        assert_eq!(sequence, 123);
        assert_eq!(timestamp, 999999);
        assert_eq!(channel_id, 5);
        assert_eq!(user_id, 42);
        assert_eq!(parsed.encrypted_payload, vec![1, 2, 3, 4, 5]);
    }
}
