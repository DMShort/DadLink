#include "network/websocket_client.h"
#include <QtWebSockets/QWebSocket>
#include <QtCore/QUrl>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <iostream>

namespace voip::network {

WebSocketClient::WebSocketClient()
    : websocket_(std::make_unique<QWebSocket>())
{
    // Connect Qt signals to our handlers
    QObject::connect(websocket_.get(), &QWebSocket::connected,
                     [this]() { on_connected(); });

    QObject::connect(websocket_.get(), &QWebSocket::disconnected,
                     [this]() { on_disconnected(); });

    QObject::connect(websocket_.get(), &QWebSocket::textMessageReceived,
                     [this](const QString& msg) { on_text_message_received(msg); });

    QObject::connect(websocket_.get(),
                     QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     [this](QAbstractSocket::SocketError) {
                         on_error(websocket_->errorString());
                     });

    // Ignore SSL errors for self-signed certificates (development only)
    QObject::connect(websocket_.get(), &QWebSocket::sslErrors,
                     [this](const QList<QSslError>& errors) {
                         std::cout << "⚠️  SSL certificate warnings (ignoring for development):" << std::endl;
                         for (const auto& error : errors) {
                             std::cout << "   - " << error.errorString().toStdString() << std::endl;
                         }
                         websocket_->ignoreSslErrors();
                     });
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

Result<void> WebSocketClient::connect(const std::string& server_address,
                                      uint16_t port,
                                      bool use_tls) {
    if (connected_) {
        return Err<void>(ErrorCode::NetworkConnectionFailed, "Already connected");
    }
    
    // Build WebSocket URL
    std::string protocol = use_tls ? "wss" : "ws";
    std::string url = protocol + "://" + server_address + ":" + std::to_string(port) + "/control";
    
    std::cout << "WebSocket connecting to: " << url << "\n";
    
    // Open connection
    websocket_->open(QUrl(QString::fromStdString(url)));
    
    return Ok();
}

void WebSocketClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    websocket_->close();
    connected_ = false;
    authenticated_ = false;
}

bool WebSocketClient::is_connected() const noexcept {
    return connected_;
}

Result<void> WebSocketClient::login(const std::string& username,
                                    const std::string& password,
                                    const std::string& org_tag) {
    if (!connected_) {
        return Err<void>(ErrorCode::NetworkConnectionFailed, "Not connected");
    }
    
    // Build JSON message to match Rust server protocol
    QJsonObject json;
    json["type"] = "authenticate";  // snake_case to match serde
    json["method"] = "password";     // Required by server
    json["username"] = QString::fromStdString(username);
    json["password"] = QString::fromStdString(password);
    json["token"] = QJsonValue::Null;  // Optional, send null
    
    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);
    
    std::cout << "Sending authenticate request for user: " << username << "\n";
    std::cout << "JSON: " << message.toStdString() << "\n";
    
    websocket_->sendTextMessage(message);
    messages_sent_++;
    
    return Ok();
}

void WebSocketClient::register_account(const std::string& username, const std::string& password) {
    QJsonObject json;
    json["type"] = "register";
    json["username"] = QString::fromStdString(username);
    json["password"] = QString::fromStdString(password);
    json["email"] = QJsonValue::Null;  // Optional field
    
    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);
    
    std::cout << "📝 Sending register request for user: " << username << std::endl;
    
    websocket_->sendTextMessage(message);
}

Result<void> WebSocketClient::join_channel(ChannelId channel_id,
                                           const std::string& password) {
    if (!authenticated_) {
        return Err<void>(ErrorCode::AuthenticationFailed, "Not authenticated");
    }
    
    // Build JSON message to match Rust server protocol
    QJsonObject json;
    json["type"] = "join_channel";  // snake_case to match serde
    json["channel_id"] = static_cast<qint64>(channel_id);
    
    if (!password.empty()) {
        json["password"] = QString::fromStdString(password);
    } else {
        json["password"] = QJsonValue::Null;
    }
    
    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);
    
    std::cout << "Joining channel: " << channel_id << "\n";
    std::cout << "JSON: " << message.toStdString() << "\n";
    
    websocket_->sendTextMessage(message);
    messages_sent_++;
    
    return Ok();
}

