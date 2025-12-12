#include "session/voice_session.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace voip::session {

VoiceSession::VoiceSession() = default;

VoiceSession::~VoiceSession() {
    shutdown();
}

Result<void> VoiceSession::initialize(const Config& config) {
    config_ = config;
    
    // Initialize audio engine
    audio_engine_ = std::make_unique<audio::AudioEngine>();
    AudioConfig audio_config;
    audio_config.sample_rate = config.sample_rate;
    audio_config.frame_size = config.frame_size;
    
    auto audio_result = audio_engine_->initialize(audio_config);
    if (!audio_result.is_ok()) {
        return Err<void>(audio_result.error().code(), 
                        "Failed to initialize audio: " + audio_result.error().message());
    }
    
    // Create Opus encoder
    OpusConfig opus_config;
    opus_config.sample_rate = config.sample_rate;
    opus_config.bitrate = config.bitrate;
    opus_config.enable_fec = config.enable_fec;
    opus_config.enable_dtx = config.enable_dtx;
    
    auto encoder_result = audio::OpusEncoder::create(opus_config);
    if (!encoder_result.is_ok()) {
        return Err<void>(encoder_result.error().code(),
                        "Failed to create encoder: " + encoder_result.error().message());
    }
    encoder_ = std::move(encoder_result.value());
    
    // Create Opus decoder
    auto decoder_result = audio::OpusDecoder::create(config.sample_rate, 1);
    if (!decoder_result.is_ok()) {
        return Err<void>(decoder_result.error().code(),
                        "Failed to create decoder: " + decoder_result.error().message());
    }
    decoder_ = std::move(decoder_result.value());
    
    // Create jitter buffer
    jitter_buffer_ = std::make_unique<audio::JitterBuffer>(
        config.jitter_buffer_frames,
        config.frame_size
    );
    
    // Create network socket
    network_ = std::make_unique<network::UdpVoiceSocket>();
    
    // Set up network receive callback
    network_->set_receive_callback([this](const network::VoicePacket& packet) {
        this->on_packet_received(packet);
    });
    
    // Connect to server
    auto connect_result = network_->connect(config.server_address, config.server_port);
    if (!connect_result.is_ok()) {
        return Err<void>(connect_result.error().code(),
                        "Failed to connect to server: " + connect_result.error().message());
    }
    
    // Allocate buffers
    capture_buffer_.resize(config.frame_size * config.channels);
    playback_buffer_.resize(config.frame_size * config.channels);
    encode_buffer_.resize(4000);  // Max Opus frame size
    
    std::cout << "VoiceSession initialized:\n";
    std::cout << "  Server: " << config.server_address << ":" << config.server_port << "\n";
    std::cout << "  Sample rate: " << config.sample_rate << " Hz\n";
    std::cout << "  Frame size: " << config.frame_size << " samples\n";
    std::cout << "  Bitrate: " << config.bitrate << " bps\n";
    std::cout << "  Channel ID: " << config.channel_id << "\n";
    std::cout << "  User ID: " << config.user_id << "\n";
    
    return Ok();
}

void VoiceSession::shutdown() {
    std::cout << "🔧 Shutting down voice session..." << std::endl;
    
    // Stop session if still active
    stop();
    
    // Disconnect network (this will close UDP socket)
    if (network_) {
        std::cout << "  📡 Disconnecting network..." << std::endl;
        network_->disconnect();
    }
    
    // Shutdown audio engine
    if (audio_engine_) {
        std::cout << "  🔊 Shutting down audio engine..." << std::endl;
        audio_engine_->shutdown();
    }
    
    // Clean up multi-channel buffers
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        channel_buffers_.clear();
        listening_channels_.clear();
        channel_muted_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(ptt_mutex_);
        ptt_channels_.clear();
    }
    
    // Clean up components
    network_.reset();
    jitter_buffer_.reset();
    decoder_.reset();
    encoder_.reset();
    audio_engine_.reset();
    
    std::cout << "✅ Voice session shutdown complete\n";
}

