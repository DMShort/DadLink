#include "audio/opus_codec.h"

namespace voip::audio {

// OpusEncoder implementation

Result<std::unique_ptr<OpusEncoder>> OpusEncoder::create(const OpusConfig& config) {
    int error;
    ::OpusEncoder* encoder = opus_encoder_create(
        config.sample_rate,
        config.channels,
        OPUS_APPLICATION_VOIP,
        &error
    );
    
    if (error != OPUS_OK || !encoder) {
        return Err<std::unique_ptr<OpusEncoder>>(
            ErrorCode::AudioInitFailed,
            std::string("opus_encoder_create failed: ") + opus_strerror(error)
        );
    }
    
    auto wrapper = std::unique_ptr<OpusEncoder>(new OpusEncoder(encoder, config));
    
    // Apply configuration
    wrapper->set_bitrate(config.bitrate);
    wrapper->set_complexity(config.complexity);
    wrapper->enable_dtx(config.enable_dtx);
    wrapper->enable_fec(config.enable_fec);
    wrapper->set_packet_loss_perc(config.expected_packet_loss);
    
    // Set signal type to voice
    opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    
    return Ok(std::move(wrapper));
}

OpusEncoder::OpusEncoder(::OpusEncoder* encoder, const OpusConfig& config)
    : encoder_(encoder)
    , config_(config)
    , encode_buffer_(4000)  // Max Opus packet size
{
}

OpusEncoder::~OpusEncoder() {
    if (encoder_) {
        opus_encoder_destroy(encoder_);
    }
}

Result<EncodedPacket> OpusEncoder::encode(const float* pcm, size_t frame_count) {
    const int encoded_bytes = opus_encode_float(
        encoder_,
        pcm,
        static_cast<int>(frame_count),
        encode_buffer_.data(),
        static_cast<int>(encode_buffer_.size())
    );
    
    if (encoded_bytes < 0) {
        return Err<EncodedPacket>(
            ErrorCode::OpusEncodeFailed,
            std::string("opus_encode_float failed: ") + opus_strerror(encoded_bytes)
        );
    }
    
    EncodedPacket packet;
    packet.data.assign(encode_buffer_.begin(), encode_buffer_.begin() + encoded_bytes);
    packet.frame_size = static_cast<uint32_t>(frame_count);
    packet.is_dtx = (encoded_bytes <= 3);  // DTX packets are very small
    
    return Ok(std::move(packet));
}

Result<void> OpusEncoder::set_bitrate(uint32_t bitrate) {
    const int result = opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(bitrate));
    if (result != OPUS_OK) {
        return Err<void>(ErrorCode::OpusEncodeFailed,
                        std::string("OPUS_SET_BITRATE failed: ") + opus_strerror(result));
    }
    config_.bitrate = bitrate;
    return Ok();
}

Result<void> OpusEncoder::set_complexity(int complexity) {
    if (complexity < 0 || complexity > 10) {
        return Err<void>(ErrorCode::OpusEncodeFailed, "Complexity must be 0-10");
    }
    
    const int result = opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(complexity));
    if (result != OPUS_OK) {
        return Err<void>(ErrorCode::OpusEncodeFailed,
                        std::string("OPUS_SET_COMPLEXITY failed: ") + opus_strerror(result));
    }
    config_.complexity = static_cast<uint32_t>(complexity);
    return Ok();
}

Result<void> OpusEncoder::enable_dtx(bool enable) {
    const int result = opus_encoder_ctl(encoder_, OPUS_SET_DTX(enable ? 1 : 0));
    if (result != OPUS_OK) {
        return Err<void>(ErrorCode::OpusEncodeFailed,
                        std::string("OPUS_SET_DTX failed: ") + opus_strerror(result));
    }
    config_.enable_dtx = enable;
    return Ok();
}

Result<void> OpusEncoder::enable_fec(bool enable) {
    const int result = opus_encoder_ctl(encoder_, OPUS_SET_INBAND_FEC(enable ? 1 : 0));
    if (result != OPUS_OK) {
        return Err<void>(ErrorCode::OpusEncodeFailed,
                        std::string("OPUS_SET_INBAND_FEC failed: ") + opus_strerror(result));
    }
    config_.enable_fec = enable;
    return Ok();
}

Result<void> OpusEncoder::set_packet_loss_perc(int percentage) {
    if (percentage < 0 || percentage > 100) {
        return Err<void>(ErrorCode::OpusEncodeFailed, "Packet loss percentage must be 0-100");
    }
    
    const int result = opus_encoder_ctl(encoder_, OPUS_SET_PACKET_LOSS_PERC(percentage));
    if (result != OPUS_OK) {
        return Err<void>(ErrorCode::OpusEncodeFailed,
                        std::string("OPUS_SET_PACKET_LOSS_PERC failed: ") + opus_strerror(result));
    }
    config_.expected_packet_loss = static_cast<uint32_t>(percentage);
    return Ok();
}

// OpusDecoder implementation

Result<std::unique_ptr<OpusDecoder>> OpusDecoder::create(uint32_t sample_rate, uint32_t channels) {
    int error;
    ::OpusDecoder* decoder = opus_decoder_create(
        sample_rate,
        channels,
        &error
    );
    
    if (error != OPUS_OK || !decoder) {
        return Err<std::unique_ptr<OpusDecoder>>(
            ErrorCode::AudioInitFailed,
            std::string("opus_decoder_create failed: ") + opus_strerror(error)
        );
    }
    
    return Ok(std::unique_ptr<OpusDecoder>(new OpusDecoder(decoder)));
}

OpusDecoder::OpusDecoder(::OpusDecoder* decoder)
    : decoder_(decoder)
{
}

OpusDecoder::~OpusDecoder() {
    if (decoder_) {
        opus_decoder_destroy(decoder_);
    }
}

Result<size_t> OpusDecoder::decode(
    const uint8_t* opus_data,
    size_t opus_size,
    float* pcm_out,
    size_t max_frame_size
) {
    const int decoded_samples = opus_decode_float(
        decoder_,
        opus_data,
        static_cast<int>(opus_size),
        pcm_out,
        static_cast<int>(max_frame_size),
        0  // Not using FEC decoding
    );
    
    if (decoded_samples < 0) {
        return Err<size_t>(
            ErrorCode::OpusDecodeFailed,
            std::string("opus_decode_float failed: ") + opus_strerror(decoded_samples)
        );
    }
    
    return Ok(static_cast<size_t>(decoded_samples));
}

Result<size_t> OpusDecoder::decode_plc(float* pcm_out, size_t frame_size) {
    // Call opus_decode with NULL packet for PLC
    const int decoded_samples = opus_decode_float(
        decoder_,
        nullptr,  // NULL triggers PLC
        0,
        pcm_out,
        static_cast<int>(frame_size),
        0
    );
    
    if (decoded_samples < 0) {
        return Err<size_t>(
            ErrorCode::OpusDecodeFailed,
            std::string("opus_decode_float (PLC) failed: ") + opus_strerror(decoded_samples)
        );
    }
    
    return Ok(static_cast<size_t>(decoded_samples));
}

} // namespace voip::audio
