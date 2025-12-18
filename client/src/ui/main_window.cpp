#include "ui/main_window.h"
#include "ui/settings_dialog.h"
#include "ui/hotkey_input_dialog.h"
#include "ui/admin/channel_manager.h"
#include "api/admin_api_client.h"
#include "crypto/key_exchange.h"
#include "crypto/srtp_session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollArea>
#include <QDateTime>
#include <QMessageBox>
#include <QStatusBar>
#include <QInputDialog>
#include <iostream>

namespace voip::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    
    // Setup multi-channel UI
    setupMultiChannelUI();
    
    // Setup stats update timer (update every second)
    statsTimer_ = new QTimer(this);
    connect(statsTimer_, &QTimer::timeout, this, &MainWindow::onVoiceStatsUpdate);
    statsTimer_->start(1000);
    
    setWindowTitle("VoIP Client - Multi-Channel");
    resize(1000, 700);
    
    addLogMessage("Welcome to VoIP Client!");
    addLogMessage("🎮 Multi-channel support enabled!");
}

MainWindow::~MainWindow() {
    std::cout << "🏠 MainWindow destructor called" << std::endl;
    
    // Clean up voice session FIRST (most important for proper socket cleanup)
    if (voiceSession_) {
        std::cout << "  🎙️ Cleaning up voice session..." << std::endl;
        voiceSession_->stop();
        voiceSession_->shutdown();
        voiceSession_.reset();
    }
    
    // Clean up WebSocket
    if (wsClient_) {
        std::cout << "  🌐 Cleaning up WebSocket..." << std::endl;
        wsClient_->disconnect();
        wsClient_.reset();
    }
    
    std::cout << "✅ MainWindow destroyed cleanly\n";
}

void MainWindow::setupUI() {
    // Create tab widget as central widget
    mainTabs_ = new QTabWidget(this);
    setCentralWidget(mainTabs_);
    
    // Create main VoIP panel
    mainPanel_ = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(mainPanel_);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // User info bar
    auto* userInfoLayout = new QHBoxLayout();
    usernameLabel_ = new QLabel("Not connected", this);
    QFont userFont = usernameLabel_->font();
    userFont.setBold(true);
    usernameLabel_->setFont(userFont);
    userInfoLayout->addWidget(new QLabel("User:", this));
    userInfoLayout->addWidget(usernameLabel_);
    userInfoLayout->addStretch();
    mainLayout->addLayout(userInfoLayout);

    // Main splitter (channels | users | log)
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left - Channels (Multi-Channel)
    auto* channelGroup = new QGroupBox("📢 Channels (Multi-Channel)", this);
    channelLayout_ = new QVBoxLayout(channelGroup);
    
    // Create scrollable container for channel widgets
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    channelContainer_ = new QWidget(this);
    auto* containerLayout = new QVBoxLayout(channelContainer_);
    containerLayout->setSpacing(2);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addStretch();
    
    scrollArea->setWidget(channelContainer_);
    channelLayout_->addWidget(scrollArea);
    
    // Keep old channel list for legacy compatibility (hidden)
    channelList_ = new QListWidget(this);
    channelList_->setVisible(false);
    
    splitter->addWidget(channelGroup);

    // Middle - Users
    auto* userGroup = new QGroupBox("👤 Users", this);
    auto* userLayout = new QVBoxLayout(userGroup);
    userList_ = new QListWidget(this);
    userLayout->addWidget(userList_);
    splitter->addWidget(userGroup);

    // Right - Log
    auto* logGroup = new QGroupBox("📋 Activity Log", this);
    auto* logLayout = new QVBoxLayout(logGroup);
    logWidget_ = new QListWidget(this);
    logWidget_->setWordWrap(true);
    logLayout->addWidget(logWidget_);
    splitter->addWidget(logGroup);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 3);
    
    mainLayout->addWidget(splitter, 1);

    // Voice controls
    auto* controlsGroup = new QGroupBox("🎤 Voice Controls", this);
    auto* controlsLayout = new QVBoxLayout(controlsGroup);

    // Audio levels
    auto* levelsLayout = new QHBoxLayout();
    
    levelsLayout->addWidget(new QLabel("🎤 Input:", this));
    inputMeter_ = new QProgressBar(this);
    inputMeter_->setRange(0, 100);
    inputMeter_->setValue(0);
    inputMeter_->setTextVisible(false);
    inputMeter_->setMaximumHeight(20);
    levelsLayout->addWidget(inputMeter_, 1);
    
    inputLevelLabel_ = new QLabel("0%", this);
    inputLevelLabel_->setMinimumWidth(60);
    levelsLayout->addWidget(inputLevelLabel_);
    
    levelsLayout->addSpacing(20);
    
    levelsLayout->addWidget(new QLabel("🔊 Output:", this));
    outputMeter_ = new QProgressBar(this);
    outputMeter_->setRange(0, 100);
    outputMeter_->setValue(0);
    outputMeter_->setTextVisible(false);
    outputMeter_->setMaximumHeight(20);
    levelsLayout->addWidget(outputMeter_, 1);
    
    outputLevelLabel_ = new QLabel("0%", this);
    outputLevelLabel_->setMinimumWidth(60);
    levelsLayout->addWidget(outputLevelLabel_);
    
    controlsLayout->addLayout(levelsLayout);

    // Stats
    auto* statsLayout = new QHBoxLayout();
    latencyLabel_ = new QLabel("Latency: --", this);
    qualityLabel_ = new QLabel("Quality: --", this);
    statsLayout->addWidget(latencyLabel_);
    statsLayout->addSpacing(20);
    statsLayout->addWidget(qualityLabel_);
    statsLayout->addStretch();
    controlsLayout->addLayout(statsLayout);

    // Push-to-Talk indicator
    auto* pttLayout = new QHBoxLayout();
    pttIndicator_ = new QLabel("🎤 Voice: Always On", this);
    QFont pttFont = pttIndicator_->font();
    pttFont.setBold(true);
    pttIndicator_->setFont(pttFont);
    pttIndicator_->setStyleSheet("QLabel { color: #23a55a; padding: 5px; }");
    pttLayout->addWidget(pttIndicator_);
    pttLayout->addStretch();
    controlsLayout->addLayout(pttLayout);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    muteButton_ = new QPushButton("🔇 Mute", this);
    muteButton_->setCheckable(true);
    muteButton_->setMinimumWidth(100);
    buttonLayout->addWidget(muteButton_);
    
    deafenButton_ = new QPushButton("🔕 Deafen", this);
    deafenButton_->setCheckable(true);
    deafenButton_->setMinimumWidth(100);
    buttonLayout->addWidget(deafenButton_);
    
    pttButton_ = new QPushButton("⌨️ Push-to-Talk: OFF", this);
    pttButton_->setCheckable(true);
    pttButton_->setMinimumWidth(150);
    pttButton_->setToolTip("Hold Ctrl key to talk");
    buttonLayout->addWidget(pttButton_);
    
    buttonLayout->addStretch();
    
    auto* settingsButton = new QPushButton("⚙️ Settings", this);
    buttonLayout->addWidget(settingsButton);
    
    auto* disconnectButton = new QPushButton("🚪 Disconnect", this);
    buttonLayout->addWidget(disconnectButton);
    
    controlsLayout->addLayout(buttonLayout);
    mainLayout->addWidget(controlsGroup);

    // Connect signals
    connect(muteButton_, &QPushButton::toggled, this, &MainWindow::onMuteToggled);
    connect(deafenButton_, &QPushButton::toggled, this, &MainWindow::onDeafenToggled);
    connect(pttButton_, &QPushButton::toggled, this, &MainWindow::onPushToTalkToggled);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(channelList_, &QListWidget::itemDoubleClicked, this, &MainWindow::onChannelItemDoubleClicked);

    // Add main panel as first tab
    mainTabs_->addTab(mainPanel_, "🎤 Voice");
    
    // Setup admin tab (hidden by default)
    setupAdminTab();

    statusBar()->showMessage("Ready");
}