Result<void> VoiceSession::start() {
    if (active_) {
        return Err<void>(ErrorCode::AudioStreamFailed, "Session already active");
    }
    
    // Set up audio callbacks
    audio_engine_->set_capture_callback(
        [this](const float* pcm, size_t frames) {
            this->on_audio_captured(pcm, frames);
        }
    );
    
    audio_engine_->set_playback_callback(
        [this](float* pcm, size_t frames) {
            this->on_audio_playback_needed(pcm, frames);
        }
    );
    
    // Start audio capture
    auto capture_result = audio_engine_->start_capture();
    if (!capture_result.is_ok()) {
        return Err<void>(capture_result.error().code(),
                        "Failed to start capture: " + capture_result.error().message());
    }
    
    // Start audio playback
    auto playback_result = audio_engine_->start_playback();
    if (!playback_result.is_ok()) {
        auto _ = audio_engine_->stop_capture();
        return Err<void>(playback_result.error().code(),
                        "Failed to start playback: " + playback_result.error().message());
    }
    
    active_ = true;
    std::cout << "\n🎤 Voice session started - speak into your microphone!\n\n";
    
    return Ok();
}

void VoiceSession::stop() {
    if (!active_) {
        return;
    }
    
    std::cout << "🛑 Stopping voice session..." << std::endl;
    
    // Mark as inactive immediately to stop new packets
    active_ = false;
    
    // Stop audio capture first (no more outgoing audio)
    if (audio_engine_) {
        std::cout << "  ⏹️ Stopping audio capture..." << std::endl;
        auto _ = audio_engine_->stop_capture();
    }
    
    // Give pending packets time to be sent (100ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop audio playback
    if (audio_engine_) {
        std::cout << "  ⏹️ Stopping audio playback..." << std::endl;
        auto _ = audio_engine_->stop_playback();
    }
    
    std::cout << "✅ Voice session stopped\n";
}

bool VoiceSession::is_active() const noexcept {
    return active_;
}

float VoiceSession::get_input_level() const noexcept {
    if (!audio_engine_ || !active_) {
        return 0.0f;
    }
    return audio_engine_->get_input_level();
}

float VoiceSession::get_output_level() const noexcept {
    if (!audio_engine_ || !active_) {
        return 0.0f;
    }
    return audio_engine_->get_output_level();
}

void VoiceSession::set_muted(bool muted) noexcept {
    is_muted_ = muted;
}

void VoiceSession::set_deafened(bool deafened) noexcept {
    is_deafened_ = deafened;
}

void VoiceSession::set_user_id(UserId user_id) noexcept {
    config_.user_id = user_id;
    std::cout << "🆔 VoiceSession user ID updated to: " << user_id << std::endl;
}

void VoiceSession::send_presence_packet(ChannelId channel_id) {
    if (!network_ || !active_) {
        std::cout << "⚠️ Cannot send presence packet - network not ready or session inactive" << std::endl;
        return;
    }
    
    // Create minimal presence packet to register UDP address
    network::VoicePacket packet;
    packet.header.magic = VOICE_PACKET_MAGIC;
    packet.header.sequence = next_sequence_++;
    packet.header.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    packet.header.channel_id = channel_id;
    packet.header.user_id = config_.user_id;
    
    // Minimal payload (1 byte of silence)
    packet.encrypted_payload.resize(1, 0);
    
    std::cout << "📍 Sending UDP presence packet for channel " << channel_id 
              << " (user " << config_.user_id << ") to register address with server" << std::endl;
    
    auto send_result = network_->send_packet(packet);
    if (!send_result.is_ok()) {
        std::cout << "⚠️ Failed to send presence packet: " << send_result.error().message() << std::endl;
    } else {
        std::cout << "✅ Presence packet sent - UDP address should now be registered" << std::endl;
    }
}

bool VoiceSession::is_muted() const noexcept {
    return is_muted_;
}

bool VoiceSession::is_deafened() const noexcept {
    return is_deafened_;
}

audio::AudioEngine* VoiceSession::get_audio_engine() noexcept {
    return audio_engine_.get();
}

void VoiceSession::set_srtp_session(std::unique_ptr<crypto::SrtpSession> srtp_session) {
    std::lock_guard<std::mutex> lock(srtp_mutex_);
    srtp_session_ = std::move(srtp_session);
    std::cout << "🔒 SRTP session installed - voice encryption enabled" << std::endl;
}

VoiceSession::Stats VoiceSession::get_stats() const {
    Stats stats;
    
    stats.frames_captured = frames_captured_.load();
    stats.frames_played = frames_played_.load();
    stats.frames_encoded = frames_encoded_.load();
    stats.encode_errors = encode_errors_.load();
    stats.frames_decoded = frames_decoded_.load();
    stats.decode_errors = decode_errors_.load();
    stats.plc_frames = plc_frames_.load();
    stats.jitter_buffer_underruns = jitter_underruns_.load();
    
    if (network_) {
        auto net_stats = network_->get_stats();
        stats.packets_sent = net_stats.packets_sent;
        stats.packets_received = net_stats.packets_received;
        stats.network_errors = net_stats.send_errors + net_stats.receive_errors;
    }
    
    if (jitter_buffer_) {
        auto jb_stats = jitter_buffer_->get_stats();
        stats.jitter_ms = jb_stats.jitter_ms;
    }
    
    // Estimate latency: encoding + network + jitter buffer + decoding
    // Very rough estimate: 20ms capture + jitter buffer + 20ms playback
    stats.estimated_latency_ms = 40.0f + (stats.jitter_ms * 2.0f);
    
    return stats;
}

