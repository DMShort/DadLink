#include "network/udp_socket.h"
#include <cstring>
#include <iostream>

namespace voip::network {

// VoicePacket serialization
std::vector<uint8_t> VoicePacket::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(VOICE_PACKET_HEADER_SIZE + encrypted_payload.size());
    
    // Convert header to network byte order
    VoicePacketHeader net_header = header;
    net_header.magic = htonl(header.magic);
    net_header.sequence = htonll(header.sequence);
    net_header.timestamp = htonll(header.timestamp);
    net_header.channel_id = htonl(header.channel_id);
    net_header.user_id = htonl(header.user_id);
    
    // Append header
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&net_header);
    data.insert(data.end(), header_bytes, header_bytes + VOICE_PACKET_HEADER_SIZE);
    
    // Append payload
    data.insert(data.end(), encrypted_payload.begin(), encrypted_payload.end());
    
    return data;
}

Result<VoicePacket> VoicePacket::deserialize(const uint8_t* data, size_t length) {
    if (length < VOICE_PACKET_HEADER_SIZE) {
        return Err<VoicePacket>(ErrorCode::InvalidPacket, "Packet too small");
    }
    
    // Parse header
    VoicePacketHeader net_header;
    std::memcpy(&net_header, data, VOICE_PACKET_HEADER_SIZE);
    
    // Convert from network byte order
    VoicePacket packet;
    packet.header.magic = ntohl(net_header.magic);
    packet.header.sequence = ntohll(net_header.sequence);
    packet.header.timestamp = ntohll(net_header.timestamp);
    packet.header.channel_id = ntohl(net_header.channel_id);
    packet.header.user_id = ntohl(net_header.user_id);
    
    // Verify magic number
    if (packet.header.magic != VOICE_PACKET_MAGIC) {
        return Err<VoicePacket>(ErrorCode::InvalidPacket, "Invalid magic number");
    }
    
    // Extract payload
    if (length > VOICE_PACKET_HEADER_SIZE) {
        packet.encrypted_payload.assign(
            data + VOICE_PACKET_HEADER_SIZE,
            data + length
        );
    }
    
    return Ok(std::move(packet));
}

// UdpVoiceSocket implementation

UdpVoiceSocket::UdpVoiceSocket() {
#ifdef _WIN32
    initialize_winsock();
#endif
}

UdpVoiceSocket::~UdpVoiceSocket() {
    disconnect();
#ifdef _WIN32
    cleanup_winsock();
#endif
}

bool UdpVoiceSocket::initialize_winsock() {
#ifdef _WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    return result == 0;
#else
    return true;
#endif
}

void UdpVoiceSocket::cleanup_winsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

Result<void> UdpVoiceSocket::connect(const std::string& server_address, uint16_t port) {
    if (connected_) {
        return Err<void>(ErrorCode::NetworkConnectionFailed, "Already connected");
    }
    
    // Create UDP socket
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        return Err<void>(ErrorCode::NetworkConnectionFailed, "Failed to create socket");
    }
    
    // Set non-blocking mode
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(socket_, FIONBIO, &mode);
#else
    int flags = fcntl(socket_, F_GETFL, 0);
    fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
#endif
    
    // Set up server address
    std::memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port);
    
    // Resolve server address
    if (inet_pton(AF_INET, server_address.c_str(), &server_addr_.sin_addr) <= 0) {
        // Try hostname resolution
        // For now, just fail - full DNS resolution would go here
        disconnect();
        return Err<void>(ErrorCode::NetworkConnectionFailed, 
                        "Invalid server address: " + server_address);
    }
    
    connected_ = true;
    
    // Set socket timeout to allow graceful shutdown
#ifdef _WIN32
    DWORD timeout = 100;  // 100ms timeout
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 100ms
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    // Start receive thread
    running_ = true;
    receive_thread_ = std::make_unique<std::thread>(&UdpVoiceSocket::receive_loop, this);
    
    return Ok();
}