Result<void> WebSocketClient::leave_channel(ChannelId channel_id) {
    if (!authenticated_) {
        return Err<void>(ErrorCode::AuthenticationFailed, "Not authenticated");
    }

    // Build JSON message to match Rust server protocol
    QJsonObject json;
    json["type"] = "leave_channel";  // snake_case to match serde
    json["channel_id"] = static_cast<qint64>(channel_id);

    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);

    std::cout << "Leaving channel: " << channel_id << "\n";
    std::cout << "JSON: " << message.toStdString() << "\n";

    websocket_->sendTextMessage(message);
    messages_sent_++;

    return Ok();
}

Result<void> WebSocketClient::leave_channel() {
    if (!authenticated_ || current_channel_ == 0) {
        return Err<void>(ErrorCode::InvalidState, "Not in a channel");
    }

    // Build JSON message to match Rust server protocol
    QJsonObject json;
    json["type"] = "leave_channel";  // snake_case to match serde
    json["channel_id"] = static_cast<qint64>(current_channel_);

    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);

    std::cout << "Leaving channel: " << current_channel_ << "\n";

    websocket_->sendTextMessage(message);
    messages_sent_++;

    current_channel_ = 0;

    return Ok();
}

Result<void> WebSocketClient::request_all_channel_rosters() {
    if (!authenticated_) {
        return Err<void>(ErrorCode::InvalidState, "Not authenticated");
    }

    // Build JSON message to match Rust server protocol
    QJsonObject json;
    json["type"] = "request_all_channel_rosters";  // snake_case to match serde

    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);

    std::cout << "📊 Requesting all channel rosters\n";

    websocket_->sendTextMessage(message);
    messages_sent_++;

    return Ok();
}

Result<void> WebSocketClient::send_key_exchange_response(const std::array<uint8_t, 32>& public_key) {
    if (!connected_) {
        return Err<void>(ErrorCode::NetworkConnectionFailed, "Not connected");
    }

    // Build JSON message to match Rust server protocol
    QJsonObject json;
    json["type"] = "key_exchange_response";  // snake_case to match serde

    // Convert public key bytes to JSON array
    QJsonArray key_array;
    for (size_t i = 0; i < 32; ++i) {
        key_array.append(static_cast<int>(public_key[i]));
    }
    json["public_key"] = key_array;

    QJsonDocument doc(json);
    QString message = doc.toJson(QJsonDocument::Compact);

    std::cout << "🔑 Sending key exchange response (32-byte public key)" << std::endl;

    websocket_->sendTextMessage(message);
    messages_sent_++;

    return Ok();
}

std::vector<protocol::ChannelInfo> WebSocketClient::get_channels() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return channels_;
}

void WebSocketClient::set_connected_callback(ConnectedCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_connected_cb_ = std::move(callback);
}

void WebSocketClient::set_disconnected_callback(DisconnectedCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_disconnected_cb_ = std::move(callback);
}

void WebSocketClient::set_login_callback(LoginCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_login_cb_ = std::move(callback);
}

void WebSocketClient::set_register_callback(RegisterCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_register_cb_ = std::move(callback);
}

void WebSocketClient::set_channel_joined_callback(ChannelJoinedCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_channel_joined_cb_ = std::move(callback);
}

void WebSocketClient::set_user_joined_callback(UserJoinedCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_user_joined_cb_ = std::move(callback);
}

void WebSocketClient::set_user_left_callback(UserLeftCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_user_left_cb_ = std::move(callback);
}

void WebSocketClient::set_error_callback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_error_cb_ = std::move(callback);
}

void WebSocketClient::set_key_exchange_init_callback(KeyExchangeInitCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_key_exchange_init_cb_ = std::move(callback);
}

void WebSocketClient::set_all_channel_rosters_callback(AllChannelRostersCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_all_channel_rosters_cb_ = std::move(callback);
}

WebSocketClient::Stats WebSocketClient::get_stats() const {
    return Stats{
        .messages_sent = messages_sent_.load(),
        .messages_received = messages_received_.load(),
        .errors = errors_.load(),
        .reconnect_attempts = reconnect_attempts_.load()
    };
}

// Internal event handlers

void WebSocketClient::on_connected() {
    std::cout << "✅ WebSocket connected!\n";
    connected_ = true;
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_connected_cb_) {
        on_connected_cb_();
    }
}

void WebSocketClient::on_disconnected() {
    std::cout << "❌ WebSocket disconnected\n";
    connected_ = false;
    authenticated_ = false;
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_disconnected_cb_) {
        on_disconnected_cb_();
    }
}

