#include "ui/channel_widget.h"
#include <QMenu>
#include <QContextMenuEvent>

namespace voip::ui {

ChannelWidget::ChannelWidget(ChannelId id, const QString& name, QWidget* parent)
    : QWidget(parent), channel_id_(id), name_(name) {
    
    // Main layout
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(8);
    
    // Channel name
    nameLabel_ = new QLabel("# " + name_, this);
    nameLabel_->setMinimumWidth(100);
    QFont nameFont = nameLabel_->font();
    nameFont.setBold(true);
    nameLabel_->setFont(nameFont);
    mainLayout->addWidget(nameLabel_);
    
    // Spacer
    mainLayout->addStretch();
    
    // Listen button
    listenButton_ = new QPushButton("🎧 --", this);
    listenButton_->setFixedSize(80, 28);
    listenButton_->setCursor(Qt::PointingHandCursor);
    connect(listenButton_, &QPushButton::clicked, this, &ChannelWidget::onListenClicked);
    mainLayout->addWidget(listenButton_);
    
    // Transmit button
    transmitButton_ = new QPushButton("🎤 --", this);
    transmitButton_->setFixedSize(100, 28);
    transmitButton_->setCursor(Qt::PointingHandCursor);
    connect(transmitButton_, &QPushButton::clicked, this, &ChannelWidget::onTransmitClicked);
    mainLayout->addWidget(transmitButton_);
    
    // Hotkey button
    hotkeyButton_ = new QPushButton("[--]", this);
    hotkeyButton_->setFixedSize(50, 28);
    hotkeyButton_->setCursor(Qt::PointingHandCursor);
    connect(hotkeyButton_, &QPushButton::clicked, this, &ChannelWidget::onHotkeyClicked);
    mainLayout->addWidget(hotkeyButton_);
    
    // Initial style
    updateUI();
}

void ChannelWidget::setJoined(bool joined) {
    joined_ = joined;
    if (!joined) {
        listening_ = false;
        muted_ = false;
        hot_mic_ = false;
        ptt_active_ = false;
    }
    updateUI();
}

void ChannelWidget::setListening(bool listening) {
    listening_ = listening;
    if (listening) {
        joined_ = true;
    }
    updateUI();
}

void ChannelWidget::setMuted(bool muted) {
    muted_ = muted;
    updateUI();
}

void ChannelWidget::setHotMic(bool active) {
    hot_mic_ = active;
    updateUI();
}

void ChannelWidget::setPTTActive(bool active) {
    ptt_active_ = active;
    updateUI();
}

void ChannelWidget::setHotkey(const QKeySequence& key) {
    hotkey_ = key;
    if (key.isEmpty()) {
        hotkeyButton_->setText("[--]");
    } else {
        hotkeyButton_->setText("[" + key.toString() + "]");
    }
}

void ChannelWidget::onListenClicked() {
    if (!joined_) {
        // Join channel
        emit listenToggled(channel_id_, true);
    } else if (listening_ && !muted_) {
        // Mute channel
        emit muteToggled(channel_id_);
    } else if (muted_) {
        // Unmute channel
        emit muteToggled(channel_id_);
    } else {
        // Leave channel
        emit listenToggled(channel_id_, false);
    }
}

void ChannelWidget::onTransmitClicked() {
    emit transmitSelected(channel_id_);
}

void ChannelWidget::onHotkeyClicked() {
    emit hotkeyChangeRequested(channel_id_);
}

void ChannelWidget::updateUI() {
    applyStyles();
}

void ChannelWidget::applyStyles() {
    // Listen button styling
    if (!joined_) {
        listenButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: #2f3136;"
            "  color: #72767d;"
            "  border: 1px solid #202225;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #36393f;"
            "}"
        );
        listenButton_->setText("🎧 OFF");
    } else if (muted_) {
        listenButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: #4f545c;"
            "  color: #dcddde;"
            "  border: 1px solid #202225;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #5d6269;"
            "}"
        );
        listenButton_->setText("🔇 MUTE");
    } else if (listening_) {
        listenButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: #43b581;"
            "  color: white;"
            "  border: 1px solid #3ca374;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #3ca374;"
            "}"
        );
        listenButton_->setText("🎧 ON");
    }
    
    // Transmit button styling
    if (hot_mic_) {
        transmitButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: #5865f2;"
            "  color: white;"
            "  border: 1px solid #4752c4;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #4752c4;"
            "}"
        );
        transmitButton_->setText("🎤 HOT MIC");
    } else if (ptt_active_) {
        transmitButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: #faa61a;"
            "  color: white;"
            "  border: 1px solid #f57731;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f57731;"
            "}"
        );
        transmitButton_->setText("🎤 PTT");
    } else {
        transmitButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: #2f3136;"
            "  color: #72767d;"
            "  border: 1px solid #202225;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #36393f;"
            "}"
        );
        transmitButton_->setText("🎤 OFF");
    }
    
    // Hotkey button styling
    hotkeyButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #36393f;"
        "  color: #dcddde;"
        "  border: 1px solid #202225;"
        "  border-radius: 4px;"
        "  font-family: monospace;"
        "}"
        "QPushButton:hover {"
        "  background-color: #40444b;"
        "}"
    );
    
    // Name label color based on state
    if (joined_ && listening_) {
        nameLabel_->setStyleSheet("color: #dcddde;");
    } else {
        nameLabel_->setStyleSheet("color: #72767d;");
    }
}

} // namespace voip::ui
