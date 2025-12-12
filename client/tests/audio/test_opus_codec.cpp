#include <gtest/gtest.h>
#include "audio/opus_codec.h"
#include <cmath>
#include <vector>

using namespace voip::audio;

// Helper: Generate sine wave test signal
std::vector<float> generate_sine_wave(float frequency, uint32_t sample_rate, size_t samples) {
    std::vector<float> signal(samples);
    const float angular_freq = 2.0f * M_PI * frequency;
    
    for (size_t i = 0; i < samples; ++i) {
        signal[i] = std::sin(angular_freq * static_cast<float>(i) / static_cast<float>(sample_rate));
    }
    
    return signal;
}

// Helper: Calculate correlation between two signals
float calculate_correlation(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return 0.0f;
    
    float sum = 0.0f;
    float sum_a_sq = 0.0f;
    float sum_b_sq = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        sum += a[i] * b[i];
        sum_a_sq += a[i] * a[i];
        sum_b_sq += b[i] * b[i];
    }
    
    const float denom = std::sqrt(sum_a_sq * sum_b_sq);
    return (denom > 0.0f) ? (sum / denom) : 0.0f;
}

TEST(OpusCodecTest, EncoderCreation) {
    OpusConfig config;
    config.sample_rate = 48000;
    config.channels = 1;
    config.bitrate = 32000;
    
    auto result = OpusEncoder::create(config);
    ASSERT_TRUE(result.is_ok());
    
    auto encoder = result.unwrap();
    ASSERT_NE(encoder, nullptr);
}

TEST(OpusCodecTest, DecoderCreation) {
    auto result = OpusDecoder::create(48000, 1);
    ASSERT_TRUE(result.is_ok());
    
    auto decoder = result.unwrap();
    ASSERT_NE(decoder, nullptr);
}

TEST(OpusCodecTest, EncodeDecodeRoundTrip) {
    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr size_t FRAME_SIZE = 960;  // 20ms @ 48kHz
    
    // Create encoder and decoder
    OpusConfig config;
    config.sample_rate = SAMPLE_RATE;
    config.channels = 1;
    config.bitrate = 32000;
    
    auto encoder_result = OpusEncoder::create(config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    auto decoder_result = OpusDecoder::create(SAMPLE_RATE, 1);
    ASSERT_TRUE(decoder_result.is_ok());
    auto decoder = decoder_result.unwrap();
    
    // Generate test signal (440 Hz sine wave)
    auto input = generate_sine_wave(440.0f, SAMPLE_RATE, FRAME_SIZE);
    
    // Encode
    auto encode_result = encoder->encode(input.data(), FRAME_SIZE);
    ASSERT_TRUE(encode_result.is_ok());
    auto packet = encode_result.unwrap();
    
    EXPECT_GT(packet.data.size(), 0);
    EXPECT_LE(packet.data.size(), 4000);  // Max Opus packet size
    EXPECT_EQ(packet.frame_size, FRAME_SIZE);
    
    // Decode
    std::vector<float> output(FRAME_SIZE);
    auto decode_result = decoder->decode(
        packet.data.data(),
        packet.data.size(),
        output.data(),
        FRAME_SIZE
    );
    ASSERT_TRUE(decode_result.is_ok());
    EXPECT_EQ(decode_result.value(), FRAME_SIZE);
    
    // Verify audio quality (correlation should be high)
    float correlation = calculate_correlation(input, output);
    EXPECT_GT(correlation, 0.9f) << "Audio quality degraded, correlation: " << correlation;
}

TEST(OpusCodecTest, BitrateConfiguration) {
    OpusConfig config;
    config.sample_rate = 48000;
    config.channels = 1;
    
    auto encoder_result = OpusEncoder::create(config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    // Test various bitrates
    for (uint32_t bitrate : {16000, 24000, 32000, 64000, 128000}) {
        auto result = encoder->set_bitrate(bitrate);
        EXPECT_TRUE(result.is_ok()) << "Failed to set bitrate: " << bitrate;
    }
}

TEST(OpusCodecTest, ComplexityConfiguration) {
    OpusConfig config;
    auto encoder_result = OpusEncoder::create(config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    // Test valid complexity values
    for (int complexity = 0; complexity <= 10; ++complexity) {
        auto result = encoder->set_complexity(complexity);
        EXPECT_TRUE(result.is_ok()) << "Failed to set complexity: " << complexity;
    }
    
    // Test invalid complexity values
    EXPECT_TRUE(encoder->set_complexity(-1).is_err());
    EXPECT_TRUE(encoder->set_complexity(11).is_err());
}

TEST(OpusCodecTest, PacketLossConcealment) {
    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr size_t FRAME_SIZE = 960;
    
    // Create decoder
    auto decoder_result = OpusDecoder::create(SAMPLE_RATE, 1);
    ASSERT_TRUE(decoder_result.is_ok());
    auto decoder = decoder_result.unwrap();
    
    // First, decode a valid packet to prime the decoder
    OpusConfig config;
    config.sample_rate = SAMPLE_RATE;
    auto encoder_result = OpusEncoder::create(config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    auto input = generate_sine_wave(440.0f, SAMPLE_RATE, FRAME_SIZE);
    auto encode_result = encoder->encode(input.data(), FRAME_SIZE);
    ASSERT_TRUE(encode_result.is_ok());
    
    std::vector<float> output(FRAME_SIZE);
    decoder->decode(
        encode_result.value().data.data(),
        encode_result.value().data.size(),
        output.data(),
        FRAME_SIZE
    );
    
    // Now test PLC
    std::vector<float> plc_output(FRAME_SIZE);
    auto plc_result = decoder->decode_plc(plc_output.data(), FRAME_SIZE);
    ASSERT_TRUE(plc_result.is_ok());
    EXPECT_EQ(plc_result.value(), FRAME_SIZE);
    
    // PLC output should not be all zeros
    bool has_non_zero = false;
    for (float sample : plc_output) {
        if (std::abs(sample) > 0.001f) {
            has_non_zero = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_zero) << "PLC output is all zeros";
}

TEST(OpusCodecTest, MultipleFrames) {
    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr size_t FRAME_SIZE = 960;
    constexpr size_t NUM_FRAMES = 10;
    
    OpusConfig config;
    config.sample_rate = SAMPLE_RATE;
    
    auto encoder_result = OpusEncoder::create(config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    auto decoder_result = OpusDecoder::create(SAMPLE_RATE, 1);
    ASSERT_TRUE(decoder_result.is_ok());
    auto decoder = decoder_result.unwrap();
    
    // Encode and decode multiple frames
    for (size_t frame = 0; frame < NUM_FRAMES; ++frame) {
        auto input = generate_sine_wave(440.0f, SAMPLE_RATE, FRAME_SIZE);
        
        auto encode_result = encoder->encode(input.data(), FRAME_SIZE);
        ASSERT_TRUE(encode_result.is_ok()) << "Frame " << frame << " encode failed";
        
        std::vector<float> output(FRAME_SIZE);
        auto decode_result = decoder->decode(
            encode_result.value().data.data(),
            encode_result.value().data.size(),
            output.data(),
            FRAME_SIZE
        );
        ASSERT_TRUE(decode_result.is_ok()) << "Frame " << frame << " decode failed";
    }
}