void WebSocketClient::on_text_message_received(const QString& message) {
    messages_received_++;
    
    // DEBUG: Print raw message
    std::cout << "📨 WebSocket RAW message: " << message.toStdString() << std::endl;
    
    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        std::cerr << "Invalid JSON message received\n";
        errors_++;
        return;
    }
    
    QJsonObject json = doc.object();
    QString typeStr = json["type"].toString();  // Server sends string, not int!
    
    std::cout << "📨 Parsed type: " << typeStr.toStdString() << std::endl;
    
    std::string json_str = message.toStdString();
    
    // Route to appropriate handler based on string type (snake_case from Rust server)
    if (typeStr == "auth_result") {  // Server sends auth_result, not login_response
        handle_login_response(json_str);
    } else if (typeStr == "register_result") {  // Registration response
        handle_register_response(json_str);
    } else if (typeStr == "channel_joined") {  // snake_case
        handle_channel_joined(json_str);
    } else if (typeStr == "channel_state") {  // Server uses channel_state for user events
        handle_user_joined(json_str);  // Parse event field to determine join/leave
    } else if (typeStr == "user_left") {  // snake_case
        handle_user_left(json_str);
    } else if (typeStr == "error") {  // snake_case
        handle_error(json_str);
    } else if (typeStr == "challenge") {  // Server sends this first
        std::cout << "✅ Received server challenge" << std::endl;
    } else if (typeStr == "key_exchange_init") {  // SRTP key exchange
        std::cout << "🔑 Received key exchange init" << std::endl;
        handle_key_exchange_init(json_str);
    } else if (typeStr == "all_channel_rosters") {  // Channel roster broadcast
        std::cout << "📊 Received all channel rosters" << std::endl;
        handle_all_channel_rosters(json_str);
    } else {
        std::cout << "Unknown message type: " << typeStr.toStdString() << "\n";
    }
}

void WebSocketClient::on_error(const QString& error) {
    std::cerr << "WebSocket error: " << error.toStdString() << "\n";
    errors_++;
}

// Message handlers

void WebSocketClient::handle_register_response(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();
    
    bool success = json["success"].toBool();
    std::string message = json["message"].toString().toStdString();
    uint32_t user_id = 0;
    
    if (json.contains("user_id") && !json["user_id"].isNull()) {
        user_id = json["user_id"].toInt();
    }
    
    if (success) {
        std::cout << "✅ Registration successful! User ID: " << user_id << "\n";
        std::cout << "   " << message << "\n";
    } else {
        std::cerr << "❌ Registration failed: " << message << "\n";
    }
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_register_cb_) {
        on_register_cb_(success, message, user_id);
    }
}

void WebSocketClient::handle_login_response(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();
    
    protocol::LoginResponse response;
    response.success = json["success"].toBool();
    
    // Server uses session_token, not token
    response.token = json["session_token"].toString().toStdString();
    
    // Server may return null user_id if auth failed
    if (json.contains("user_id") && !json["user_id"].isNull()) {
        response.user_id = json["user_id"].toInt();
    } else {
        response.user_id = 0;
    }
    
    // Read org_id from server
    if (json.contains("org_id") && !json["org_id"].isNull()) {
        response.org_id = json["org_id"].toInt();
    } else {
        response.org_id = 1;  // Default fallback
    }
    
    // Read permissions from server
    if (json.contains("permissions") && !json["permissions"].isNull()) {
        response.permissions = json["permissions"].toInt();
    } else {
        response.permissions = 0;  // No permissions by default
    }
    
    // Server uses message field for error message
    if (json.contains("message") && !json["message"].isNull()) {
        response.error_message = json["message"].toString().toStdString();
    }
    
    if (response.success) {
        std::cout << "✅ Authentication successful!\n";
        std::cout << "   User ID: " << response.user_id << "\n";
        std::cout << "   Org ID: " << response.org_id << "\n";
        std::cout << "   Permissions: 0x" << std::hex << response.permissions << std::dec << "\n";
        std::cout << "   Session token: " << (response.token.empty() ? "<empty>" : "<received>") << "\n";
        
        authenticated_ = true;
        auth_token_ = response.token;
        user_id_ = response.user_id;
        org_id_ = response.org_id;
        
        // Server doesn't send channels list in auth_result
        // Channels will be discovered via other means
        
    } else {
        std::cerr << "❌ Authentication failed";
        if (!response.error_message.empty()) {
            std::cerr << ": " << response.error_message;
        }
        std::cerr << "\n";
    }
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_login_cb_) {
        on_login_cb_(response);
    }
}

