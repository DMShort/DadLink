#pragma once

#include "common/types.h"
#include "common/result.h"
#include <opus/opus.h>
#include <vector>
#include <memory>

namespace voip::audio {

/**
 * Encoded Opus packet
 */
struct EncodedPacket {
    std::vector<uint8_t> data;
    uint32_t frame_size;
    bool is_dtx;  // Discontinuous transmission (silence)
};

/**
 * OpusEncoder wrapper - Encodes PCM audio to Opus
 * 
 * Thread Safety: Not thread-safe. Use one encoder per thread.
 */
class OpusEncoder {
public:
    /**
     * Create encoder with configuration
     */
    static Result<std::unique_ptr<OpusEncoder>> create(const OpusConfig& config);
    
    ~OpusEncoder();
    
    // Disable copy
    OpusEncoder(const OpusEncoder&) = delete;
    OpusEncoder& operator=(const OpusEncoder&) = delete;
    
    /**
     * Encode PCM audio to Opus
     * Input: PCM float samples (-1.0 to 1.0)
     * Returns: Encoded packet
     */
    Result<EncodedPacket> encode(const float* pcm, size_t frame_count);
    
    /**
     * Set bitrate (bits per second)
     */
    Result<void> set_bitrate(uint32_t bitrate);
    
    /**
     * Set complexity (0-10, higher = better quality, more CPU)
     */
    Result<void> set_complexity(int complexity);
    
    /**
     * Enable/disable DTX (Discontinuous Transmission)
     */
    Result<void> enable_dtx(bool enable);
    
    /**
     * Enable/disable FEC (Forward Error Correction)
     */
    Result<void> enable_fec(bool enable);
    
    /**
     * Set expected packet loss percentage (0-100)
     */
    Result<void> set_packet_loss_perc(int percentage);
    
private:
    explicit OpusEncoder(::OpusEncoder* encoder, const OpusConfig& config);
    
    ::OpusEncoder* encoder_;
    OpusConfig config_;
    std::vector<uint8_t> encode_buffer_;
};

/**
 * OpusDecoder wrapper - Decodes Opus to PCM audio
 * 
 * Thread Safety: Not thread-safe. Use one decoder per stream/source.
 */
class OpusDecoder {
public:
    /**
     * Create decoder with configuration
     */
    static Result<std::unique_ptr<OpusDecoder>> create(uint32_t sample_rate, uint32_t channels);
    
    ~OpusDecoder();
    
    // Disable copy
    OpusDecoder(const OpusDecoder&) = delete;
    OpusDecoder& operator=(const OpusDecoder&) = delete;
    
    /**
     * Decode Opus packet to PCM
     * Returns: Number of samples decoded
     */
    Result<size_t> decode(
        const uint8_t* opus_data,
        size_t opus_size,
        float* pcm_out,
        size_t max_frame_size
    );
    
    /**
     * Packet Loss Concealment - generate audio for missing packet
     * Call when expected packet is missing
     * Returns: Number of samples generated
     */
    Result<size_t> decode_plc(float* pcm_out, size_t frame_size);
    
private:
    explicit OpusDecoder(::OpusDecoder* decoder);
    
    ::OpusDecoder* decoder_;
};

} // namespace voip::audio
