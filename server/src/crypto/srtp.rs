use crate::error::{Result, VoipError};
use aes_gcm::{
    aead::{Aead, KeyInit, OsRng},
    Aes128Gcm, Nonce,
};
use rand::RngCore;
use std::sync::Arc;
use tokio::sync::Mutex;

/// SRTP session for encrypting/decrypting voice packets
pub struct SrtpSession {
    /// Encryption cipher
    cipher: Aes128Gcm,
    /// Salt for key derivation
    salt: [u8; 14],
    /// Replay protection window
    replay_window: Arc<Mutex<ReplayWindow>>,
    /// Session ID for logging
    session_id: u64,
}

/// Replay attack protection
struct ReplayWindow {
    /// Highest sequence number seen
    max_seq: u64,
    /// Bitmap of recently seen sequence numbers
    bitmap: u64,
}

impl SrtpSession {
    /// Create a new SRTP session with a master key
    pub fn new(master_key: &[u8], salt: &[u8], session_id: u64) -> Result<Self> {
        if master_key.len() != 16 {
            return Err(VoipError::Other(
                "SRTP master key must be 16 bytes (AES-128)".to_string(),
            ));
        }

        if salt.len() != 14 {
            return Err(VoipError::Other("SRTP salt must be 14 bytes".to_string()));
        }

        let cipher = Aes128Gcm::new_from_slice(master_key)
            .map_err(|e| VoipError::Other(format!("Failed to create cipher: {}", e)))?;

        let mut salt_array = [0u8; 14];
        salt_array.copy_from_slice(salt);

        Ok(Self {
            cipher,
            salt: salt_array,
            replay_window: Arc::new(Mutex::new(ReplayWindow::new())),
            session_id,
        })
    }

    /// Generate a random master key and salt
    pub fn generate_key_material() -> ([u8; 16], [u8; 14]) {
        let mut master_key = [0u8; 16];
        let mut salt = [0u8; 14];

        OsRng.fill_bytes(&mut master_key);
        OsRng.fill_bytes(&mut salt);

        (master_key, salt)
    }

    /// Encrypt a voice packet
    ///
    /// Packet format:
    /// - 4 bytes: sequence number (big-endian)
    /// - N bytes: plaintext audio data
    ///
    /// Encrypted format:
    /// - 4 bytes: sequence number (big-endian, not encrypted)
    /// - N bytes: ciphertext
    /// - 16 bytes: authentication tag
    pub fn encrypt(&self, plaintext: &[u8], sequence: u32) -> Result<Vec<u8>> {
        // Create nonce from sequence number and salt
        let nonce = self.derive_nonce(sequence);
        let nonce = Nonce::from_slice(&nonce);

        // Encrypt the payload
        let ciphertext = self
            .cipher
            .encrypt(nonce, plaintext)
            .map_err(|e| VoipError::Other(format!("SRTP encryption failed: {}", e)))?;

        // Build encrypted packet: [seq(4) | ciphertext | tag]
        let mut encrypted = Vec::with_capacity(4 + ciphertext.len());
        encrypted.extend_from_slice(&sequence.to_be_bytes());
        encrypted.extend_from_slice(&ciphertext);

        Ok(encrypted)
    }

    /// Decrypt a voice packet
    pub async fn decrypt(&self, encrypted: &[u8]) -> Result<Vec<u8>> {
        if encrypted.len() < 4 + 16 {
            // Minimum: 4 bytes seq + 16 bytes tag
            return Err(VoipError::Other("SRTP packet too short".to_string()));
        }

        // Extract sequence number
        let sequence = u32::from_be_bytes([encrypted[0], encrypted[1], encrypted[2], encrypted[3]]);

        // Check for replay attack
        {
            let mut window = self.replay_window.lock().await;
            if !window.check_and_update(sequence as u64) {
                return Err(VoipError::Other(format!(
                    "SRTP replay attack detected: sequence {}",
                    sequence
                )));
            }
        }

        // Create nonce
        let nonce = self.derive_nonce(sequence);
        let nonce = Nonce::from_slice(&nonce);

        // Decrypt (ciphertext includes auth tag)
        let ciphertext = &encrypted[4..];
        let plaintext = self
            .cipher
            .decrypt(nonce, ciphertext)
            .map_err(|e| {
                VoipError::Other(format!(
                    "SRTP decryption failed (session {}): {}",
                    self.session_id, e
                ))
            })?;

        Ok(plaintext)
    }

