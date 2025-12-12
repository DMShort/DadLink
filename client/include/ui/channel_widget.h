#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeySequence>
#include "common/types.h"

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
    explicit ChannelWidget(ChannelId id, const QString& name, QWidget* parent = nullptr);
    
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
    
private:
    void updateUI();
    void applyStyles();
    
    // Channel info
    ChannelId channel_id_;
    QString name_;
    
    // UI Elements
    QLabel* nameLabel_;
    QPushButton* listenButton_;
    QPushButton* transmitButton_;
    QPushButton* hotkeyButton_;
    QLabel* statusLabel_;
    
    // State
    bool joined_ = false;
    bool listening_ = false;
    bool muted_ = false;
    bool hot_mic_ = false;
    bool ptt_active_ = false;
    QKeySequence hotkey_;
};

} // namespace voip::ui