void MainWindow::setUserInfo(const QString& username, uint32_t userId) {
    username_ = username;
    userId_ = userId;
    usernameLabel_->setText(username);
    
    // Add yourself to user list
    auto* item = new QListWidgetItem(username + " (You)");
    item->setData(Qt::UserRole, userId);
    QFont font = item->font();
    font.setBold(true);
    item->setFont(font);
    userList_->addItem(item);
    
    addLogMessage(QString("Logged in as: %1 (ID: %2)").arg(username).arg(userId));
}

void MainWindow::setLoginCredentials(const QString& username, const QString& password) {
    username_ = username;
    password_ = password;
}

void MainWindow::setServerInfo(const QString& address, uint16_t port, bool useTls) {
    serverAddress_ = address;
    serverPort_ = port;
    useTls_ = useTls;

    std::cout << "Server info set: " << address.toStdString()
              << ":" << port << " (TLS: " << (useTls ? "enabled" : "disabled") << ")" << std::endl;
}

void MainWindow::setWebSocketClient(std::shared_ptr<network::WebSocketClient> wsClient) {
    wsClient_ = wsClient;
    setupWebSocketCallbacks();
    addLogMessage("WebSocket client connected");
}

void MainWindow::setupWebSocketCallbacks() {
    if (!wsClient_) return;
    
    // Set up WebSocket callbacks using correct API names
    wsClient_->set_connected_callback([this]() {
        // First, call the UI update
        QMetaObject::invokeMethod(this, "onWsConnected", Qt::QueuedConnection);
        
        // Then, automatically send login request
        std::cout << "✅ WebSocket CONNECTED in MainWindow! Now logging in as: " 
                  << username_.toStdString() << std::endl;
        auto loginResult = wsClient_->login(username_.toStdString(), 
                                           password_.toStdString(), 
                                           "DEFAULT");
        if (!loginResult.is_ok()) {
            std::cout << "❌ Login FAILED: " << loginResult.error().message() << std::endl;
        } else {
            std::cout << "✅ Login request sent successfully" << std::endl;
        }
    });
    
    wsClient_->set_disconnected_callback([this]() {
        QMetaObject::invokeMethod(this, "onWsDisconnected", Qt::QueuedConnection);
    });
    
    // After login response, auto-join channel 1
    wsClient_->set_login_callback([this](const protocol::LoginResponse& response) {
        if (response.success) {
            std::cout << "✅ Login SUCCESS! User ID: " << response.user_id 
                      << ", Org ID: " << response.org_id 
                      << ", Permissions: 0x" << std::hex << response.permissions << std::dec
                      << ". Now joining channel 1..." << std::endl;
            
            // Store authenticated user ID
            userId_ = response.user_id;
            
            // Set user permissions for admin panel access
            setUserPermissions(response.permissions, response.org_id);
            
            // Update voice session with correct user ID
            if (voiceSession_) {
                std::cout << "🔧 Updating VoiceSession user ID from hardcoded 42 to " 
                          << userId_ << std::endl;
                voiceSession_->set_user_id(userId_);
            }
            
            auto joinResult = wsClient_->join_channel(1);
            if (!joinResult.is_ok()) {
                std::cout << "❌ Join channel FAILED: " << joinResult.error().message() << std::endl;
            } else {
                std::cout << "✅ Join channel 1 request sent" << std::endl;
            }
        } else {
            std::cout << "❌ Login FAILED: " << response.error_message << std::endl;
        }
    });
    
    wsClient_->set_error_callback([this](const protocol::ErrorMessage& error) {
        std::string errorMsg = error.message;
        QMetaObject::invokeMethod(this, "onWsError", Qt::QueuedConnection,
            Q_ARG(std::string, errorMsg));
    });

    // Handle SRTP key exchange
    wsClient_->set_key_exchange_init_callback([this](const protocol::KeyExchangeInit& init) {
        std::cout << "🔑 Received server public key, performing key exchange..." << std::endl;

        try {
            // Create our key exchange instance
            auto kx = std::make_unique<crypto::KeyExchange>();

            // Get our public key to send to server
            auto our_public_key = kx->public_key_bytes();

            // Derive shared keys using server's public key
            auto key_material = kx->derive_keys(init.public_key);

            // Send our public key back to server
            auto send_result = wsClient_->send_key_exchange_response(our_public_key);
            if (!send_result.is_ok()) {
                std::cerr << "❌ Failed to send key exchange response: "
                          << send_result.error().message() << std::endl;
                return;
            }

            // Create SRTP session with derived keys
            auto srtp_session = std::make_unique<crypto::SrtpSession>(
                key_material.master_key,
                key_material.salt
            );

            // Install SRTP session in voice session
            voiceSession_->set_srtp_session(std::move(srtp_session));

            std::cout << "✅ SRTP key exchange complete - encrypted voice active!" << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "❌ Key exchange failed: " << e.what() << std::endl;
        }
    });

    wsClient_->set_channel_joined_callback([this](const protocol::ChannelJoinedResponse& response) {
        std::string channelName = "Channel " + std::to_string(response.channel_id);
        
        std::cout << "DEBUG: ChannelJoined callback - channel_id=" << response.channel_id 
                  << " users.size()=" << response.users.size() << std::endl;
        
        // Store users list for onWsChannelJoined to populate
        // DON'T call onWsUserJoined here - that's only for notifications of NEW users joining
        std::vector<protocol::UserInfo> users = response.users;
        
        for (const auto& user : users) {
            std::cout << "DEBUG: User in channel: id=" << user.id 
                      << " username=" << user.username << std::endl;
        }
        
        QMetaObject::invokeMethod(this, [this, response, channelName]() {
            onWsChannelJoined(response.channel_id, channelName);
            
            // Populate user list with ALL users (including self)
            userList_->clear();
            for (const auto& user : response.users) {
                auto* item = new QListWidgetItem(QString::fromStdString(user.username));
                item->setData(Qt::UserRole, user.id);
                userList_->addItem(item);
                std::cout << "DEBUG: Added user to list: " << user.username << " (ID: " << user.id << ")\n";
            }
            
            std::cout << "DEBUG: User list populated with " << response.users.size() << " users\n";
            
            // START VOICE SESSION NOW that we've joined the channel!
            if (voiceSession_ && !voiceSession_->is_active()) {
                std::cout << "🎤 Starting voice session now that channel is joined..." << std::endl;
                auto startResult = voiceSession_->start();
                if (startResult.is_ok()) {
                    std::cout << "✅ Voice session started successfully!" << std::endl;
                    addLogMessage("✅ Voice session ready - you can now use PTT!");
                    
                    // Send presence packet to register UDP address with server
                    // This ensures immediate voice routing without waiting for first transmission
                    voiceSession_->send_presence_packet(response.channel_id);
                } else {
                    std::cerr << "❌ Failed to start voice session: " << startResult.error().message() << std::endl;
                }
            } else if (voiceSession_ && voiceSession_->is_active()) {
                // Voice session already active (switching channels)
                // Send presence packet for new channel
                std::cout << "🔄 Switching to channel " << response.channel_id << " - registering UDP address..." << std::endl;
                voiceSession_->send_presence_packet(response.channel_id);
            }
        }, Qt::QueuedConnection);
    });
    
    wsClient_->set_user_joined_callback([this](const protocol::UserJoinedNotification& notification) {
        QMetaObject::invokeMethod(this, "onWsUserJoined", Qt::QueuedConnection,
            Q_ARG(uint32_t, notification.user_id), 
            Q_ARG(std::string, notification.username));
    });
    
    wsClient_->set_user_left_callback([this](const protocol::UserLeftNotification& notification) {
        QMetaObject::invokeMethod(this, "onWsUserLeft", Qt::QueuedConnection,
            Q_ARG(uint32_t, notification.user_id));
    });
}

