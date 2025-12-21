#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeySequence>
#include <vector>
#include "common/types.h"
#include "ui/channel_roster_manager.h"

namespace voip::ui {

/**
 * ChannelWidget - Rocker switch control for a single channel
 * 
 * Visual Layout:
 * ┌──────────────────────────────────────────┐
 * │ # Channel Name          🎧 ON    🎤 HOT  │
 * │ [Listen] [Transmit]     [F1]             │
 * └──────────────────────────────────────────┘
 * 
 * States:
 * - Listen: ON (green) / MUTED (gray) / OFF (dark)
 * - Transmit: HOT MIC (blue) / PTT (yellow) / OFF (dark)
 */
class ChannelWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ChannelWidget(ChannelId id, const QString& name, bool isSubChannel = false, QWidget* parent = nullptr);
    
    // State setters
    void setJoined(bool joined);
    void setListening(bool listening);
    void setMuted(bool muted);
    void setHotMic(bool active);
    void setPTTActive(bool active);
    void setHotkey(const QKeySequence& key);
    
    // State getters
    ChannelId getChannelId() const { return channel_id_; }
    QString getChannelName() const { return name_; }
    bool isJoined() const { return joined_; }
    bool isListening() const { return listening_; }
    bool isMuted() const { return muted_; }
    bool isHotMic() const { return hot_mic_; }
    bool isPTTActive() const { return ptt_active_; }
    QKeySequence getHotkey() const { return hotkey_; }

    // User list management
    void setUserList(const std::vector<ChannelRosterManager::ChannelUser>& users);
    void setUserCount(int count);
    void setExpanded(bool expanded);
    bool isExpanded() const { return expanded_; }
    
signals:
    // User interactions
    void listenToggled(ChannelId id, bool listen);
    void muteToggled(ChannelId id);
    void transmitSelected(ChannelId id);
    void hotkeyChangeRequested(ChannelId id);
    
private slots:
    void onListenClicked();
    void onTransmitClicked();
    void onHotkeyClicked();
    void onExpandClicked();

private:
    void updateUI();
    void applyStyles();
    void updateUserListUI();
    QWidget* createUserListItem(const ChannelRosterManager::ChannelUser& user);
    
    // Channel info
    ChannelId channel_id_;
    QString name_;
    
    // UI Elements - Main controls
    QLabel* nameLabel_;
    QPushButton* listenButton_;
    QPushButton* transmitButton_;
    QPushButton* hotkeyButton_;
    QLabel* statusLabel_;

    // UI Elements - User list
    QPushButton* expandButton_;     // Arrow button to expand/collapse
    QLabel* userCountLabel_;        // Shows "(3)" user count
    QWidget* userListContainer_;    // Container for user list
    QVBoxLayout* userListLayout_;   // Layout for user items
    QVBoxLayout* mainLayout_;       // Main vertical layout

    // State
    bool joined_ = false;
    bool listening_ = false;
    bool muted_ = false;
    bool hot_mic_ = false;
    bool ptt_active_ = false;
    QKeySequence hotkey_;
    bool expanded_ = false;         // User list expanded state
    int userCount_ = 0;             // Current user count

    // Cached user list
    std::vector<ChannelRosterManager::ChannelUser> users_;
};

} // namespace voip::ui