    /// Derive nonce from sequence number and salt
    /// SRTP nonce = salt XOR (sequence || padding)
    fn derive_nonce(&self, sequence: u32) -> [u8; 12] {
        let mut nonce = [0u8; 12];

        // Copy salt (first 12 bytes, we have 14 but only use 12 for AES-GCM)
        nonce.copy_from_slice(&self.salt[..12]);

        // XOR with sequence number (in bytes 8-11, leaving SSRC space)
        let seq_bytes = sequence.to_be_bytes();
        nonce[8] ^= seq_bytes[0];
        nonce[9] ^= seq_bytes[1];
        nonce[10] ^= seq_bytes[2];
        nonce[11] ^= seq_bytes[3];

        nonce
    }
}

impl ReplayWindow {
    fn new() -> Self {
        Self {
            max_seq: 0,
            bitmap: 0,
        }
    }

    /// Check if sequence number is valid and update window
    /// Returns true if packet should be accepted
    fn check_and_update(&mut self, seq: u64) -> bool {
        const WINDOW_SIZE: u64 = 64;

        // First packet
        if self.max_seq == 0 {
            self.max_seq = seq;
            self.bitmap = 1;
            return true;
        }

        // Calculate difference
        let diff = if seq > self.max_seq {
            // New packet ahead of window
            let diff = seq - self.max_seq;
            if diff < WINDOW_SIZE {
                // Shift window forward
                self.bitmap <<= diff;
                self.bitmap |= 1;
                self.max_seq = seq;
            } else {
                // Too far ahead, reset window
                self.bitmap = 1;
                self.max_seq = seq;
            }
            return true;
        } else {
            self.max_seq - seq
        };

        // Check if packet is within window
        if diff >= WINDOW_SIZE {
            // Too old, reject
            tracing::debug!("SRTP packet too old: seq={}, max={}", seq, self.max_seq);
            return false;
        }

        // Check if we've already seen this packet
        let mask = 1u64 << diff;
        if (self.bitmap & mask) != 0 {
            // Duplicate packet
            tracing::debug!("SRTP duplicate packet: seq={}", seq);
            return false;
        }

        // Mark as seen
        self.bitmap |= mask;
        true
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn test_srtp_encrypt_decrypt() {
        let (master_key, salt) = SrtpSession::generate_key_material();
        let session = SrtpSession::new(&master_key, &salt, 1).unwrap();

        let plaintext = b"Hello, SRTP!";
        let sequence = 12345;

        // Encrypt
        let encrypted = session.encrypt(plaintext, sequence).unwrap();
        assert!(encrypted.len() > plaintext.len()); // Includes seq + tag

        // Decrypt
        let decrypted = session.decrypt(&encrypted).await.unwrap();
        assert_eq!(decrypted, plaintext);
    }

    #[tokio::test]
    async fn test_srtp_replay_protection() {
        let (master_key, salt) = SrtpSession::generate_key_material();
        let session = SrtpSession::new(&master_key, &salt, 1).unwrap();

        let plaintext = b"Test packet";

        // Encrypt packet 1
        let encrypted1 = session.encrypt(plaintext, 1).unwrap();
        // Encrypt packet 2
        let encrypted2 = session.encrypt(plaintext, 2).unwrap();

        // Decrypt in order - should succeed
        assert!(session.decrypt(&encrypted1).await.is_ok());
        assert!(session.decrypt(&encrypted2).await.is_ok());

        // Try to replay packet 1 - should fail
        assert!(session.decrypt(&encrypted1).await.is_err());
    }

    #[tokio::test]
    async fn test_srtp_sequence_window() {
        let (master_key, salt) = SrtpSession::generate_key_material();
        let session = SrtpSession::new(&master_key, &salt, 1).unwrap();

        let plaintext = b"Test";

        // Send packets 1, 2, 3
        for seq in 1..=3 {
            let encrypted = session.encrypt(plaintext, seq).unwrap();
            assert!(session.decrypt(&encrypted).await.is_ok());
        }

        // Send packet 2 again (within window) - should be rejected as duplicate
        let encrypted2 = session.encrypt(plaintext, 2).unwrap();
        assert!(session.decrypt(&encrypted2).await.is_err());
    }

    #[test]
    fn test_nonce_derivation() {
        let (master_key, salt) = SrtpSession::generate_key_material();
        let session = SrtpSession::new(&master_key, &salt, 1).unwrap();

        let nonce1 = session.derive_nonce(100);
        let nonce2 = session.derive_nonce(101);

        // Nonces should be different for different sequence numbers
        assert_ne!(nonce1, nonce2);
    }
}
