#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QKeyEvent>
#include <memory>
#include <map>
#include "network/websocket_client.h"
#include "session/voice_session.h"
#include "protocol/control_messages.h"
#include "ui/hotkey_manager.h"
#include "ui/channel_widget.h"
#include "ui/admin/admin_panel.h"
#include <QTabWidget>

namespace voip::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void setUserInfo(const QString& username, uint32_t userId);
    void setLoginCredentials(const QString& username, const QString& password);
    void setWebSocketClient(std::shared_ptr<network::WebSocketClient> wsClient);
    void setVoiceSession(std::shared_ptr<session::VoiceSession> voiceSession);
    void setUserPermissions(uint32_t permissions, uint32_t orgId = 0);

signals:
    void disconnectRequested();
    void channelJoinRequested(uint32_t channelId);

private slots:
    void onChannelItemDoubleClicked(QListWidgetItem* item);
    void onMuteToggled(bool checked);
    void onDeafenToggled(bool checked);
    void onPushToTalkToggled(bool checked);
    void onDisconnectClicked();
    void onSettingsClicked();
    void onVoiceStatsUpdate();
    
    // WebSocket callbacks
    void onWsConnected();
    void onWsDisconnected();
    void onWsError(const std::string& error);
    void onWsChannelJoined(uint32_t channelId, const std::string& channelName);
    void onWsUserJoined(uint32_t userId, const std::string& username);
    void onWsUserLeft(uint32_t userId);
    
    // Multi-channel callbacks
    void onChannelListenToggled(ChannelId id, bool listen);
    void onChannelMuteToggled(ChannelId id);
    void onChannelTransmitSelected(ChannelId id);
    void onChannelHotkeyChangeRequested(ChannelId id);
    void onHotkeyPressed(ChannelId id);
    void onHotkeyReleased(ChannelId id);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void setupUI();
    void updateVoiceStats();
    void addLogMessage(const QString& message);
    void setupWebSocketCallbacks();
    void setPushToTalkActive(bool active);
    void updateMicrophoneState();
    void setupMultiChannelUI();
    void updateChannelWidgets();
    void createDefaultChannels();
    void setupAdminTab();
    void updateAdminTabVisibility();

    // UI Components
    QTabWidget* mainTabs_;
    QWidget* mainPanel_;  // Main VoIP panel
    QListWidget* channelList_;
    QListWidget* userList_;
    QWidget* channelContainer_;
    QVBoxLayout* channelLayout_;
    QPushButton* muteButton_;
    QPushButton* deafenButton_;
    QPushButton* pttButton_;
    QLabel* usernameLabel_;
    QLabel* pttIndicator_;
    QLabel* inputLevelLabel_;
    QLabel* outputLevelLabel_;
    QLabel* latencyLabel_;
    QLabel* qualityLabel_;
    QProgressBar* inputMeter_;
    QProgressBar* outputMeter_;
    QListWidget* logWidget_;
    
    // Admin components
    admin::AdminPanel* adminPanel_ = nullptr;
    int adminTabIndex_ = -1;

    // Backend
    std::shared_ptr<network::WebSocketClient> wsClient_;
    std::shared_ptr<session::VoiceSession> voiceSession_;
    HotkeyManager* hotkeyManager_ = nullptr;
    std::map<ChannelId, ChannelWidget*> channelWidgets_;
    
    // State
    QString username_;
    QString password_;
    uint32_t userId_ = 0;
    uint32_t orgId_ = 0;
    uint32_t permissions_ = 0;
    uint32_t currentChannelId_ = 0;
    bool isMuted_ = false;
    bool isDeafened_ = false;
    bool isPushToTalkMode_ = false;
    bool isPushToTalkActive_ = false;
    bool isAdmin_ = false;
    Qt::Key pttKey_ = Qt::Key_Control;
    
    // Stats timer
    QTimer* statsTimer_ = nullptr;
};

} // namespace voip::ui