void UdpVoiceSocket::disconnect() {
    if (!connected_) {
        return;
    }
    
    std::cout << "🔌 Disconnecting UDP socket..." << std::endl;
    
    // Signal receive thread to stop
    running_ = false;
    
    // Shutdown socket to interrupt any blocking recv calls
    if (socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(socket_, SD_BOTH);  // Stop both send and receive
#else
        shutdown(socket_, SHUT_RDWR);
#endif
    }
    
    // Wait for receive thread to finish (with timeout protection)
    if (receive_thread_ && receive_thread_->joinable()) {
        // Thread should exit within 100ms due to socket timeout
        receive_thread_->join();
        std::cout << "✅ UDP receive thread stopped" << std::endl;
    }
    receive_thread_.reset();
    
    // Close socket
    if (socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        std::cout << "✅ UDP socket closed" << std::endl;
        socket_ = INVALID_SOCKET;
    }
    
    connected_ = false;
}

Result<void> UdpVoiceSocket::send_packet(const VoicePacket& packet) {
    if (!connected_) {
        return Err<void>(ErrorCode::NetworkSendFailed, "Not connected");
    }
    
    // Serialize packet
    auto data = packet.serialize();
    
    // Send to server
    int sent = sendto(
        socket_,
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()),
        0,
        reinterpret_cast<const sockaddr*>(&server_addr_),
        sizeof(server_addr_)
    );
    
    if (sent == SOCKET_ERROR) {
        send_errors_++;
        return Err<void>(ErrorCode::NetworkSendFailed, "sendto failed");
    }
    
    packets_sent_++;
    bytes_sent_ += sent;
    
    return Ok();
}

void UdpVoiceSocket::set_receive_callback(PacketReceivedCallback callback) {
    receive_callback_ = std::move(callback);
}

bool UdpVoiceSocket::is_connected() const noexcept {
    return connected_;
}

UdpVoiceSocket::Stats UdpVoiceSocket::get_stats() const {
    return Stats{
        .packets_sent = packets_sent_.load(),
        .packets_received = packets_received_.load(),
        .send_errors = send_errors_.load(),
        .receive_errors = receive_errors_.load(),
        .bytes_sent = bytes_sent_.load(),
        .bytes_received = bytes_received_.load()
    };
}

void UdpVoiceSocket::receive_loop() {
    std::vector<uint8_t> buffer(2048);  // Max UDP packet size
    sockaddr_in from_addr;
    
#ifdef _WIN32
    int from_len = sizeof(from_addr);
#else
    socklen_t from_len = sizeof(from_addr);
#endif
    
    while (running_) {
        // Receive packet
        int received = recvfrom(
            socket_,
            reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(buffer.size()),
            0,
            reinterpret_cast<sockaddr*>(&from_addr),
            &from_len
        );
        
        if (received > 0) {
            bytes_received_ += received;
            packets_received_++;
            
            // Parse packet
            auto result = VoicePacket::deserialize(buffer.data(), received);
            if (result.is_ok()) {
                // Call callback if set
                if (receive_callback_) {
                    receive_callback_(result.value());
                }
            } else {
                receive_errors_++;
            }
        } else if (received == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            // Ignore timeout and connection reset (happens during shutdown)
            if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT && error != WSAECONNRESET) {
                receive_errors_++;
                if (!running_) break;  // Exit if shutting down
            }
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                receive_errors_++;
                if (!running_) break;
            }
#endif
        }
        
        // No need for sleep - socket timeout handles this
    }
}

// Helper functions for 64-bit byte order conversion
#ifdef _WIN32
static inline uint64_t htonll(uint64_t value) {
    // Convert 64-bit value to network byte order (big-endian)
    // Upper 32 bits go to upper position, lower 32 bits go to lower position
    return ((uint64_t)htonl((uint32_t)(value >> 32)) << 32) | htonl((uint32_t)(value & 0xFFFFFFFF));
}

static inline uint64_t ntohll(uint64_t value) {
    // Convert 64-bit value from network byte order (big-endian)
    return ((uint64_t)ntohl((uint32_t)(value >> 32)) << 32) | ntohl((uint32_t)(value & 0xFFFFFFFF));
}
#else
#include <arpa/inet.h>
// Linux usually has these
#ifndef htonll
#define htonll(x) htobe64(x)
#endif
#ifndef ntohll
#define ntohll(x) be64toh(x)
#endif
#endif

} // namespace voip::network
