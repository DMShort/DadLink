#include "crypto/srtp_session.h"
#include <openssl/evp.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

// Portable endianness conversion
#ifdef _WIN32
#include <stdlib.h>
#define htobe32(x) _byteswap_ulong(x)
#define be32toh(x) _byteswap_ulong(x)
#else
#include <endian.h>
#endif

namespace voip::crypto {

class SrtpSession::Impl {
public:
    std::array<uint8_t, 16> master_key;
    std::array<uint8_t, 14> salt;
    EVP_CIPHER_CTX* enc_ctx = nullptr;
    EVP_CIPHER_CTX* dec_ctx = nullptr;

    // Replay protection
    uint64_t max_seq = 0;
    uint64_t bitmap = 0;
    static constexpr uint64_t WINDOW_SIZE = 64;

    Impl(const std::array<uint8_t, 16>& key, const std::array<uint8_t, 14>& s)
        : master_key(key), salt(s) {
        enc_ctx = EVP_CIPHER_CTX_new();
        dec_ctx = EVP_CIPHER_CTX_new();
        if (!enc_ctx || !dec_ctx) {
            throw std::runtime_error("Failed to create cipher contexts");
        }
    }

    ~Impl() {
        if (enc_ctx) EVP_CIPHER_CTX_free(enc_ctx);
        if (dec_ctx) EVP_CIPHER_CTX_free(dec_ctx);
    }

    // Derive nonce from sequence number (matching server logic)
    std::array<uint8_t, 12> derive_nonce(uint32_t sequence) {
        std::array<uint8_t, 12> nonce{};

        // Copy first 12 bytes of salt
        std::memcpy(nonce.data(), salt.data(), 12);

        // XOR sequence number into bytes 8-11
        uint32_t seq_be = htobe32(sequence);
        const uint8_t* seq_bytes = reinterpret_cast<const uint8_t*>(&seq_be);
        nonce[8] ^= seq_bytes[0];
        nonce[9] ^= seq_bytes[1];
        nonce[10] ^= seq_bytes[2];
        nonce[11] ^= seq_bytes[3];

        return nonce;
    }

