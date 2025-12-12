#include <gtest/gtest.h>
#include "audio/audio_engine.h"
#include "audio/opus_codec.h"
#include "audio/jitter_buffer.h"
#include "common/lock_free_queue.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace voip::audio;
using namespace std::chrono_literals;

/**
 * Audio Loopback Integration Test
 * 
 * Tests the complete audio pipeline:
 * 1. Capture audio from microphone
 * 2. Encode with Opus
 * 3. Pass through jitter buffer
 * 4. Decode with Opus
 * 5. Play back to speakers
 * 
 * This validates that the entire system works together.
 */
class AudioLoopbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.sample_rate = 48000;
        config_.frame_size = 960;  // 20ms
        config_.buffer_frames = 3;
    }
    
    AudioConfig config_;
};

TEST_F(AudioLoopbackTest, EncodeDecodeChain) {
    // Test without actual audio device (simulation)
    
    // Create encoder
    OpusConfig opus_config;
    opus_config.sample_rate = 48000;
    opus_config.channels = 1;
    opus_config.bitrate = 32000;
    
    auto encoder_result = OpusEncoder::create(opus_config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    // Create decoder
    auto decoder_result = OpusDecoder::create(48000, 1);
    ASSERT_TRUE(decoder_result.is_ok());
    auto decoder = decoder_result.unwrap();
    
    // Create jitter buffer
    JitterBuffer jitter_buffer(5, 960);
    
    // Simulate audio pipeline
    constexpr size_t NUM_FRAMES = 10;
    constexpr size_t FRAME_SIZE = 960;
    
    // Generate test audio (simple pattern)
    std::vector<float> test_audio(FRAME_SIZE);
    for (size_t i = 0; i < FRAME_SIZE; i++) {
        test_audio[i] = std::sin(2.0f * M_PI * 440.0f * i / 48000.0f);
    }
    
    // Pipeline: Encode → Buffer → Decode
    for (SequenceNumber seq = 0; seq < NUM_FRAMES; seq++) {
        // 1. Encode
        auto encode_result = encoder->encode(test_audio.data(), FRAME_SIZE);
        ASSERT_TRUE(encode_result.is_ok()) << "Frame " << seq << " encode failed";
        auto packet = encode_result.unwrap();
        
        // 2. Put in jitter buffer
        AudioPacket audio_packet;
        audio_packet.sequence = seq;
        audio_packet.timestamp = Timestamp(seq * 20000);
        audio_packet.samples.resize(FRAME_SIZE);
        
        // Decode the opus packet first
        auto decode_result = decoder->decode(
            packet.data.data(),
            packet.data.size(),
            audio_packet.samples.data(),
            FRAME_SIZE
        );
        ASSERT_TRUE(decode_result.is_ok()) << "Frame " << seq << " decode failed";
        
        audio_packet.frame_size = FRAME_SIZE;
        
        EXPECT_TRUE(jitter_buffer.push(std::move(audio_packet)));
    }
    
    // Wait for buffer to be ready
    EXPECT_TRUE(jitter_buffer.is_ready());
    
    // 3. Pop and verify
    for (SequenceNumber seq = 0; seq < NUM_FRAMES; seq++) {
        auto result = jitter_buffer.pop();
        ASSERT_TRUE(result.has_value()) << "Frame " << seq << " pop failed";
        EXPECT_EQ(result->sequence, seq);
        EXPECT_EQ(result->samples.size(), FRAME_SIZE);
    }
    
    auto stats = jitter_buffer.get_stats();
    EXPECT_EQ(stats.packets_received, NUM_FRAMES);
    EXPECT_EQ(stats.packets_dropped, 0);
    EXPECT_EQ(stats.packets_late, 0);
}

TEST_F(AudioLoopbackTest, EncodeDecodeWithPacketLoss) {
    // Test resilience to packet loss
    
    OpusConfig opus_config;
    opus_config.sample_rate = 48000;
    opus_config.enable_fec = true;  // Enable forward error correction
    
    auto encoder_result = OpusEncoder::create(opus_config);
    ASSERT_TRUE(encoder_result.is_ok());
    auto encoder = encoder_result.unwrap();
    
    auto decoder_result = OpusDecoder::create(48000, 1);
    ASSERT_TRUE(decoder_result.is_ok());
    auto decoder = decoder_result.unwrap();
    
    JitterBuffer jitter_buffer(5, 960);
    
    constexpr size_t FRAME_SIZE = 960;
    std::vector<float> test_audio(FRAME_SIZE, 0.5f);
    
    // Encode and buffer packets, but simulate loss
    for (SequenceNumber seq = 0; seq < 10; seq++) {
        // Simulate 20% packet loss (drop sequence 2, 5, 7)
        if (seq == 2 || seq == 5 || seq == 7) {
            continue;  // Packet "lost"
        }
        
        auto encode_result = encoder->encode(test_audio.data(), FRAME_SIZE);
        ASSERT_TRUE(encode_result.is_ok());
        
        AudioPacket audio_packet;
        audio_packet.sequence = seq;
        audio_packet.timestamp = Timestamp(seq * 20000);
        audio_packet.samples.resize(FRAME_SIZE);
        
        auto decode_result = decoder->decode(
            encode_result.value().data.data(),
            encode_result.value().data.size(),
            audio_packet.samples.data(),
            FRAME_SIZE
        );
        ASSERT_TRUE(decode_result.is_ok());
        
        audio_packet.frame_size = FRAME_SIZE;
        jitter_buffer.push(std::move(audio_packet));
    }
    
    // Pop all packets - should get PLC for missing ones
    size_t plc_count = 0;
    for (SequenceNumber seq = 0; seq < 10; seq++) {
        auto result = jitter_buffer.pop();
        ASSERT_TRUE(result.has_value()) << "Expected packet or PLC for seq " << seq;
        
        if (result->samples.empty()) {
            plc_count++;
            // In real system, would call decoder->decode_plc() here
        }
    }
    
    EXPECT_EQ(plc_count, 3) << "Should have 3 PLC packets for lost packets";
    
    auto stats = jitter_buffer.get_stats();
    EXPECT_EQ(stats.packets_late, 3);  // Lost packets marked as late
}

TEST_F(AudioLoopbackTest, LockFreeQueueIntegration) {
    // Test lock-free queue with audio data
    constexpr size_t QUEUE_CAPACITY = 10;
    constexpr size_t FRAME_SIZE = 960;
    
    AudioBufferQueue queue(QUEUE_CAPACITY, FRAME_SIZE);
    
    // Producer: Generate and push frames
    std::vector<float> input_frame(FRAME_SIZE);
    for (size_t i = 0; i < FRAME_SIZE; i++) {
        input_frame[i] = static_cast<float>(i) / FRAME_SIZE;
    }
    
    // Push multiple frames
    for (size_t i = 0; i < 5; i++) {
        EXPECT_TRUE(queue.try_push(input_frame.data(), FRAME_SIZE));
    }
    
    EXPECT_EQ(queue.size(), 5);
    
    // Consumer: Pop and verify frames
    std::vector<float> output_frame(FRAME_SIZE);
    for (size_t i = 0; i < 5; i++) {
        EXPECT_TRUE(queue.try_pop(output_frame.data(), FRAME_SIZE));
        
        // Verify data integrity
        for (size_t j = 0; j < FRAME_SIZE; j++) {
            EXPECT_FLOAT_EQ(output_frame[j], input_frame[j]);
        }
    }
    
    EXPECT_TRUE(queue.empty());
}

// Manual test (requires audio device, disabled by default)
TEST_F(AudioLoopbackTest, DISABLED_RealDeviceLoopback) {
    // This test requires actual audio hardware and user interaction
    // Enable manually for testing with real devices
    
    AudioEngine engine;
    
    auto init_result = engine.initialize(config_);
    ASSERT_TRUE(init_result.is_ok()) << init_result.error().to_string();
    
    // Create encoder/decoder
    OpusConfig opus_config;
    auto encoder = OpusEncoder::create(opus_config).unwrap();
    auto decoder = OpusDecoder::create(48000, 1).unwrap();
    
    // Lock-free queues for RT-safe communication
    AudioBufferQueue capture_queue(10, 960);
    AudioBufferQueue playback_queue(10, 960);
    
    std::atomic<bool> running{true};
    
    // Set capture callback
    engine.set_capture_callback([&](const float* pcm, size_t frame_count) {
        // RT-SAFE: Just push to queue, no encoding here
        capture_queue.try_push(pcm, frame_count);
    });
    
    // Set playback callback
    engine.set_playback_callback([&](float* pcm, size_t frame_count) {
        // RT-SAFE: Just pop from queue, no decoding here
        if (!playback_queue.try_pop(pcm, frame_count)) {
            // Underrun - output silence
            std::fill(pcm, pcm + frame_count, 0.0f);
        }
    });
    
    // Processing thread (non-RT)
    std::thread processor([&]() {
        std::vector<float> frame(960);
        std::vector<float> decoded(960);
        
        while (running) {
            // Get captured audio
            if (capture_queue.try_pop(frame.data(), 960)) {
                // Encode
                auto encoded = encoder->encode(frame.data(), 960);
                if (encoded.is_ok()) {
                    // Decode (simulating network transmission)
                    auto decode_result = decoder->decode(
                        encoded.value().data.data(),
                        encoded.value().data.size(),
                        decoded.data(),
                        960
                    );
                    
                    if (decode_result.is_ok()) {
                        // Push to playback
                        playback_queue.try_push(decoded.data(), 960);
                    }
                }
            } else {
                std::this_thread::sleep_for(1ms);
            }
        }
    });
    
    // Start audio
    ASSERT_TRUE(engine.start_capture().is_ok());
    ASSERT_TRUE(engine.start_playback().is_ok());
    
    // Run for 3 seconds
    std::this_thread::sleep_for(3s);
    
    // Stop
    running = false;
    processor.join();
    
    engine.stop_capture();
    engine.stop_playback();
    
    // Check stats
    auto stats = engine.get_stats();
    std::cout << "Input overflows: " << stats.input_overflows << "\n";
    std::cout << "Output underflows: " << stats.output_underflows << "\n";
    std::cout << "Input level: " << stats.current_input_level << "\n";
    
    EXPECT_LT(stats.input_overflows, 10) << "Too many input overflows";
    EXPECT_LT(stats.output_underflows, 10) << "Too many output underflows";
}

// Performance benchmark test
TEST_F(AudioLoopbackTest, EncodingPerformance) {
    OpusConfig opus_config;
    opus_config.complexity = 10;  // Maximum quality
    
    auto encoder = OpusEncoder::create(opus_config).unwrap();
    
    constexpr size_t FRAME_SIZE = 960;
    constexpr size_t NUM_ITERATIONS = 1000;
    
    std::vector<float> test_frame(FRAME_SIZE, 0.5f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        auto result = encoder->encode(test_frame.data(), FRAME_SIZE);
        ASSERT_TRUE(result.is_ok());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_time_us = static_cast<double>(duration.count()) / NUM_ITERATIONS;
    double avg_time_ms = avg_time_us / 1000.0;
    
    std::cout << "Average encoding time: " << avg_time_ms << " ms\n";
    std::cout << "Frames per second: " << (1000.0 / avg_time_ms) << "\n";
    
    // 20ms frame should encode in < 5ms to leave headroom
    EXPECT_LT(avg_time_ms, 5.0) << "Encoding too slow for real-time";
}
