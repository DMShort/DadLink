#pragma once

#include "audio/audio_engine.h"
#include "audio/opus_codec.h"
#include "audio/jitter_buffer.h"
#include "network/udp_socket.h"
#include "crypto/srtp_session.h"
#include "common/types.h"
#include "common/result.h"
#include <atomic>
#include <memory>
#include <map>
#include <set>
#include <mutex>

namespace voip::session {

/**
 * VoiceSession - Manages complete voice transmission pipeline
 * 
 * Integrates all components:
 * - Audio capture → Opus encode → UDP send
 * - UDP receive → Jitter buffer → Opus decode → Audio playback
 * 
 * Handles:
 * - Sequence number tracking
 * - Timestamp generation
 * - Packet loss detection
 * - Audio synchronization
 */
class VoiceSession {
public:
    struct Config {
        // Audio config
        uint32_t sample_rate = 48000;
        uint32_t channels = 1;
        uint32_t frame_size = 960;  // 20ms at 48kHz
        
        // Opus config
        uint32_t bitrate = 32000;
        bool enable_fec = true;
        bool enable_dtx = false;
        
        // Network config
        std::string server_address = "127.0.0.1";
        uint16_t server_port = 9001;
        
        // Session config (DEPRECATED: use multi-channel methods)
        ChannelId channel_id = 1;  // Legacy: single channel mode
        UserId user_id = 42;  // TODO: Get from authentication
        
        // Multi-channel config
        bool multi_channel_mode = true;  // Enable multi-channel support
        
        // Jitter buffer config
        uint32_t jitter_buffer_frames = 5;  // ~100ms at 20ms/frame
    };
    
    VoiceSession();
    ~VoiceSession();
    
    // Disable copy
    VoiceSession(const VoiceSession&) = delete;
    VoiceSession& operator=(const VoiceSession&) = delete;
    
    /**
     * Initialize session with config
     */
    Result<void> initialize(const Config& config);
    
    /**
     * Shutdown session
     */
    void shutdown();
    
    /**
     * Start voice transmission
     * Begins capturing audio and sending to server
     */
    Result<void> start();
    
    /**
     * Stop voice transmission
     */
    void stop();
    
    /**
     * Check if session is active
     */
    [[nodiscard]] bool is_active() const noexcept;
    
    /**
     * Get current audio levels (0.0 - 1.0)
     */
    [[nodiscard]] float get_input_level() const noexcept;
    [[nodiscard]] float get_output_level() const noexcept;
    
    /**
     * Control microphone mute state
     * When muted, audio capture continues but encoding/transmission stops
     */
    void set_muted(bool muted) noexcept;
    
    /**
     * Control speaker deafen state
     * When deafened, audio reception continues but playback is silenced
     */
    void set_deafened(bool deafened) noexcept;
    
    /**
     * Get current mute/deafen state
     */
    [[nodiscard]] bool is_muted() const noexcept;
    [[nodiscard]] bool is_deafened() const noexcept;
    
    /**
     * Set user ID for voice packets
     * Should be called with authenticated user ID from server
     */
    void set_user_id(UserId user_id) noexcept;
    
    /**
     * Send UDP presence packet to register address with server
     * Call this when joining a channel to enable immediate voice routing
     * This ensures the server knows your UDP address before you transmit
     */
    void send_presence_packet(ChannelId channel_id);
    
    // === MULTI-CHANNEL SUPPORT ===
    
    /**
     * Join a channel for listening (doesn't affect transmit)
     */
    Result<void> join_channel(ChannelId channel_id);
    
    /**
     * Leave a channel (stop listening)
     */
    Result<void> leave_channel(ChannelId channel_id);
    
    /**
     * Mute/unmute a specific channel (stay joined, don't hear)
     */
    void set_channel_muted(ChannelId channel_id, bool muted);
    
    /**
     * Check if a channel is muted
     */
    [[nodiscard]] bool is_channel_muted(ChannelId channel_id) const;
    
    /**
     * Get all joined channels
     */
    [[nodiscard]] std::set<ChannelId> get_joined_channels() const;
    
    /**
     * Set hot mic channel (always transmitting)
     * Set to 0 to disable hot mic
     */
    void set_hot_mic_channel(ChannelId channel_id);
    
    /**
     * Get current hot mic channel (0 = none)
     */
    [[nodiscard]] ChannelId get_hot_mic_channel() const noexcept;
    
