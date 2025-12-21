#pragma once

#include "protocol/control_messages.h"
#include "common/result.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

// Forward declarations for Qt WebSocket
class QWebSocket;
class QString;
class QUrl;

namespace voip::network {

/**
 * Callback types for WebSocket events
 */
using ConnectedCallback = std::function<void()>;
using DisconnectedCallback = std::function<void()>;
using LoginCallback = std::function<void(const protocol::LoginResponse&)>;
using RegisterCallback = std::function<void(bool success, const std::string& message, uint32_t user_id)>;
using ChannelJoinedCallback = std::function<void(const protocol::ChannelJoinedResponse&)>;
using UserJoinedCallback = std::function<void(const protocol::UserJoinedNotification&)>;
using UserLeftCallback = std::function<void(const protocol::UserLeftNotification&)>;
using ErrorCallback = std::function<void(const protocol::ErrorMessage&)>;
using KeyExchangeInitCallback = std::function<void(const protocol::KeyExchangeInit&)>;
using AllChannelRostersCallback = std::function<void(const protocol::AllChannelRostersResponse&)>;

/**
 * WebSocketClient - Handles WebSocket control channel
 * 
 * Responsibilities:
 * - Connect to control server
 * - Authenticate users
 * - Join/leave channels
 * - Send/receive control messages
 * - Handle notifications
 * 
 * Thread Safety: Public methods are thread-safe
 */
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    
    // Disable copy
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
    
    /**
     * Connect to control server
     * @param server_address Server IP or hostname
     * @param port WebSocket port (typically 9000)
     * @param use_tls Use wss:// instead of ws://
     */
    Result<void> connect(const std::string& server_address, 
                         uint16_t port, 
                         bool use_tls = false);
    
    /**
     * Disconnect from server
     */
    void disconnect();
    
    /**
     * Check if connected
     */
    [[nodiscard]] bool is_connected() const noexcept;
    
    /**
     * Authenticate with server
     * @param username User's username
     * @param password User's password
     * @param org_tag Optional organization tag
     */
    Result<void> login(const std::string& username, 
                       const std::string& password,
                       const std::string& org_tag = "");
    
    /**
     * Register new account
     * @param username Desired username (3-20 chars)
     * @param password Password (min 3 chars)
     */
    void register_account(const std::string& username, const std::string& password);
    
    /**
     * Join a channel
     * @param channel_id Channel to join
     * @param password Optional channel password
     */
    Result<void> join_channel(ChannelId channel_id, 
                              const std::string& password = "");
    
    /**
     * Leave current channel (legacy single-channel)
     */
    Result<void> leave_channel();

    /**
     * Leave a specific channel (multi-channel)
     */
    Result<void> leave_channel(ChannelId channel_id);

    /**
     * Request all channel rosters from server
     * Server will send rosters for all channels user has permission to see
     */
    Result<void> request_all_channel_rosters();

    /**
     * Send key exchange response to server
     * @param public_key Client's X25519 public key (32 bytes)
     */
    Result<void> send_key_exchange_response(const std::array<uint8_t, 32>& public_key);

    /**
     * Get authentication token (JWT)
     */
    [[nodiscard]] const std::string& get_token() const noexcept { return auth_token_; }
    
    /**
     * Get user ID
     */
    [[nodiscard]] UserId get_user_id() const noexcept { return user_id_; }
    
    /**
     * Get organization ID
     */
    [[nodiscard]] OrgId get_org_id() const noexcept { return org_id_; }
    
    /**
     * Get current channel ID
     */
    [[nodiscard]] ChannelId get_current_channel() const noexcept { return current_channel_; }
    
    /**
     * Get available channels
     */
    [[nodiscard]] std::vector<protocol::ChannelInfo> get_channels() const;
    
    /**
     * Set callback handlers
     */
    void set_connected_callback(ConnectedCallback callback);
    void set_disconnected_callback(DisconnectedCallback callback);
    void set_login_callback(LoginCallback callback);
    void set_register_callback(RegisterCallback callback);
    void set_channel_joined_callback(ChannelJoinedCallback callback);
    void set_user_joined_callback(UserJoinedCallback callback);
    void set_user_left_callback(UserLeftCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_key_exchange_init_callback(KeyExchangeInitCallback callback);
    void set_all_channel_rosters_callback(AllChannelRostersCallback callback);
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t messages_sent = 0;
        uint64_t messages_received = 0;
        uint64_t errors = 0;
        uint64_t reconnect_attempts = 0;
    };
    
    [[nodiscard]] Stats get_stats() const;
    
private:
    // Internal event handlers
    void on_connected();
    void on_disconnected();
    void on_text_message_received(const QString& message);
    void on_error(const QString& error);
    
    // Message handlers
    void handle_login_response(const std::string& json);
    void handle_register_response(const std::string& json);
    void handle_channel_joined(const std::string& json);
    void handle_user_joined(const std::string& json);
    void handle_user_left(const std::string& json);
    void handle_error(const std::string& json);
    void handle_key_exchange_init(const std::string& json);
    void handle_all_channel_rosters(const std::string& json);
    
    // Send message helpers
    Result<void> send_message(protocol::MessageType type, const std::string& json);
    
    // WebSocket connection
    std::unique_ptr<QWebSocket> websocket_;
    
    // Authentication state
    std::string auth_token_;
    UserId user_id_ = 0;
    OrgId org_id_ = 0;
    ChannelId current_channel_ = 0;
    
    // Available channels
    std::vector<protocol::ChannelInfo> channels_;
    mutable std::mutex channels_mutex_;
    
    // Connection state
    std::atomic<bool> connected_{false};
    std::atomic<bool> authenticated_{false};
    
    // Callbacks
    ConnectedCallback on_connected_cb_;
    DisconnectedCallback on_disconnected_cb_;
    LoginCallback on_login_cb_;
    RegisterCallback on_register_cb_;
    ChannelJoinedCallback on_channel_joined_cb_;
    UserJoinedCallback on_user_joined_cb_;
    UserLeftCallback on_user_left_cb_;
    ErrorCallback on_error_cb_;
    KeyExchangeInitCallback on_key_exchange_init_cb_;
    AllChannelRostersCallback on_all_channel_rosters_cb_;

    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable std::atomic<uint64_t> messages_sent_{0};
    mutable std::atomic<uint64_t> messages_received_{0};
    mutable std::atomic<uint64_t> errors_{0};
    mutable std::atomic<uint64_t> reconnect_attempts_{0};
};

} // namespace voip::network
