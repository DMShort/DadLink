#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace voip {

// Type aliases for clarity
using UserId = uint32_t;
using ChannelId = uint32_t;
using OrgId = uint32_t;
using RoleId = uint32_t;
using SequenceNumber = uint64_t;
using Timestamp = std::chrono::microseconds;
using DeviceId = int32_t;

// Audio configuration
struct AudioConfig {
    uint32_t sample_rate = 48000;      // Hz
    uint32_t frame_size = 960;         // samples (20ms @ 48kHz)
    uint32_t buffer_frames = 3;        // Total buffering (60ms)
    float input_volume = 1.0f;         // 0.0-2.0
    float output_volume = 0.8f;        // 0.0-2.0
    
    [[nodiscard]] constexpr uint32_t frame_duration_ms() const noexcept {
        return (frame_size * 1000) / sample_rate;
    }
};

// Audio device information
struct AudioDevice {
    DeviceId id;
    std::string name;
    int max_input_channels;
    int max_output_channels;
    double default_sample_rate;
    bool is_default;
};

// Audio statistics
struct AudioStats {
    uint64_t input_overflows = 0;
    uint64_t output_underflows = 0;
    uint64_t queue_full_errors = 0;
    uint64_t queue_empty_errors = 0;
    float current_input_level = 0.0f;
    float current_output_level = 0.0f;
    uint32_t estimated_latency_ms = 0;
};

// Opus codec configuration
struct OpusConfig {
    uint32_t sample_rate = 48000;      // Hz (8000, 12000, 16000, 24000, 48000)
    uint32_t channels = 1;             // Mono for bandwidth savings
    uint32_t bitrate = 32000;          // bps (6000-510000)
    int complexity = 10;                // 0-10 (10 = best quality)
    bool enable_fec = true;             // Forward error correction
    bool enable_dtx = false;            // Discontinuous transmission (silence suppression)
    uint32_t expected_packet_loss = 5; // Percentage
};

// Network configuration
struct NetworkConfig {
    std::string server_address;
    uint16_t control_port = 9000;
    uint16_t voice_port = 9001;
    uint32_t connect_timeout_ms = 5000;
    uint32_t keepalive_interval_ms = 30000;
};

// Voice packet header - MUST match server's packed layout exactly!
// Use #pragma pack to prevent compiler from adding padding bytes
#pragma pack(push, 1)
struct VoicePacketHeader {
    uint32_t magic;                    // Protocol identifier (offset 0, 4 bytes)
    SequenceNumber sequence;           // Monotonic counter (offset 4, 8 bytes)
    uint64_t timestamp;                // Microseconds (offset 12, 8 bytes)
    ChannelId channel_id;              // Channel ID (offset 20, 4 bytes)
    UserId user_id;                    // User ID (offset 24, 4 bytes)
};                                     // Total: 28 bytes (no padding!)
#pragma pack(pop)

constexpr uint32_t VOICE_PACKET_MAGIC = 0x564F4950; // 'VOIP'
constexpr size_t VOICE_PACKET_HEADER_SIZE = 28;
constexpr size_t AES_GCM_NONCE_SIZE = 12;
constexpr size_t AES_GCM_TAG_SIZE = 16;

} // namespace voip