void MainWindow::setVoiceSession(std::shared_ptr<session::VoiceSession> voiceSession) {
    voiceSession_ = voiceSession;
    
    if (voiceSession_) {
        addLogMessage("✅ Voice session initialized (waiting for channel join...)");
        
        // Auto-join Channel 1 (General) for listening by default
        auto result = voiceSession_->join_channel(1);
        if (result.is_ok()) {
            addLogMessage("📢 Auto-joined Channel 1 (General) for listening");
            
            // Update widget state
            if (channelWidgets_.count(1)) {
                channelWidgets_[1]->setJoined(true);
                channelWidgets_[1]->setListening(true);
            }
            
            // Notify server via WebSocket
            if (wsClient_) {
                wsClient_->join_channel(1);
            }
        }
        
        addLogMessage("🎤 Press F1-F4 to transmit to channels");
        addLogMessage("💡 Click 'Listen' to join more channels");
        statusBar()->showMessage("Voice: Connected | Channel 1 active");
    }
}

void MainWindow::onChannelItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    
    uint32_t channelId = item->data(Qt::UserRole).toUInt();
    emit channelJoinRequested(channelId);
    addLogMessage(QString("📢 Joining channel: %1").arg(item->text()));
}

void MainWindow::onMuteToggled(bool checked) {
    if (isPushToTalkMode_) {
        // In PTT mode, manual mute is disabled
        muteButton_->setChecked(!checked);
        addLogMessage("⚠️ Mute disabled in Push-to-Talk mode");
        return;
    }
    
    isMuted_ = checked;
    updateMicrophoneState();
    
    if (checked) {
        muteButton_->setText("🔊 Unmute");
        addLogMessage("🔇 Microphone muted");
        statusBar()->showMessage("Voice: Muted");
    } else {
        muteButton_->setText("🔇 Mute");
        addLogMessage("🎤 Microphone active");
        statusBar()->showMessage("Voice: Connected");
    }
}