// Audio capture callback (runs in audio thread - must be RT-safe!)
void VoiceSession::on_audio_captured(const float* pcm, size_t frames) {
    static int capture_count = 0;
    if (capture_count++ % 100 == 0) {  // Print every 100 frames (every 2 seconds)
        std::cout << "🎤 Capturing audio: frame " << capture_count << std::endl;
    }
    
    if (!active_ || frames != config_.frame_size) {
        return;
    }
    
    // Don't transmit if muted
    if (is_muted_) {
        static int mute_warn_count = 0;
        if (mute_warn_count++ % 100 == 0) {  // Warn every 2 seconds
            std::cout << "⚠️ Audio muted - not transmitting (frame " << capture_count << ")" << std::endl;
        }
        return;
    }
    
    frames_captured_++;
    
    // Encode with Opus
    auto encode_result = encoder_->encode(pcm, frames);
    if (!encode_result.is_ok()) {
        encode_errors_++;
        return;
    }
    
    frames_encoded_++;
    auto& encoded = encode_result.value();
    
    // Determine which channels to transmit to
    // Hot mic channel (if set)
    ChannelId hot_mic = hot_mic_channel_.load();
    
    // Get active PTT channels (copy to avoid holding mutex in RT thread)
    std::set<ChannelId> ptt_targets;
    {
        std::lock_guard<std::mutex> lock(ptt_mutex_);
        ptt_targets = ptt_channels_;
    }
    
    // PTT OVERRIDES hot mic (don't transmit to hot mic if PTT is active)
    std::set<ChannelId> target_channels;
    if (!ptt_targets.empty()) {
        // PTT active → use only PTT channels
        target_channels = ptt_targets;
    } else if (hot_mic != 0) {
        // No PTT active → use hot mic channel
        target_channels.insert(hot_mic);
    }
    
    // Debug logging - track when targets change
    static std::set<ChannelId> last_targets;
    static int tx_count = 0;
    bool targets_changed = (target_channels != last_targets);
    
    // Log immediately when targets change, or every 50 frames (~1 second)
    if (targets_changed || (tx_count++ % 50 == 0)) {
        std::cout << "📡 Transmit targets: ";
        if (target_channels.empty()) {
            std::cout << "(none - will drop audio)";
        } else {
            std::cout << "Channels: ";
            for (auto ch : target_channels) {
                std::cout << ch << " ";
            }
            std::cout << "| Hot mic: " << (hot_mic != 0 ? std::to_string(hot_mic) : "off");
            std::cout << " | PTT: ";
            if (ptt_targets.empty()) {
                std::cout << "none";
            } else {
                for (auto ch : ptt_targets) {
                    std::cout << ch << " ";
                }
            }
        }
        std::cout << std::endl;
        last_targets = target_channels;
    }
    
    // If no targets, don't transmit
    if (target_channels.empty()) {
        return;
    }
    
    // Send to each target channel
    for (auto channel_id : target_channels) {
        network::VoicePacket packet;
        packet.header.magic = VOICE_PACKET_MAGIC;
        packet.header.sequence = next_sequence_++;
        packet.header.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count());
        packet.header.channel_id = channel_id;
        packet.header.user_id = config_.user_id;

        // Encrypt voice data with SRTP if session is available
        {
            std::lock_guard<std::mutex> lock(srtp_mutex_);
            if (srtp_session_) {
                // Encrypt opus-encoded voice data
                std::vector<uint8_t> plaintext(encoded.data.begin(), encoded.data.end());
                std::vector<uint8_t> encrypted = srtp_session_->encrypt(plaintext, packet.header.sequence);
                if (!encrypted.empty()) {
                    packet.encrypted_payload = std::move(encrypted);
                } else {
                    std::cerr << "❌ SRTP encryption failed, dropping packet" << std::endl;
                    continue;  // Skip this packet
                }
            } else {
                // Development mode: send unencrypted
                packet.encrypted_payload.assign(encoded.data.begin(), encoded.data.end());
            }
        }

        // Send to server (async, won't block)
        auto send_result = network_->send_packet(packet);
        if (!send_result.is_ok()) {
            // Track send errors
            static int send_error_count = 0;
            if (send_error_count++ % 10 == 0) {  // Warn every 10 errors
                std::cout << "⚠️ UDP send failed for channel " << channel_id 
                          << " (error count: " << send_error_count << ")" << std::endl;
            }
        }
    }
}