void WebSocketClient::handle_channel_joined(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();
    
    protocol::ChannelJoinedResponse response;
    response.channel_id = json["channel_id"].toInt();
    
    // Parse users in channel - server uses "name" not "username"
    QJsonArray users_array = json["users"].toArray();
    for (const auto& u : users_array) {
        QJsonObject u_obj = u.toObject();
        protocol::UserInfo user;
        user.id = u_obj["id"].toInt();
        user.username = u_obj["name"].toString().toStdString();  // Server uses "name"
        user.speaking = u_obj["speaking"].toBool();
        user.muted = false;  // Server doesn't send muted field yet
        
        response.users.push_back(user);
    }
    
    current_channel_ = response.channel_id;
    
    std::cout << "✅ Joined channel " << response.channel_id 
              << " with " << response.users.size() << " users:\n";
    for (const auto& user : response.users) {
        std::cout << "   - " << user.username << " (ID: " << user.id << ")\n";
    }
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_channel_joined_cb_) {
        on_channel_joined_cb_(response);
    }
}

void WebSocketClient::handle_user_joined(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();
    
    // Server sends channel_state with nested user object
    protocol::UserJoinedNotification notification;
    notification.channel_id = json["channel_id"].toInt();
    
    // Extract user from nested object
    if (json.contains("user") && json["user"].isObject()) {
        QJsonObject user_obj = json["user"].toObject();
        notification.user_id = user_obj["id"].toInt();
        notification.username = user_obj["name"].toString().toStdString();  // Server uses "name", not "username"
        
        std::cout << "👤 User joined channel " << notification.channel_id 
                  << ": " << notification.username 
                  << " (ID: " << notification.user_id << ")\n";
        
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        if (on_user_joined_cb_) {
            on_user_joined_cb_(notification);
        }
    } else {
        std::cerr << "❌ Invalid channel_state message: missing user object\n";
    }
}

void WebSocketClient::handle_user_left(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();
    
    protocol::UserLeftNotification notification;
    notification.channel_id = json["channel_id"].toInt();
    notification.user_id = json["user_id"].toInt();
    
    std::cout << "👤 User left: " << notification.user_id << "\n";
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_user_left_cb_) {
        on_user_left_cb_(notification);
    }
}

void WebSocketClient::handle_error(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();

    protocol::ErrorMessage error;
    error.message = json["message"].toString().toStdString();
    error.code = json["code"].toInt();

    std::cerr << "❌ Server error: " << error.message << " (code: " << error.code << ")\n";

    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_error_cb_) {
        on_error_cb_(error);
    }
}

void WebSocketClient::handle_key_exchange_init(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();

    // Parse server's public key (32 bytes)
    QJsonArray public_key_array = json["public_key"].toArray();
    if (public_key_array.size() != 32) {
        std::cerr << "❌ Invalid public key size: " << public_key_array.size() << std::endl;
        return;
    }

    protocol::KeyExchangeInit key_exchange;
    for (int i = 0; i < 32; ++i) {
        key_exchange.public_key[i] = static_cast<uint8_t>(public_key_array[i].toInt());
    }

    std::cout << "🔑 Parsed server public key (" << key_exchange.public_key.size() << " bytes)" << std::endl;

    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_key_exchange_init_cb_) {
        on_key_exchange_init_cb_(key_exchange);
    }
}

void WebSocketClient::handle_all_channel_rosters(const std::string& json_str) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json_str));
    QJsonObject json = doc.object();

    // Parse channels array
    QJsonArray channels_array = json["channels"].toArray();

    protocol::AllChannelRostersResponse response;
    response.channels.reserve(channels_array.size());

    for (const QJsonValue& ch_val : channels_array) {
        QJsonObject ch_obj = ch_val.toObject();

        protocol::ChannelRosterInfo roster;
        roster.channel_id = ch_obj["channel_id"].toInt();
        roster.channel_name = ch_obj["channel_name"].toString().toStdString();

        // Parse users array
        QJsonArray users_array = ch_obj["users"].toArray();
        roster.users.reserve(users_array.size());

        for (const QJsonValue& user_val : users_array) {
            QJsonObject user_obj = user_val.toObject();

            protocol::UserInfo user;
            user.id = user_obj["id"].toInt();
            user.username = user_obj["name"].toString().toStdString();
            user.speaking = user_obj["speaking"].toBool();
            user.muted = false;  // Server doesn't send this yet

            roster.users.push_back(user);
        }

        response.channels.push_back(roster);
    }

    std::cout << "📊 Parsed rosters for " << response.channels.size() << " channels" << std::endl;

    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (on_all_channel_rosters_cb_) {
        on_all_channel_rosters_cb_(response);
    }
}

} // namespace voip::network
