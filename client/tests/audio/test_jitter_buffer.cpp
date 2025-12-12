#include <gtest/gtest.h>
#include "audio/jitter_buffer.h"

using namespace voip::audio;

// Helper to create test packet
AudioPacket create_packet(SequenceNumber seq, size_t frame_size) {
    AudioPacket packet;
    packet.sequence = seq;
    packet.timestamp = Timestamp(seq * 20000);  // 20ms intervals
    packet.samples.resize(frame_size, 0.5f);  // Fill with test data
    packet.frame_size = frame_size;
    return packet;
}

TEST(JitterBufferTest, Creation) {
    JitterBuffer buffer(5, 960);
    
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_FALSE(buffer.is_ready());
}

TEST(JitterBufferTest, InOrderPackets) {
    constexpr size_t FRAME_SIZE = 960;
    constexpr uint32_t BUFFER_SIZE = 5;
    
    JitterBuffer buffer(BUFFER_SIZE, FRAME_SIZE);
    
    // Push packets in order
    for (SequenceNumber seq = 0; seq < 10; seq++) {
        auto packet = create_packet(seq, FRAME_SIZE);
        EXPECT_TRUE(buffer.push(std::move(packet)));
    }
    
    EXPECT_EQ(buffer.size(), 10);
    EXPECT_TRUE(buffer.is_ready());
    
    // Pop packets in order
    for (SequenceNumber seq = 0; seq < 10; seq++) {
        auto result = buffer.pop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->sequence, seq);
        EXPECT_EQ(result->samples.size(), FRAME_SIZE);
    }
    
    EXPECT_EQ(buffer.size(), 0);
}

TEST(JitterBufferTest, OutOfOrderPackets) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    // Push packets out of order
    buffer.push(create_packet(0, FRAME_SIZE));
    buffer.push(create_packet(2, FRAME_SIZE));  // Skip 1
    buffer.push(create_packet(1, FRAME_SIZE));  // Late arrival
    buffer.push(create_packet(3, FRAME_SIZE));
    
    // Should still pop in correct order
    for (SequenceNumber seq = 0; seq < 4; seq++) {
        auto result = buffer.pop();
        ASSERT_TRUE(result.has_value()) << "Failed to pop sequence " << seq;
        EXPECT_EQ(result->sequence, seq);
    }
}

TEST(JitterBufferTest, PacketLoss) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    // Push packets with gap
    buffer.push(create_packet(0, FRAME_SIZE));
    buffer.push(create_packet(1, FRAME_SIZE));
    // Packet 2 is lost
    buffer.push(create_packet(3, FRAME_SIZE));
    buffer.push(create_packet(4, FRAME_SIZE));
    
    // Pop sequence 0 and 1
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_TRUE(buffer.pop().has_value());
    
    // Pop sequence 2 - should return PLC packet (empty samples)
    auto plc_packet = buffer.pop();
    ASSERT_TRUE(plc_packet.has_value());
    EXPECT_EQ(plc_packet->sequence, 2);
    EXPECT_TRUE(plc_packet->samples.empty()) << "PLC packet should have empty samples";
    
    // Next packets should be normal
    auto packet3 = buffer.pop();
    ASSERT_TRUE(packet3.has_value());
    EXPECT_EQ(packet3->sequence, 3);
    EXPECT_FALSE(packet3->samples.empty());
}

TEST(JitterBufferTest, DuplicatePackets) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    buffer.push(create_packet(0, FRAME_SIZE));
    buffer.push(create_packet(1, FRAME_SIZE));
    
    // Try to push duplicate
    EXPECT_FALSE(buffer.push(create_packet(0, FRAME_SIZE)));
    EXPECT_FALSE(buffer.push(create_packet(1, FRAME_SIZE)));
    
    auto stats = buffer.get_stats();
    EXPECT_EQ(stats.packets_duplicate, 2);
}

TEST(JitterBufferTest, LatePackets) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    // Push initial packets
    for (SequenceNumber seq = 0; seq < 5; seq++) {
        buffer.push(create_packet(seq, FRAME_SIZE));
    }
    
    // Pop some packets
    buffer.pop();
    buffer.pop();
    buffer.pop();
    
    // Now next expected is 3, try to push very old packet (0)
    EXPECT_FALSE(buffer.push(create_packet(0, FRAME_SIZE)));
    
    auto stats = buffer.get_stats();
    EXPECT_GT(stats.packets_duplicate, 0);
}

TEST(JitterBufferTest, BufferReady) {
    constexpr size_t FRAME_SIZE = 960;
    constexpr uint32_t BUFFER_SIZE = 5;
    
    JitterBuffer buffer(BUFFER_SIZE, FRAME_SIZE);
    
    EXPECT_FALSE(buffer.is_ready());
    
    // Add packets one by one
    for (uint32_t i = 0; i < BUFFER_SIZE - 1; i++) {
        buffer.push(create_packet(i, FRAME_SIZE));
        EXPECT_FALSE(buffer.is_ready()) << "Should not be ready with " << (i + 1) << " packets";
    }
    
    // Add one more to reach target
    buffer.push(create_packet(BUFFER_SIZE - 1, FRAME_SIZE));
    EXPECT_TRUE(buffer.is_ready()) << "Should be ready with " << BUFFER_SIZE << " packets";
}

TEST(JitterBufferTest, Underrun) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    // Try to pop from empty buffer
    auto result = buffer.pop();
    EXPECT_FALSE(result.has_value());
    
    auto stats = buffer.get_stats();
    EXPECT_EQ(stats.underruns, 0);  // Not initialized yet, so no underrun
    
    // Add and pop all packets
    buffer.push(create_packet(0, FRAME_SIZE));
    buffer.pop();
    
    // Now popping from empty should be underrun
    result = buffer.pop();
    EXPECT_FALSE(result.has_value());
    
    stats = buffer.get_stats();
    EXPECT_GT(stats.underruns, 0);
}

TEST(JitterBufferTest, Reset) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    // Add some packets
    for (SequenceNumber seq = 0; seq < 5; seq++) {
        buffer.push(create_packet(seq, FRAME_SIZE));
    }
    
    EXPECT_GT(buffer.size(), 0);
    EXPECT_TRUE(buffer.is_ready());
    
    // Reset
    buffer.reset();
    
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_FALSE(buffer.is_ready());
    
    // Should be able to start fresh
    buffer.push(create_packet(100, FRAME_SIZE));  // New sequence range
    EXPECT_EQ(buffer.size(), 1);
}

TEST(JitterBufferTest, Statistics) {
    constexpr size_t FRAME_SIZE = 960;
    JitterBuffer buffer(5, FRAME_SIZE);
    
    // Add packets with some loss and duplicates
    buffer.push(create_packet(0, FRAME_SIZE));
    buffer.push(create_packet(1, FRAME_SIZE));
    buffer.push(create_packet(1, FRAME_SIZE));  // Duplicate
    buffer.push(create_packet(3, FRAME_SIZE));  // Gap at 2
    
    buffer.pop();  // 0
    buffer.pop();  // 1
    buffer.pop();  // 2 (PLC)
    buffer.pop();  // 3
    
    auto stats = buffer.get_stats();
    
    EXPECT_EQ(stats.packets_received, 4);
    EXPECT_EQ(stats.packets_duplicate, 1);
    EXPECT_GT(stats.packets_late, 0);  // Packet 2 was marked as late/lost
}