void MainWindow::onDeafenToggled(bool checked) {
    isDeafened_ = checked;
    
    if (checked) {
        // Deafening also mutes
        isMuted_ = true;
        muteButton_->setChecked(true);
        muteButton_->setEnabled(false);
        deafenButton_->setText("🔔 Undeafen");
        addLogMessage("🔕 Deafened (muted + no audio output)");
        statusBar()->showMessage("Voice: Deafened");
    } else {
        muteButton_->setEnabled(true);
        deafenButton_->setText("🔕 Deafen");
        addLogMessage("🔊 Undeafened");
        statusBar()->showMessage("Voice: Connected");
    }
    
    // TODO: Actually control voice session audio output
}

void MainWindow::onDisconnectClicked() {
    auto reply = QMessageBox::question(this, "Disconnect", 
        "Are you sure you want to disconnect?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        emit disconnectRequested();
        addLogMessage("Disconnecting...");
        close();
    }
}

void MainWindow::onPushToTalkToggled(bool checked) {
    isPushToTalkMode_ = checked;
    
    if (checked) {
        pttButton_->setText("⌨️ Push-to-Talk: ON");
        pttIndicator_->setText(QString("🎤 Voice: Press %1 to Talk")
            .arg(QKeySequence(pttKey_).toString()));
        pttIndicator_->setStyleSheet("QLabel { color: #f0b232; padding: 5px; }");
        muteButton_->setEnabled(false);
        isMuted_ = true;  // Start muted in PTT mode
        isPushToTalkActive_ = false;
        addLogMessage(QString("⌨️ Push-to-Talk enabled (Hold %1 to talk)")
            .arg(QKeySequence(pttKey_).toString()));
        statusBar()->showMessage("Voice: Push-to-Talk Mode");
    } else {
        pttButton_->setText("⌨️ Push-to-Talk: OFF");
        pttIndicator_->setText("🎤 Voice: Always On");
        pttIndicator_->setStyleSheet("QLabel { color: #23a55a; padding: 5px; }");
        muteButton_->setEnabled(true);
        isPushToTalkActive_ = false;
        isMuted_ = false;  // Unmute when leaving PTT mode
        addLogMessage("🎤 Push-to-Talk disabled (Always On)");
        statusBar()->showMessage("Voice: Connected");
    }
    
    updateMicrophoneState();
}

void MainWindow::onSettingsClicked() {
    if (!voiceSession_) {
        QMessageBox::warning(this, "Settings",
            "Voice session not initialized. Please restart the application.");
        return;
    }
    
    auto* audioEngine = voiceSession_->get_audio_engine();
    if (!audioEngine) {
        QMessageBox::warning(this, "Settings",
            "Audio engine not available. Please restart the application.");
        return;
    }
    
    SettingsDialog settingsDialog(audioEngine, this);
    
    if (settingsDialog.exec() == QDialog::Accepted) {
        addLogMessage("⚙️ Audio settings updated");
        addLogMessage(QString("   Input: %1%").arg(static_cast<int>(settingsDialog.inputVolume() * 100)));
        addLogMessage(QString("   Output: %1%").arg(static_cast<int>(settingsDialog.outputVolume() * 100)));
    }
}

