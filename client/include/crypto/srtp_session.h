#pragma once

#include <array>
#include <vector>
#include <memory>
#include <cstdint>

namespace voip::crypto {

/// SRTP session for encrypting and decrypting voice packets
///
/// Uses AES-128-GCM for encryption with replay protection.
/// Matches the server's SRTP implementation.
class SrtpSession {
public:
    /// Create SRTP session with key material
    /// @param master_key 16-byte AES-128 master key
    /// @param salt 14-byte SRTP salt
    SrtpSession(const std::array<uint8_t, 16>& master_key,
                const std::array<uint8_t, 14>& salt);
    ~SrtpSession();

    // No copying (contains cipher state)
    SrtpSession(const SrtpSession&) = delete;
    SrtpSession& operator=(const SrtpSession&) = delete;

    /// Encrypt voice data
    /// @param plaintext Raw voice data to encrypt
    /// @param sequence Packet sequence number
    /// @return Encrypted packet: [seq(4 bytes) | ciphertext | auth_tag(16 bytes)]
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, uint32_t sequence);

    /// Decrypt voice data
    /// @param encrypted Encrypted packet from encrypt()
    /// @return Decrypted voice data, or empty vector on failure
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encrypted);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace voip::crypto
