#pragma once

#include "common/types.h"
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace voip::protocol {

/**
 * Control message types for WebSocket communication
 */
enum class MessageType : uint8_t {
    // Authentication
    LoginRequest = 1,
    LoginResponse = 2,
    Logout = 3,
    
    // Channel management
    JoinChannel = 10,
    LeaveChannel = 11,
    ChannelJoined = 12,
    ChannelLeft = 13,
    
    // Notifications
    UserJoined = 20,
    UserLeft = 21,
    UserSpeaking = 22,
    UserMuted = 23,
    
    // Ping/pong
    Ping = 30,
    Pong = 31,
    
    // Errors
    Error = 255,
};

/**
 * Channel information
 */
struct ChannelInfo {
    ChannelId id;
    std::string name;
    std::string description;
    uint32_t user_count;
    uint32_t max_users;
    bool password_protected;
};

/**
 * User information
 */
struct UserInfo {
    UserId id;
    std::string username;
    bool speaking;
    bool muted;
};

/**
 * Login Request: Client → Server
 */
struct LoginRequest {
    std::string username;
    std::string password;
    std::string org_tag;
    
    static constexpr MessageType TYPE = MessageType::LoginRequest;
};

/**
 * Login Response: Server → Client
 */
struct LoginResponse {
    bool success;
    std::string token;              // JWT token
    UserId user_id;
    OrgId org_id;
    uint32_t permissions;           // User permissions bitflags
    std::vector<ChannelInfo> channels;
    std::string error_message;
    
    static constexpr MessageType TYPE = MessageType::LoginResponse;
};

/**
 * Join Channel Request: Client → Server
 */
struct JoinChannelRequest {
    ChannelId channel_id;
    std::string password;  // Optional channel password
    
    static constexpr MessageType TYPE = MessageType::JoinChannel;
};

/**
 * Channel Joined: Server → Client
 */
struct ChannelJoinedResponse {
    ChannelId channel_id;
    std::vector<UserInfo> users;  // Current users in channel
    
    static constexpr MessageType TYPE = MessageType::ChannelJoined;
};

/**
 * Leave Channel: Client → Server
 */
struct LeaveChannelRequest {
    ChannelId channel_id;
    
    static constexpr MessageType TYPE = MessageType::LeaveChannel;
};

/**
 * User Joined Notification: Server → Client
 */
struct UserJoinedNotification {
    ChannelId channel_id;
    UserId user_id;
    std::string username;
    
    static constexpr MessageType TYPE = MessageType::UserJoined;
};

/**
 * User Left Notification: Server → Client
 */
struct UserLeftNotification {
    ChannelId channel_id;
    UserId user_id;
    
    static constexpr MessageType TYPE = MessageType::UserLeft;
};

/**
 * Error Message: Server → Client
 */
struct ErrorMessage {
    std::string message;
    uint32_t code;
    
    static constexpr MessageType TYPE = MessageType::Error;
};

/**
 * Ping: Client ↔ Server
 */
struct PingMessage {
    uint64_t timestamp;
    
    static constexpr MessageType TYPE = MessageType::Ping;
};

/**
 * Pong: Client ↔ Server
 */
struct PongMessage {
    uint64_t timestamp;

    static constexpr MessageType TYPE = MessageType::Pong;
};

/**
 * Key Exchange Init: Server → Client
 * Server sends its X25519 public key to initiate SRTP key exchange
 */
struct KeyExchangeInit {
    std::array<uint8_t, 32> public_key;  // Server's X25519 public key
};

/**
 * Key Exchange Response: Client → Server
 * Client responds with its X25519 public key to complete key exchange
 */
struct KeyExchangeResponse {
    std::array<uint8_t, 32> public_key;  // Client's X25519 public key
};

} // namespace voip::protocol