// Network receive callback (runs in network thread)
void VoiceSession::on_packet_received(const network::VoicePacket& packet) {
    static int recv_count = 0;
    if (recv_count++ % 50 == 0) {  // Print every 50 packets
        std::cout << "📥 Received packet: seq=" << packet.header.sequence 
                  << " ch=" << packet.header.channel_id 
                  << " user=" << packet.header.user_id << std::endl;
    }
    
    if (!active_) return;
    
    // Check if we're listening to this channel
    ChannelId channel_id = packet.header.channel_id;
    bool is_listening = false;
    bool is_muted = false;
    
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        is_listening = listening_channels_.count(channel_id) > 0;
        if (is_listening) {
            auto it = channel_muted_.find(channel_id);
            is_muted = (it != channel_muted_.end() && it->second);
        }
    }
    
    // Ignore packets from channels we're not listening to or have muted
    if (!is_listening || is_muted) {
        return;
    }

    // Decrypt voice data with SRTP if session is available
    std::vector<uint8_t> opus_data;
    {
        std::lock_guard<std::mutex> lock(srtp_mutex_);
        if (srtp_session_) {
            // Decrypt SRTP packet to get opus-encoded voice data
            opus_data = srtp_session_->decrypt(packet.encrypted_payload);
            if (opus_data.empty()) {
                std::cerr << "❌ SRTP decryption failed, dropping packet seq="
                          << packet.header.sequence << std::endl;
                decode_errors_++;
                return;
            }
        } else {
            // Development mode: use unencrypted payload
            opus_data.assign(packet.encrypted_payload.begin(), packet.encrypted_payload.end());
        }
    }

    // Decode Opus
    std::vector<float> decoded_samples(config_.frame_size * config_.channels);

    auto decode_result = decoder_->decode(
        opus_data.data(),
        opus_data.size(),
        decoded_samples.data(),
        config_.frame_size
    );
    
    if (!decode_result.is_ok()) {
        decode_errors_++;
        return;
    }
    
    frames_decoded_++;
    
    // Add to channel-specific jitter buffer
    audio::AudioPacket audio_packet;
    audio_packet.sequence = packet.header.sequence;
    audio_packet.timestamp = Timestamp(packet.header.timestamp);
    audio_packet.samples = std::move(decoded_samples);
    audio_packet.frame_size = config_.frame_size;
    
    // Get or create jitter buffer for this channel
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        auto it = channel_buffers_.find(channel_id);
        if (it != channel_buffers_.end()) {
            if (!it->second->push(std::move(audio_packet))) {
                // Buffer full - this is rare but can happen
            }
        }
    }
}

// Audio playback callback (runs in audio thread - must be RT-safe!)
void VoiceSession::on_audio_playback_needed(float* pcm, size_t frames) {
    if (!active_) {
        // Fill with silence
        std::fill(pcm, pcm + frames, 0.0f);
        return;
    }
    
    // Output silence if deafened
    if (is_deafened_) {
        std::fill(pcm, pcm + frames, 0.0f);
        return;
    }
    
    frames_played_++;
    
    // Mix audio from all listening channels
    mix_channels(pcm, frames);
}

// Mix audio from multiple channels (called from audio thread - RT-safe)
void VoiceSession::mix_channels(float* output, size_t frames) {
    // Start with silence
    std::fill(output, output + frames, 0.0f);
    
    // Get snapshot of channels (avoid holding mutex too long)
    std::vector<ChannelId> channels_to_mix;
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        for (auto channel_id : listening_channels_) {
            // Skip muted channels
            auto it = channel_muted_.find(channel_id);
            if (it != channel_muted_.end() && it->second) {
                continue;
            }
            channels_to_mix.push_back(channel_id);
        }
    }
    
    // Mix each channel
    std::vector<float> temp_buffer(frames);
    for (auto channel_id : channels_to_mix) {
        // Get jitter buffer for this channel
        audio::JitterBuffer* buffer = nullptr;
        {
            std::lock_guard<std::mutex> lock(channels_mutex_);
            auto it = channel_buffers_.find(channel_id);
            if (it != channel_buffers_.end()) {
                buffer = it->second.get();
            }
        }
        
        if (!buffer) continue;
        
        // Try to get audio from this channel
        auto packet_opt = buffer->pop();
        if (packet_opt.has_value()) {
            auto& packet = packet_opt.value();
            size_t samples_to_mix = (std::min)(frames, packet.samples.size());
            
            // Additive mixing with clipping
            for (size_t i = 0; i < samples_to_mix; i++) {
                output[i] += packet.samples[i];
                // Manual clamp to [-1.0, 1.0]
                if (output[i] > 1.0f) output[i] = 1.0f;
                else if (output[i] < -1.0f) output[i] = -1.0f;
            }
        }
    }
}