void MainWindow::onVoiceStatsUpdate() {
    updateVoiceStats();
}

void MainWindow::updateVoiceStats() {
    if (!voiceSession_ || !voiceSession_->is_active()) {
        inputMeter_->setValue(0);
        outputMeter_->setValue(0);
        inputLevelLabel_->setText("Not active");
        outputLevelLabel_->setText("Not active");
        latencyLabel_->setText("Latency: --");
        qualityLabel_->setText("Quality: --");
        return;
    }
    
    // Get real stats from voice session
    auto stats = voiceSession_->get_stats();
    
    // Update audio level meters with ACTUAL audio levels
    if (isMuted_) {
        inputMeter_->setValue(0);
        inputLevelLabel_->setText("Muted");
    } else {
        // Get real input level from audio engine
        float inputLevel = voiceSession_->get_input_level();
        int inputPercent = static_cast<int>(inputLevel * 100.0f);
        inputMeter_->setValue(inputPercent);
        inputLevelLabel_->setText(QString::number(inputPercent) + "%");
    }
    
    if (isDeafened_) {
        outputMeter_->setValue(0);
        outputLevelLabel_->setText("Deafened");
    } else {
        // Get real output level from audio engine
        float outputLevel = voiceSession_->get_output_level();
        int outputPercent = static_cast<int>(outputLevel * 100.0f);
        outputMeter_->setValue(outputPercent);
        outputLevelLabel_->setText(QString::number(outputPercent) + "%");
    }
    
    // Update stats display
    latencyLabel_->setText(QString("Latency: %1 ms").arg(stats.estimated_latency_ms, 0, 'f', 1));
    
    // Calculate packet loss percentage
    float packetLoss = 0.0f;
    if (stats.packets_received > 0) {
        packetLoss = (stats.plc_frames * 100.0f) / stats.packets_received;
    }
    
    // Show quality indicator
    QString qualityText;
    if (packetLoss < 1.0f) {
        qualityText = "Quality: ⭐⭐⭐⭐⭐ Excellent";
    } else if (packetLoss < 3.0f) {
        qualityText = "Quality: ⭐⭐⭐⭐ Good";
    } else if (packetLoss < 5.0f) {
        qualityText = "Quality: ⭐⭐⭐ Fair";
    } else {
        qualityText = QString("Quality: ⭐⭐ Poor (%1% loss)").arg(packetLoss, 0, 'f', 1);
    }
    qualityLabel_->setText(qualityText);
}