    // Check and update replay window
    bool check_replay(uint32_t sequence) {
        uint64_t seq = static_cast<uint64_t>(sequence);

        // First packet
        if (max_seq == 0) {
            max_seq = seq;
            bitmap = 1;
            return true;
        }

        // New packet ahead of window
        if (seq > max_seq) {
            uint64_t diff = seq - max_seq;
            if (diff < WINDOW_SIZE) {
                bitmap <<= diff;
                bitmap |= 1;
            } else {
                bitmap = 1;
            }
            max_seq = seq;
            return true;
        }

        // Check if within window
        uint64_t diff = max_seq - seq;
        if (diff >= WINDOW_SIZE) {
            return false; // Too old
        }

        // Check if already seen
        uint64_t mask = 1ULL << diff;
        if ((bitmap & mask) != 0) {
            return false; // Duplicate
        }

        // Mark as seen
        bitmap |= mask;
        return true;
    }
};

SrtpSession::SrtpSession(const std::array<uint8_t, 16>& master_key,
                         const std::array<uint8_t, 14>& salt)
    : pImpl_(std::make_unique<Impl>(master_key, salt)) {
    std::cout << "🔒 SRTP session created" << std::endl;
}

SrtpSession::~SrtpSession() = default;

std::vector<uint8_t> SrtpSession::encrypt(const std::vector<uint8_t>& plaintext, uint32_t sequence) {
    // Derive nonce
    auto nonce = pImpl_->derive_nonce(sequence);

    // Initialize encryption
    if (EVP_EncryptInit_ex(pImpl_->enc_ctx, EVP_aes_128_gcm(), nullptr,
                           pImpl_->master_key.data(), nonce.data()) != 1) {
        std::cerr << "❌ Failed to init AES-GCM encryption" << std::endl;
        return {};
    }

    // Encrypt plaintext
    std::vector<uint8_t> ciphertext(plaintext.size());
    int len = 0;
    if (EVP_EncryptUpdate(pImpl_->enc_ctx, ciphertext.data(), &len,
                          plaintext.data(), plaintext.size()) != 1) {
        std::cerr << "❌ Failed to encrypt data" << std::endl;
        return {};
    }

    // Finalize encryption
    int final_len = 0;
    if (EVP_EncryptFinal_ex(pImpl_->enc_ctx, ciphertext.data() + len, &final_len) != 1) {
        std::cerr << "❌ Failed to finalize encryption" << std::endl;
        return {};
    }

    // Get authentication tag
    std::array<uint8_t, 16> tag{};
    if (EVP_CIPHER_CTX_ctrl(pImpl_->enc_ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        std::cerr << "❌ Failed to get GCM tag" << std::endl;
        return {};
    }

    // Build result: [seq(4) | ciphertext | tag(16)]
    std::vector<uint8_t> result;
    result.reserve(4 + ciphertext.size() + 16);

    // Add sequence number (big-endian)
    uint32_t seq_be = htobe32(sequence);
    const uint8_t* seq_bytes = reinterpret_cast<const uint8_t*>(&seq_be);
    result.insert(result.end(), seq_bytes, seq_bytes + 4);

    // Add ciphertext
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + len + final_len);

    // Add tag
    result.insert(result.end(), tag.begin(), tag.end());

    return result;
}

std::vector<uint8_t> SrtpSession::decrypt(const std::vector<uint8_t>& encrypted) {
    // Minimum size check: seq(4) + tag(16)
    if (encrypted.size() < 20) {
        std::cerr << "❌ SRTP packet too short: " << encrypted.size() << " bytes" << std::endl;
        return {};
    }

    // Extract sequence number (big-endian)
    uint32_t seq_be;
    std::memcpy(&seq_be, encrypted.data(), 4);
    uint32_t sequence = be32toh(seq_be);

    // Check replay
    if (!pImpl_->check_replay(sequence)) {
        std::cerr << "❌ SRTP replay attack detected: seq=" << sequence << std::endl;
        return {};
    }

    // Derive nonce
    auto nonce = pImpl_->derive_nonce(sequence);

    // Extract ciphertext (without seq and tag)
    size_t ciphertext_len = encrypted.size() - 4 - 16;
    const uint8_t* ciphertext_ptr = encrypted.data() + 4;

    // Extract tag
    const uint8_t* tag_ptr = encrypted.data() + encrypted.size() - 16;

    // Initialize decryption
    if (EVP_DecryptInit_ex(pImpl_->dec_ctx, EVP_aes_128_gcm(), nullptr,
                           pImpl_->master_key.data(), nonce.data()) != 1) {
        std::cerr << "❌ Failed to init AES-GCM decryption" << std::endl;
        return {};
    }

    // Decrypt ciphertext
    std::vector<uint8_t> plaintext(ciphertext_len);
    int len = 0;
    if (EVP_DecryptUpdate(pImpl_->dec_ctx, plaintext.data(), &len,
                          ciphertext_ptr, ciphertext_len) != 1) {
        std::cerr << "❌ Failed to decrypt data" << std::endl;
        return {};
    }

    // Set expected tag
    if (EVP_CIPHER_CTX_ctrl(pImpl_->dec_ctx, EVP_CTRL_GCM_SET_TAG, 16,
                            const_cast<uint8_t*>(tag_ptr)) != 1) {
        std::cerr << "❌ Failed to set GCM tag" << std::endl;
        return {};
    }

    // Finalize decryption (verifies tag)
    int final_len = 0;
    if (EVP_DecryptFinal_ex(pImpl_->dec_ctx, plaintext.data() + len, &final_len) != 1) {
        std::cerr << "❌ SRTP authentication failed (invalid tag)" << std::endl;
        return {};
    }

    plaintext.resize(len + final_len);
    return plaintext;
}

} // namespace voip::crypto