// === MULTI-CHANNEL CONTROL METHODS ===

Result<void> VoiceSession::join_channel(ChannelId channel_id) {
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        
        // Add to listening channels
        listening_channels_.insert(channel_id);
        channel_muted_[channel_id] = false;  // Not muted by default
        
        // Create jitter buffer for this channel
        channel_buffers_[channel_id] = std::make_unique<audio::JitterBuffer>(
            config_.jitter_buffer_frames,
            config_.frame_size
        );
        
        std::cout << "✅ Joined channel " << channel_id << " for listening\n";
    }
    
    // Send presence packet to register UDP address for this channel
    // This ensures the server knows our UDP address before we transmit
    if (active_ && network_) {
        send_presence_packet(channel_id);
    }
    
    return Ok();
}

Result<void> VoiceSession::leave_channel(ChannelId channel_id) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    
    // Remove from listening channels
    listening_channels_.erase(channel_id);
    channel_muted_.erase(channel_id);
    channel_buffers_.erase(channel_id);
    
    std::cout << "👋 Left channel " << channel_id << "\n";
    return Ok();
}

void VoiceSession::set_channel_muted(ChannelId channel_id, bool muted) {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    
    // Only allow muting channels we're listening to
    if (listening_channels_.count(channel_id) > 0) {
        channel_muted_[channel_id] = muted;
        std::cout << (muted ? "🔇" : "🔊") << " Channel " << channel_id 
                  << (muted ? " muted" : " unmuted") << "\n";
    }
}

bool VoiceSession::is_channel_muted(ChannelId channel_id) const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    auto it = channel_muted_.find(channel_id);
    return (it != channel_muted_.end() && it->second);
}

std::set<ChannelId> VoiceSession::get_joined_channels() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return listening_channels_;
}

void VoiceSession::set_hot_mic_channel(ChannelId channel_id) {
    hot_mic_channel_.store(channel_id);
    if (channel_id == 0) {
        std::cout << "🎤 Hot mic disabled\n";
    } else {
        std::cout << "🎤 Hot mic set to channel " << channel_id << "\n";
    }
}

ChannelId VoiceSession::get_hot_mic_channel() const noexcept {
    return hot_mic_channel_.load();
}

void VoiceSession::start_ptt(ChannelId channel_id) {
    {
        std::lock_guard<std::mutex> lock(ptt_mutex_);
        ptt_channels_.insert(channel_id);
        
        std::cout << "🎤 PTT started for channel " << channel_id;
        std::cout << " | Active PTT channels now: ";
        for (auto ch : ptt_channels_) {
            std::cout << ch << " ";
        }
        std::cout << " | Hot mic: " << (hot_mic_channel_.load() != 0 ? std::to_string(hot_mic_channel_.load()) : "off");
        std::cout << std::endl;
    }
    
    // Send presence packet to ensure UDP address is registered for this channel
    // Critical for multi-channel scenarios where user may not have joined the channel yet
    if (active_ && network_) {
        send_presence_packet(channel_id);
    }
}

void VoiceSession::stop_ptt(ChannelId channel_id) {
    std::lock_guard<std::mutex> lock(ptt_mutex_);
    ptt_channels_.erase(channel_id);
    
    std::cout << "🔇 PTT stopped for channel " << channel_id;
    std::cout << " | Remaining PTT channels: ";
    if (ptt_channels_.empty()) {
        std::cout << "(none)";
    } else {
        for (auto ch : ptt_channels_) {
            std::cout << ch << " ";
        }
    }
    std::cout << " | Hot mic: " << (hot_mic_channel_.load() != 0 ? std::to_string(hot_mic_channel_.load()) : "off");
    std::cout << std::endl;
}

std::set<ChannelId> VoiceSession::get_active_ptt_channels() const {
    std::lock_guard<std::mutex> lock(ptt_mutex_);
    return ptt_channels_;
}

} // namespace voip::session