void MainWindow::addLogMessage(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);
    
    auto* item = new QListWidgetItem(logEntry);
    logWidget_->addItem(item);
    logWidget_->scrollToBottom();
    
    // Keep log size manageable
    while (logWidget_->count() > 1000) {
        delete logWidget_->takeItem(0);
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (isPushToTalkMode_ && !event->isAutoRepeat() && event->key() == pttKey_) {
        setPushToTalkActive(true);
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    if (isPushToTalkMode_ && !event->isAutoRepeat() && event->key() == pttKey_) {
        setPushToTalkActive(false);
        event->accept();
        return;
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::setPushToTalkActive(bool active) {
    if (isPushToTalkActive_ == active) return;
    
    isPushToTalkActive_ = active;
    
    if (active) {
        pttIndicator_->setText("🎤 Voice: TRANSMITTING");
        pttIndicator_->setStyleSheet("QLabel { color: #da373c; padding: 5px; " 
                                     "background-color: rgba(218, 55, 60, 0.2); " 
                                     "border-radius: 3px; }");
        isMuted_ = false;
        addLogMessage("🔴 Push-to-Talk: ACTIVE");
    } else {
        pttIndicator_->setText(QString("🎤 Voice: Press %1 to Talk")
            .arg(QKeySequence(pttKey_).toString()));
        pttIndicator_->setStyleSheet("QLabel { color: #f0b232; padding: 5px; }");
        isMuted_ = true;
        addLogMessage("⚪ Push-to-Talk: Released");
    }
    
    updateMicrophoneState();
}

void MainWindow::updateMicrophoneState() {
    if (!voiceSession_) {
        return;  // No voice session yet
    }
    
    // Update voice session with current mute/deafen state
    voiceSession_->set_muted(isMuted_);
    voiceSession_->set_deafened(isDeafened_);
    
    // Update status bar to reflect current state
    if (isMuted_ && isDeafened_) {
        statusBar()->showMessage("Voice: Muted & Deafened");
    } else if (isDeafened_) {
        statusBar()->showMessage("Voice: Deafened");
    } else if (isMuted_) {
        statusBar()->showMessage("Voice: Muted");
    } else if (isPushToTalkMode_ && !isPushToTalkActive_) {
        statusBar()->showMessage("Voice: PTT Ready");
    } else {
        statusBar()->showMessage("Voice: Connected");
    }
}

// WebSocket callback implementations
void MainWindow::onWsConnected() {
    addLogMessage("✅ WebSocket Connected to server");
    statusBar()->showMessage("Status: Connected");
    std::cout << "DEBUG: WebSocket connected!" << std::endl;
}

void MainWindow::onWsDisconnected() {
    addLogMessage("⚠️ Disconnected from server");
    statusBar()->showMessage("Status: Disconnected");
}

void MainWindow::onWsError(const std::string& error) {
    addLogMessage(QString("❌ WebSocket error: %1").arg(QString::fromStdString(error)));
}

void MainWindow::onWsChannelJoined(uint32_t channelId, const std::string& channelName) {
    currentChannelId_ = channelId;
    addLogMessage(QString("📢 Joined channel: %1 (ID: %2)")
        .arg(QString::fromStdString(channelName))
        .arg(channelId));
    
    std::cout << "DEBUG: Channel joined - ID: " << channelId << std::endl;
    
    // Highlight the channel in the list
    for (int i = 0; i < channelList_->count(); ++i) {
        auto* item = channelList_->item(i);
        if (item->data(Qt::UserRole).toUInt() == channelId) {
            channelList_->setCurrentItem(item);
            break;
        }
    }
    
    std::cout << "DEBUG: Current user list count: " << userList_->count() << std::endl;
    addLogMessage(QString("💡 User list has %1 users").arg(userList_->count()));
}

void MainWindow::onWsUserJoined(uint32_t userId, const std::string& username) {
    std::cout << "DEBUG: onWsUserJoined called - userId=" << userId 
              << " username=" << username 
              << " myUserId=" << userId_ << std::endl;
    
    if (userId == userId_) {
        std::cout << "DEBUG: Skipping self" << std::endl;
        return;  // Don't add yourself again
    }
    
    auto* item = new QListWidgetItem(QString::fromStdString(username));
    item->setData(Qt::UserRole, userId);
    userList_->addItem(item);
    
    std::cout << "DEBUG: Added user to list. New count: " << userList_->count() << std::endl;
    
    addLogMessage(QString("👤 %1 joined the channel")
        .arg(QString::fromStdString(username)));
}

void MainWindow::onWsUserLeft(uint32_t userId) {
    for (int i = 0; i < userList_->count(); ++i) {
        auto* item = userList_->item(i);
        if (item->data(Qt::UserRole).toUInt() == userId) {
            QString username = item->text();
            delete userList_->takeItem(i);
            addLogMessage(QString("👋 %1 left the channel").arg(username));
            break;
        }
    }
}

// === MULTI-CHANNEL IMPLEMENTATION ===

void MainWindow::setupMultiChannelUI() {
    std::cout << "DEBUG: setupMultiChannelUI() called" << std::endl;
    
    // Create hotkey manager
    hotkeyManager_ = new HotkeyManager(this);
    
    // Connect hotkey signals to PTT functions
    connect(hotkeyManager_, &HotkeyManager::hotkeyPressed,
            this, &MainWindow::onHotkeyPressed);
    connect(hotkeyManager_, &HotkeyManager::hotkeyReleased,
            this, &MainWindow::onHotkeyReleased);
    
    // Create default channels
    createDefaultChannels();
    
    addLogMessage("⌨️ Hotkey system initialized");
}

void MainWindow::createDefaultChannels() {
    // Create default channels
    struct ChannelDef {
        ChannelId id;
        QString name;
        Qt::Key hotkey;
    };
    
    std::vector<ChannelDef> channels = {
        {1, "General", Qt::Key_F1},
        {2, "Operations", Qt::Key_F2},
        {3, "Alpha Squad", Qt::Key_F3},
        {4, "Bravo Squad", Qt::Key_F4},
        {5, "Social", Qt::Key_F5}
    };
    
    // Get layout from container
    auto* layout = channelContainer_->layout();
    
    for (const auto& def : channels) {
        // Create channel widget
        auto* widget = new ChannelWidget(def.id, def.name, this);
        
        // Register hotkey
        hotkeyManager_->registerHotkey(def.id, QKeySequence(def.hotkey));
        widget->setHotkey(QKeySequence(def.hotkey));
        
        // Connect signals
        connect(widget, &ChannelWidget::listenToggled,
                this, &MainWindow::onChannelListenToggled);
        connect(widget, &ChannelWidget::muteToggled,
                this, &MainWindow::onChannelMuteToggled);
        connect(widget, &ChannelWidget::transmitSelected,
                this, &MainWindow::onChannelTransmitSelected);
        connect(widget, &ChannelWidget::hotkeyChangeRequested,
                this, &MainWindow::onChannelHotkeyChangeRequested);
        
        // Store widget
        channelWidgets_[def.id] = widget;
        
        // Add to layout (before stretch)
        layout->removeItem(layout->itemAt(layout->count() - 1)); // Remove stretch
        layout->addWidget(widget);
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    
    addLogMessage(QString("📢 Created %1 channels with hotkeys").arg(channels.size()));
}

void MainWindow::onChannelListenToggled(ChannelId id, bool listen) {
    if (!voiceSession_) return;
    
    if (listen) {
        auto result = voiceSession_->join_channel(id);
        if (result.is_ok()) {
            addLogMessage(QString("✅ Joined channel %1 for listening").arg(id));
            
            // Update widget state
            if (channelWidgets_.count(id)) {
                channelWidgets_[id]->setJoined(true);
                channelWidgets_[id]->setListening(true);
            }
            
            // Send join message to server
            if (wsClient_) {
                wsClient_->join_channel(id);
            }
        } else {
            addLogMessage(QString("❌ Failed to join channel %1").arg(id));
        }
    } else {
        auto result = voiceSession_->leave_channel(id);
        if (result.is_ok()) {
            addLogMessage(QString("👋 Left channel %1").arg(id));
            
            // Update widget state
            if (channelWidgets_.count(id)) {
                channelWidgets_[id]->setJoined(false);
                channelWidgets_[id]->setListening(false);
            }
            
            // Send leave message to server (WebSocketClient doesn't support per-channel leave yet)
            // TODO: Implement multi-channel leave in WebSocketClient
            // if (wsClient_) {
            //     wsClient_->leave_channel();
            // }
        }
    }
}

void MainWindow::onChannelMuteToggled(ChannelId id) {
    if (!voiceSession_) return;
    
    bool currentlyMuted = voiceSession_->is_channel_muted(id);
    voiceSession_->set_channel_muted(id, !currentlyMuted);
    
    // Update widget
    if (channelWidgets_.count(id)) {
        channelWidgets_[id]->setMuted(!currentlyMuted);
    }
    
    if (!currentlyMuted) {
        addLogMessage(QString("🔇 Muted channel %1").arg(id));
    } else {
        addLogMessage(QString("🔊 Unmuted channel %1").arg(id));
    }
}

void MainWindow::onChannelTransmitSelected(ChannelId id) {
    if (!voiceSession_) return;
    
    // Check current hot mic channel
    ChannelId currentHotMic = voiceSession_->get_hot_mic_channel();
    
    if (currentHotMic == id) {
        // Disable hot mic
        voiceSession_->set_hot_mic_channel(0);
        addLogMessage("🎤 Hot mic disabled");
    } else {
        // Set as hot mic channel
        voiceSession_->set_hot_mic_channel(id);
        addLogMessage(QString("🎤 Hot mic set to channel %1").arg(id));
    }
    
    updateChannelWidgets();
}

void MainWindow::onChannelHotkeyChangeRequested(ChannelId id) {
    // Show hotkey capture dialog
    HotkeyInputDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QKeySequence newKey = dialog.capturedKey();

        if (!newKey.isEmpty()) {
            // Unregister old hotkey
            hotkeyManager_->unregisterHotkey(id);

            // Register new hotkey
            if (hotkeyManager_->registerHotkey(id, newKey)) {
                if (channelWidgets_.count(id)) {
                    channelWidgets_[id]->setHotkey(newKey);
                }
                addLogMessage(QString("⌨️ Channel %1 hotkey changed to %2").arg(id).arg(newKey.toString()));
            } else {
                QMessageBox::warning(this, "Hotkey Conflict",
                                   "This hotkey is already in use by another channel!");
                // Re-register the old hotkey if new one conflicts
                // (We could store the old one but for now just leave it unregistered)
            }
        }
    }
}

void MainWindow::onHotkeyPressed(ChannelId id) {
    if (!voiceSession_) {
        std::cout << "❌ Hotkey pressed but no voice session!" << std::endl;
        return;
    }
    
    std::cout << "⌨️ F" << id << " pressed - Starting PTT for channel " << id << std::endl;
    
    voiceSession_->start_ptt(id);
    
    // Update widget
    if (channelWidgets_.count(id)) {
        channelWidgets_[id]->setPTTActive(true);
        std::cout << "  ✅ Widget updated - transmit button should turn orange" << std::endl;
    } else {
        std::cout << "  ⚠️ No widget found for channel " << id << std::endl;
    }
    
    // Check if we're actually joined to this channel
    auto joined_channels = voiceSession_->get_joined_channels();
    bool is_joined = joined_channels.count(id) > 0;
    std::cout << "  📡 Channel " << id << " joined for listening: " << (is_joined ? "YES" : "NO") << std::endl;
    
    // Check active PTT channels
    auto ptt_channels = voiceSession_->get_active_ptt_channels();
    std::cout << "  🎤 Active PTT channels: ";
    for (auto ch : ptt_channels) {
        std::cout << ch << " ";
    }
    std::cout << std::endl;
    
    addLogMessage(QString("🎤 PTT started for channel %1").arg(id));
}

void MainWindow::onHotkeyReleased(ChannelId id) {
    if (!voiceSession_) return;
    
    std::cout << "⌨️ F" << id << " released - Stopping PTT for channel " << id << std::endl;
    
    voiceSession_->stop_ptt(id);
    
    // Update widget
    if (channelWidgets_.count(id)) {
        channelWidgets_[id]->setPTTActive(false);
        std::cout << "  ✅ Widget updated - transmit button should return to gray" << std::endl;
    }
    
    // Check remaining active PTT channels
    auto ptt_channels = voiceSession_->get_active_ptt_channels();
    std::cout << "  🎤 Remaining PTT channels: ";
    if (ptt_channels.empty()) {
        std::cout << "(none)";
    } else {
        for (auto ch : ptt_channels) {
            std::cout << ch << " ";
        }
    }
    std::cout << std::endl;
    
    addLogMessage(QString("🔇 PTT stopped for channel %1").arg(id));
}

void MainWindow::updateChannelWidgets() {
    if (!voiceSession_) return;
    
    ChannelId hotMicChannel = voiceSession_->get_hot_mic_channel();
    auto pttChannels = voiceSession_->get_active_ptt_channels();
    
    for (auto& [id, widget] : channelWidgets_) {
        // Update hot mic state
        widget->setHotMic(id == hotMicChannel);
        
        // Update PTT state
        widget->setPTTActive(pttChannels.count(id) > 0);
    }
}

void MainWindow::setupAdminTab() {
    // Create admin panel
    adminPanel_ = new admin::AdminPanel(this);

    // Add admin tab (hidden by default until permissions are set)
    adminTabIndex_ = mainTabs_->addTab(adminPanel_, "🔧 Admin");
    mainTabs_->setTabVisible(adminTabIndex_, false);

    // Connect channel manager signals to refresh voice tab
    if (adminPanel_->getChannelManager()) {
        connect(adminPanel_->getChannelManager(), &admin::ChannelManager::channelsChanged,
                this, &MainWindow::onAdminChannelsChanged);
    }

    addLogMessage("Admin panel initialized (hidden until admin login)");
}

void MainWindow::updateAdminTabVisibility() {
    if (adminTabIndex_ >= 0 && mainTabs_) {
        mainTabs_->setTabVisible(adminTabIndex_, isAdmin_);
        
        if (isAdmin_) {
            addLogMessage("✅ Admin panel enabled - Admin access granted");
        }
    }
}

void MainWindow::setUserPermissions(uint32_t permissions, uint32_t orgId) {
    permissions_ = permissions;
    orgId_ = orgId;
    
    // Check if user has admin permission (bit 0x01 for example)
    // TODO: Define proper permission constants
    isAdmin_ = (permissions & 0x01) != 0;  // Simple admin check
    
    // Update admin tab visibility
    updateAdminTabVisibility();
    
    // Set admin context if admin panel exists
    if (isAdmin_ && adminPanel_) {
        adminPanel_->setUserContext(userId_, orgId_, permissions_);

        // Set API base URL using server info from login
        auto apiClient = adminPanel_->getApiClient();
        if (apiClient) {
            QString protocol = useTls_ ? "https" : "http";
            QString baseUrl = QString("%1://%2:%3")
                .arg(protocol)
                .arg(serverAddress_)
                .arg(serverPort_);

            std::cout << "Setting admin API base URL: " << baseUrl.toStdString() << std::endl;
            apiClient->setBaseUrl(baseUrl);

            // Set JWT token if available
            if (!jwtToken_.isEmpty()) {
                apiClient->setAuthToken(jwtToken_);
                std::cout << "JWT token set for admin API" << std::endl;
            } else {
                std::cout << "Warning: No JWT token available for admin API" << std::endl;
            }
        }

        addLogMessage(QString("Admin permissions set - Org: %1, Perms: 0x%2")
                     .arg(orgId_).arg(permissions_, 0, 16));
    }
}

void MainWindow::onAdminChannelsChanged() {
    // Reload channels from server when admin creates/edits/deletes channels
    addLogMessage("📡 Channels updated via admin panel - refreshing voice tab...");

    auto apiClient = adminPanel_->getApiClient();
    if (!apiClient) {
        addLogMessage("⚠️ Cannot refresh channels: No API client available");
        return;
    }

    // Get channels from server
    apiClient->getChannels([this](const QJsonArray& channels) {
        // Clear existing channel widgets
        for (auto& pair : channelWidgets_) {
            delete pair.second;
        }
        channelWidgets_.clear();

        // Get layout from container
        auto* layout = channelContainer_->layout();

        // Remove all widgets from layout except the stretch at the end
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        // Recreate channel widgets from server data
        int hotkeyIndex = 0;
        Qt::Key hotkeys[] = {Qt::Key_F1, Qt::Key_F2, Qt::Key_F3, Qt::Key_F4, Qt::Key_F5,
                            Qt::Key_F6, Qt::Key_F7, Qt::Key_F8, Qt::Key_F9, Qt::Key_F10};

        for (const QJsonValue& value : channels) {
            QJsonObject channel = value.toObject();
            ChannelId id = channel["id"].toInt();
            QString name = channel["name"].toString();

            // Create channel widget
            auto* widget = new ChannelWidget(id, name, this);

            // Assign hotkey (cycle through F1-F10)
            if (hotkeyIndex < 10) {
                Qt::Key hotkey = hotkeys[hotkeyIndex];
                hotkeyManager_->registerHotkey(id, QKeySequence(hotkey));
                widget->setHotkey(QKeySequence(hotkey));
                hotkeyIndex++;
            }

            // Connect signals
            connect(widget, &ChannelWidget::listenToggled,
                    this, &MainWindow::onChannelListenToggled);
            connect(widget, &ChannelWidget::muteToggled,
                    this, &MainWindow::onChannelMuteToggled);
            connect(widget, &ChannelWidget::transmitSelected,
                    this, &MainWindow::onChannelTransmitSelected);
            connect(widget, &ChannelWidget::hotkeyChangeRequested,
                    this, &MainWindow::onChannelHotkeyChangeRequested);

            // Store widget
            channelWidgets_[id] = widget;

            // Add to layout
            layout->addWidget(widget);
        }

        // Add stretch at the end
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

        addLogMessage(QString("✅ Reloaded %1 channels from server").arg(channels.size()));
    });
}

} // namespace voip::ui
