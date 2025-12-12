#pragma once

#include <array>
#include <vector>
#include <memory>
#include <cstdint>

namespace voip::crypto {

/// Key material derived from Diffie-Hellman key exchange
struct KeyMaterial {
    std::array<uint8_t, 16> master_key;  // AES-128 key
    std::array<uint8_t, 14> salt;        // SRTP salt
};

/// X25519 Diffie-Hellman key exchange for SRTP
///
/// Uses Curve25519 for ephemeral key exchange, matching the server implementation.
/// Derives SRTP keys using HKDF-SHA256.
class KeyExchange {
public:
    /// Create a new key exchange with random ephemeral keys
    KeyExchange();
    ~KeyExchange();

    // No copying (contains sensitive key material)
    KeyExchange(const KeyExchange&) = delete;
    KeyExchange& operator=(const KeyExchange&) = delete;

    /// Get our public key to send to peer (32 bytes)
    std::array<uint8_t, 32> public_key_bytes() const;

    /// Derive SRTP keys from peer's public key
    /// @param peer_public_key The peer's X25519 public key (32 bytes)
    /// @return Key material (master key + salt) for SRTP
    KeyMaterial derive_keys(const std::array<uint8_t, 32>& peer_public_key);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace voip::crypto
