#pragma once

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET SocketType;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SocketType;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace voip::network {

// Forward declaration
struct VoicePacket;

/**
 * Callback for received voice packets
 * Called from network thread - must be thread-safe!
 */
using PacketReceivedCallback = std::function<void(const VoicePacket& packet)>;

/**
 * Voice packet structure for network transmission
 */
struct VoicePacket {
    VoicePacketHeader header;
    std::vector<uint8_t> encrypted_payload;
    
    // Serialize to bytes for network transmission
    std::vector<uint8_t> serialize() const;
    
    // Deserialize from received bytes
    static Result<VoicePacket> deserialize(const uint8_t* data, size_t length);
};

/**
 * UdpVoiceSocket - Handles UDP voice packet transmission
 * 
 * Responsibilities:
 * - Send voice packets to server
 * - Receive voice packets from server
 * - Packet serialization/deserialization
 * - Non-blocking async operations
 * 
 * Thread Safety: Public methods are thread-safe
 */
class UdpVoiceSocket {
public:
    UdpVoiceSocket();
    ~UdpVoiceSocket();
    
    // Disable copy
    UdpVoiceSocket(const UdpVoiceSocket&) = delete;
    UdpVoiceSocket& operator=(const UdpVoiceSocket&) = delete;
    
    /**
     * Connect to server
     * @param server_address Server IP or hostname
     * @param port UDP port (typically 9001)
     */
    Result<void> connect(const std::string& server_address, uint16_t port);
    
    /**
     * Disconnect from server
     */
    void disconnect();
    
    /**
     * Send voice packet to server
     * Non-blocking, queues packet for transmission
     */
    Result<void> send_packet(const VoicePacket& packet);
    
    /**
     * Set callback for received packets
     * Called from network thread - must be thread-safe!
     */
    void set_receive_callback(PacketReceivedCallback callback);
    
    /**
     * Check if connected
     */
    [[nodiscard]] bool is_connected() const noexcept;
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t packets_sent = 0;
        uint64_t packets_received = 0;
        uint64_t send_errors = 0;
        uint64_t receive_errors = 0;
        uint64_t bytes_sent = 0;
        uint64_t bytes_received = 0;
    };
    
    [[nodiscard]] Stats get_stats() const;
    
private:
    // Socket initialization (Windows-specific)
    static bool initialize_winsock();
    static void cleanup_winsock();
    
    // Receive loop (runs in background thread)
    void receive_loop();
    
    // Socket handle
    SocketType socket_ = INVALID_SOCKET;
    
    // Server address
    sockaddr_in server_addr_{};
    bool connected_ = false;
    
    // Receive thread
    std::unique_ptr<std::thread> receive_thread_;
    std::atomic<bool> running_{false};
    
    // Callback for received packets
    PacketReceivedCallback receive_callback_;
    
    // Statistics (atomic for thread safety)
    mutable std::atomic<uint64_t> packets_sent_{0};
    mutable std::atomic<uint64_t> packets_received_{0};
    mutable std::atomic<uint64_t> send_errors_{0};
    mutable std::atomic<uint64_t> receive_errors_{0};
    mutable std::atomic<uint64_t> bytes_sent_{0};
    mutable std::atomic<uint64_t> bytes_received_{0};
};

} // namespace voip::network
