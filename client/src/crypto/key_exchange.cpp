#include "crypto/key_exchange.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

namespace voip::crypto {

class KeyExchange::Impl {
public:
    EVP_PKEY* pkey = nullptr;

    Impl() {
        // Generate X25519 keypair
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
        if (!pctx) {
            throw std::runtime_error("Failed to create X25519 context");
        }

        if (EVP_PKEY_keygen_init(pctx) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            throw std::runtime_error("Failed to init X25519 keygen");
        }

        if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
            EVP_PKEY_CTX_free(pctx);
            throw std::runtime_error("Failed to generate X25519 keypair");
        }

        EVP_PKEY_CTX_free(pctx);
    }

    ~Impl() {
        if (pkey) {
            EVP_PKEY_free(pkey);
        }
    }
};

KeyExchange::KeyExchange() : pImpl_(std::make_unique<Impl>()) {
    std::cout << "🔑 Generated X25519 keypair for SRTP" << std::endl;
}

KeyExchange::~KeyExchange() = default;

std::array<uint8_t, 32> KeyExchange::public_key_bytes() const {
    std::array<uint8_t, 32> pub_key{};
    size_t len = 32;

    if (EVP_PKEY_get_raw_public_key(pImpl_->pkey, pub_key.data(), &len) <= 0 || len != 32) {
        throw std::runtime_error("Failed to get X25519 public key");
    }

    return pub_key;
}

KeyMaterial KeyExchange::derive_keys(const std::array<uint8_t, 32>& peer_public_key) {
    // Create peer's public key object
    EVP_PKEY* peer_pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                                       peer_public_key.data(), 32);
    if (!peer_pkey) {
        throw std::runtime_error("Failed to create peer public key");
    }

    // Derive shared secret
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pImpl_->pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(peer_pkey);
        throw std::runtime_error("Failed to create derivation context");
    }

    if (EVP_PKEY_derive_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peer_pkey);
        throw std::runtime_error("Failed to init key derivation");
    }

    if (EVP_PKEY_derive_set_peer(ctx, peer_pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peer_pkey);
        throw std::runtime_error("Failed to set peer key");
    }

    // Get shared secret length
    size_t secret_len = 0;
    if (EVP_PKEY_derive(ctx, nullptr, &secret_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peer_pkey);
        throw std::runtime_error("Failed to get shared secret length");
    }

    // Derive shared secret
    std::vector<uint8_t> shared_secret(secret_len);
    if (EVP_PKEY_derive(ctx, shared_secret.data(), &secret_len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peer_pkey);
        throw std::runtime_error("Failed to derive shared secret");
    }

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peer_pkey);

    // Derive SRTP keys using HKDF-SHA256 (matching server)
    KeyMaterial km;

    // Derive master key
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!kctx) {
        throw std::runtime_error("Failed to create HKDF context");
    }

    if (EVP_PKEY_derive_init(kctx) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to init HKDF");
    }

    if (EVP_PKEY_CTX_set_hkdf_md(kctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to set HKDF MD");
    }

    if (EVP_PKEY_CTX_set1_hkdf_key(kctx, shared_secret.data(), shared_secret.size()) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to set HKDF key");
    }

    // Derive master key
    const char* master_key_info = "SRTP master key";
    if (EVP_PKEY_CTX_add1_hkdf_info(kctx, (const unsigned char*)master_key_info, strlen(master_key_info)) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to set HKDF info");
    }

    size_t master_key_len = 16;
    if (EVP_PKEY_derive(kctx, km.master_key.data(), &master_key_len) <= 0 || master_key_len != 16) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to derive master key");
    }

    EVP_PKEY_CTX_free(kctx);

    // Derive salt (need new context)
    kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!kctx) {
        throw std::runtime_error("Failed to create HKDF context for salt");
    }

    if (EVP_PKEY_derive_init(kctx) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to init HKDF for salt");
    }

    if (EVP_PKEY_CTX_set_hkdf_md(kctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to set HKDF MD for salt");
    }

    if (EVP_PKEY_CTX_set1_hkdf_key(kctx, shared_secret.data(), shared_secret.size()) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to set HKDF key for salt");
    }

    const char* salt_info = "SRTP master salt";
    if (EVP_PKEY_CTX_add1_hkdf_info(kctx, (const unsigned char*)salt_info, strlen(salt_info)) <= 0) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to set HKDF info for salt");
    }

    size_t salt_len = 14;
    if (EVP_PKEY_derive(kctx, km.salt.data(), &salt_len) <= 0 || salt_len != 14) {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("Failed to derive salt");
    }

    EVP_PKEY_CTX_free(kctx);

    std::cout << "🔐 SRTP key material derived successfully" << std::endl;

    return km;
}

} // namespace voip::crypto
