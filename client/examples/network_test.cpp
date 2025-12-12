/**
 * Simple network test - Connect to server and send test packet
 * 
 * Usage: network_test.exe [server_ip] [port]
 * Example: network_test.exe 127.0.0.1 9001
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "network/udp_socket.h"

using namespace voip;
using namespace voip::network;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    std::cout << "VoIP Network Test\n";
    std::cout << "=================\n\n";
    
    // Parse arguments
    std::string server = "127.0.0.1";
    uint16_t port = 9001;
    
    if (argc >= 2) {
        server = argv[1];
    }
    if (argc >= 3) {
        port = static_cast<uint16_t>(std::atoi(argv[2]));
    }
    
    std::cout << "Server: " << server << ":" << port << "\n\n";
    
    // Create UDP socket
    UdpVoiceSocket socket;
    
    // Set up receive callback
    socket.set_receive_callback([](const VoicePacket& packet) {
        std::cout << "Received packet:\n";
        std::cout << "  Sequence: " << packet.header.sequence << "\n";
        std::cout << "  Timestamp: " << packet.header.timestamp << "\n";
        std::cout << "  Channel: " << packet.header.channel_id << "\n";
        std::cout << "  User: " << packet.header.user_id << "\n";
        std::cout << "  Payload size: " << packet.encrypted_payload.size() << " bytes\n\n";
    });
    
    // Connect to server
    std::cout << "Connecting to server...\n";
    auto result = socket.connect(server, port);
    if (!result.is_ok()) {
        std::cerr << "Failed to connect: " << result.error().to_string() << "\n";
        return 1;
    }
    
    std::cout << "Connected!\n\n";
    
    // Send test packets
    std::cout << "Sending 10 test packets...\n";
    for (int i = 0; i < 10; i++) {
        VoicePacket packet;
        packet.header.magic = VOICE_PACKET_MAGIC;
        packet.header.sequence = i;
        packet.header.timestamp = i * 20000;  // 20ms intervals
        packet.header.channel_id = 1;
        packet.header.user_id = 42;
        
        // Test payload
        packet.encrypted_payload = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        auto send_result = socket.send_packet(packet);
        if (send_result.is_ok()) {
            std::cout << "  Sent packet " << i << "\n";
        } else {
            std::cerr << "  Failed to send packet " << i << ": " 
                      << send_result.error().to_string() << "\n";
        }
        
        std::this_thread::sleep_for(100ms);
    }
    
    std::cout << "\nWaiting for responses (5 seconds)...\n";
    std::this_thread::sleep_for(5s);
    
    // Show statistics
    auto stats = socket.get_stats();
    std::cout << "\nStatistics:\n";
    std::cout << "  Packets sent: " << stats.packets_sent << "\n";
    std::cout << "  Packets received: " << stats.packets_received << "\n";
    std::cout << "  Bytes sent: " << stats.bytes_sent << "\n";
    std::cout << "  Bytes received: " << stats.bytes_received << "\n";
    std::cout << "  Send errors: " << stats.send_errors << "\n";
    std::cout << "  Receive errors: " << stats.receive_errors << "\n";
    
    // Disconnect
    std::cout << "\nDisconnecting...\n";
    socket.disconnect();
    
    std::cout << "Done!\n";
    
    return 0;
}
