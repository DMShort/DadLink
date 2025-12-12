use crate::error::{Result, VoipError};
use hkdf::Hkdf;
use rand::rngs::OsRng;
use sha2::Sha256;
use x25519_dalek::{EphemeralSecret, PublicKey};

/// Key material derived from key exchange
#[derive(Clone)]
pub struct KeyMaterial {
    /// Master key for SRTP (16 bytes for AES-128)
    pub master_key: [u8; 16],
    /// Salt for SRTP (14 bytes)
    pub salt: [u8; 14],
}

/// Handles Diffie-Hellman key exchange for SRTP
pub struct KeyExchange {
    /// Our ephemeral secret key
    secret: EphemeralSecret,
    /// Our public key to send to peer
    public: PublicKey,
}

impl KeyExchange {
    /// Create a new key exchange with random ephemeral keys
    pub fn new() -> Self {
        let secret = EphemeralSecret::random_from_rng(OsRng);
        let public = PublicKey::from(&secret);

        Self { secret, public }
    }

    /// Get our public key to send to the peer
    pub fn public_key(&self) -> &PublicKey {
        &self.public
    }

    /// Get public key as bytes
    pub fn public_key_bytes(&self) -> [u8; 32] {
        *self.public.as_bytes()
    }

    /// Perform key exchange with peer's public key and derive SRTP keys
    ///
    /// This implements HKDF key derivation from the shared secret
    pub fn derive_keys(self, peer_public_bytes: &[u8; 32]) -> Result<KeyMaterial> {
        // Parse peer's public key
        let peer_public = PublicKey::from(*peer_public_bytes);

        // Perform Diffie-Hellman key exchange
        let shared_secret = self.secret.diffie_hellman(&peer_public);

        // Derive SRTP master key and salt using HKDF
        // This follows RFC 5764 (DTLS-SRTP) key derivation
        let hk = Hkdf::<Sha256>::new(None, shared_secret.as_bytes());

        // Derive master key (16 bytes for AES-128)
        let mut master_key = [0u8; 16];
        hk.expand(b"SRTP master key", &mut master_key)
            .map_err(|e| VoipError::Other(format!("Key derivation failed: {}", e)))?;

        // Derive salt (14 bytes)
        let mut salt = [0u8; 14];
        hk.expand(b"SRTP master salt", &mut salt)
            .map_err(|e| VoipError::Other(format!("Salt derivation failed: {}", e)))?;

        tracing::info!("SRTP key material derived successfully");

        Ok(KeyMaterial { master_key, salt })
    }
}

impl Default for KeyExchange {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_key_exchange() {
        // Create two key exchange instances (client and server)
        let client_kx = KeyExchange::new();
        let server_kx = KeyExchange::new();

        // Exchange public keys and derive shared secrets
        let client_key_material = client_kx
            .derive_keys(&server_kx.public_key_bytes())
            .unwrap();

        let server_key_material = server_kx
            .derive_keys(&client_kx.public_key_bytes())
            .unwrap();

        // Both sides should derive the same key material
        assert_eq!(client_key_material.master_key, server_key_material.master_key);
        assert_eq!(client_key_material.salt, server_key_material.salt);
    }

    #[test]
    fn test_different_peers_different_keys() {
        let kx1 = KeyExchange::new();
        let kx2 = KeyExchange::new();
        let kx3 = KeyExchange::new();

        // Exchange between 1 and 2
        let keys_12 = kx1.derive_keys(&kx2.public_key_bytes()).unwrap();

        // Exchange between 2 and 3 (different pair)
        let keys_23 = kx2.derive_keys(&kx3.public_key_bytes()).unwrap();

        // Keys should be different
        assert_ne!(keys_12.master_key, keys_23.master_key);
    }

    #[test]
    fn test_public_key_bytes() {
        let kx = KeyExchange::new();
        let bytes = kx.public_key_bytes();

        // Public key should be 32 bytes for X25519
        assert_eq!(bytes.len(), 32);

        // Should not be all zeros
        assert_ne!(bytes, [0u8; 32]);
    }
}