    /**
     * Start PTT (push-to-talk) for a specific channel
     * Can have multiple PTT channels active simultaneously
     */
    void start_ptt(ChannelId channel_id);
    
    /**
     * Stop PTT for a specific channel
     */
    void stop_ptt(ChannelId channel_id);
    
    /**
     * Get all active PTT channels
     */
    [[nodiscard]] std::set<ChannelId> get_active_ptt_channels() const;
    
    /**
     * Get audio engine for device configuration
     * Returns nullptr if not initialized
     */
    [[nodiscard]] audio::AudioEngine* get_audio_engine() noexcept;

    /**
     * Set SRTP session for encrypted voice transmission
     * Should be called after key exchange completes
     */
    void set_srtp_session(std::unique_ptr<crypto::SrtpSession> srtp_session);
    
    /**
     * Get current statistics
     */
    struct Stats {
        // Audio stats
        uint64_t frames_captured = 0;
        uint64_t frames_played = 0;
        
        // Encoding stats
        uint64_t frames_encoded = 0;
        uint64_t encode_errors = 0;
        
        // Network stats
        uint64_t packets_sent = 0;
        uint64_t packets_received = 0;
        uint64_t network_errors = 0;
        
        // Decoding stats
        uint64_t frames_decoded = 0;
        uint64_t decode_errors = 0;
        uint64_t plc_frames = 0;  // Packet loss concealment
        
        // Jitter buffer stats
        uint64_t jitter_buffer_underruns = 0;
        float jitter_ms = 0.0f;
        
        // Latency estimate (ms)
        float estimated_latency_ms = 0.0f;
    };
    
    [[nodiscard]] Stats get_stats() const;
    
private:
    // Audio capture callback (from audio thread)
    void on_audio_captured(const float* pcm, size_t frames);
    
    // Network receive callback (from network thread)
    void on_packet_received(const network::VoicePacket& packet);
    
    // Audio playback callback (from audio thread)
    void on_audio_playback_needed(float* pcm, size_t frames);
    
    // Multi-channel audio mixing
    void mix_channels(float* output, size_t frames);
    
    // Components
    std::unique_ptr<audio::AudioEngine> audio_engine_;
    std::unique_ptr<audio::OpusEncoder> encoder_;
    std::unique_ptr<audio::OpusDecoder> decoder_;  // Shared decoder for all channels
    std::unique_ptr<audio::JitterBuffer> jitter_buffer_;  // Legacy: single channel
    std::unique_ptr<network::UdpVoiceSocket> network_;
    
    // Multi-channel components
    std::map<ChannelId, std::unique_ptr<audio::JitterBuffer>> channel_buffers_;
    mutable std::mutex channels_mutex_;  // Protects channel state

    // SRTP encryption
    std::unique_ptr<crypto::SrtpSession> srtp_session_;
    mutable std::mutex srtp_mutex_;  // Protects SRTP session

    // Configuration
    Config config_;
    
    // State
    std::atomic<bool> active_{false};
    std::atomic<bool> is_muted_{false};
    std::atomic<bool> is_deafened_{false};
    std::atomic<SequenceNumber> next_sequence_{0};
    
    // Multi-channel state
    std::set<ChannelId> listening_channels_;        // Channels we're listening to
    std::map<ChannelId, bool> channel_muted_;       // Per-channel mute state
    std::atomic<ChannelId> hot_mic_channel_{0};     // Hot mic target (0 = none)
    std::set<ChannelId> ptt_channels_;              // Active PTT channels
    mutable std::mutex ptt_mutex_;                  // Protects PTT state
    
    // Statistics (atomic for thread safety)
    mutable std::atomic<uint64_t> frames_captured_{0};
    mutable std::atomic<uint64_t> frames_played_{0};
    mutable std::atomic<uint64_t> frames_encoded_{0};
    mutable std::atomic<uint64_t> encode_errors_{0};
    mutable std::atomic<uint64_t> frames_decoded_{0};
    mutable std::atomic<uint64_t> decode_errors_{0};
    mutable std::atomic<uint64_t> plc_frames_{0};
    mutable std::atomic<uint64_t> jitter_underruns_{0};
    
    // Temporary buffers for audio processing
    std::vector<float> capture_buffer_;
    std::vector<float> playback_buffer_;
    std::vector<uint8_t> encode_buffer_;
};

} // namespace voip::session
