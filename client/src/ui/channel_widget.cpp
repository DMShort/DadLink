#include "ui/channel_widget.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <iostream>

namespace voip::ui {

ChannelWidget::ChannelWidget(ChannelId id, const QString& name, bool isSubChannel, QWidget* parent)
    : QWidget(parent), channel_id_(id), name_(name) {

    // Main vertical layout
    mainLayout_ = new QVBoxLayout(this);
    // Add left indent for sub-channels
    int leftMargin = isSubChannel ? 32 : 8;
    mainLayout_->setContentsMargins(leftMargin, 4, 8, 4);
    mainLayout_->setSpacing(2);

    // Top controls horizontal layout
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);

    // Expand arrow button (Discord-style, visible)
    expandButton_ = new QPushButton("▶", this);
    expandButton_->setFixedSize(32, 32);
    expandButton_->setCursor(Qt::PointingHandCursor);
    expandButton_->setStyleSheet(
        "QPushButton { "
        "  background-color: #40444b; "  // Subtle dark background
        "  border: none; "
        "  border-radius: 4px; "
        "  color: #dcddde; "  // Light gray text
        "  font-size: 16px; "
        "  font-weight: bold; "
        "  padding: 4px; "
        "}"
        "QPushButton:hover { "
        "  background-color: #5865f2; "  // Discord blue on hover
        "  color: #ffffff; "
        "}"
    );
    connect(expandButton_, &QPushButton::clicked, this, &ChannelWidget::onExpandClicked);
    controlsLayout->addWidget(expandButton_);

    // Channel name
    nameLabel_ = new QLabel("# " + name_, this);
    nameLabel_->setMinimumWidth(100);
    QFont nameFont = nameLabel_->font();
    nameFont.setBold(true);
    nameLabel_->setFont(nameFont);
    controlsLayout->addWidget(nameLabel_);

    // User count badge (Discord-style)
    userCountLabel_ = new QLabel("", this);
    userCountLabel_->setStyleSheet(
        "color: #b9bbbe; "
        "font-size: 11px; "
        "font-weight: 600; "
        "padding: 2px 6px; "
        "background-color: #202225; "
        "border-radius: 8px;"
    );
    userCountLabel_->setVisible(false);  // Hidden when count is 0
    controlsLayout->addWidget(userCountLabel_);

    // Spacer
    controlsLayout->addStretch();

    // Listen button
    listenButton_ = new QPushButton("🎧 --", this);
    listenButton_->setFixedSize(80, 28);
    listenButton_->setCursor(Qt::PointingHandCursor);
    connect(listenButton_, &QPushButton::clicked, this, &ChannelWidget::onListenClicked);
    controlsLayout->addWidget(listenButton_);

    // Transmit button
    transmitButton_ = new QPushButton("🎤 --", this);
    transmitButton_->setFixedSize(100, 28);
    transmitButton_->setCursor(Qt::PointingHandCursor);
    connect(transmitButton_, &QPushButton::clicked, this, &ChannelWidget::onTransmitClicked);
    controlsLayout->addWidget(transmitButton_);

    // Hotkey button
    hotkeyButton_ = new QPushButton("[--]", this);
    hotkeyButton_->setFixedSize(50, 28);
    hotkeyButton_->setCursor(Qt::PointingHandCursor);
    connect(hotkeyButton_, &QPushButton::clicked, this, &ChannelWidget::onHotkeyClicked);
    controlsLayout->addWidget(hotkeyButton_);

    mainLayout_->addLayout(controlsLayout);

    // User list container (initially hidden)
    userListContainer_ = new QWidget(this);
    userListContainer_->setStyleSheet("background-color: #202225; border-radius: 4px;");
    userListLayout_ = new QVBoxLayout(userListContainer_);
    userListLayout_->setContentsMargins(4, 4, 4, 4);
    userListLayout_->setSpacing(2);
    userListContainer_->setVisible(false);

    mainLayout_->addWidget(userListContainer_);

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
    // Simple toggle: ON <-> OFF
    if (joined_ && listening_) {
        // Currently listening → leave channel
        emit listenToggled(channel_id_, false);
    } else {
        // Not listening → join channel
        emit listenToggled(channel_id_, true);
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

void ChannelWidget::setUserList(const std::vector<ChannelRosterManager::ChannelUser>& users) {
    users_ = users;
    updateUserListUI();
}

void ChannelWidget::setUserCount(int count) {
    userCount_ = count;

    if (count > 0) {
        userCountLabel_->setText(QString("%1").arg(count));
        userCountLabel_->setVisible(true);
    } else {
        userCountLabel_->setVisible(false);
    }
}

void ChannelWidget::setExpanded(bool expanded) {
    expanded_ = expanded;
    userListContainer_->setVisible(expanded);
    expandButton_->setText(expanded ? "▼" : "▶");
}

void ChannelWidget::onExpandClicked() {
    setExpanded(!expanded_);
}

void ChannelWidget::updateUserListUI() {
    // Clear existing items
    QLayoutItem* item;
    while ((item = userListLayout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Add user items
    const int MAX_DISPLAY = 20;  // Performance limit
    int displayCount = std::min(static_cast<int>(users_.size()), MAX_DISPLAY);

    for (int i = 0; i < displayCount; ++i) {
        auto* userWidget = createUserListItem(users_[i]);
        userListLayout_->addWidget(userWidget);
    }

    // Show "and X more..." if over limit
    if (users_.size() > MAX_DISPLAY) {
        auto* moreLabel = new QLabel(QString("   ...and %1 more").arg(users_.size() - MAX_DISPLAY), this);
        moreLabel->setStyleSheet("color: #72767d; font-size: 11px; padding: 2px;");
        userListLayout_->addWidget(moreLabel);
    }

    // Add spacer at bottom
    userListLayout_->addStretch();
}

QWidget* ChannelWidget::createUserListItem(const ChannelRosterManager::ChannelUser& user) {
    auto* widget = new QWidget(this);
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(20, 2, 8, 2);
    layout->setSpacing(6);

    // User icon + name
    QString displayName = "👤 " + user.username;
    auto* nameLabel = new QLabel(displayName, widget);
    nameLabel->setStyleSheet("color: #dcddde; font-size: 12px;");
    layout->addWidget(nameLabel);

    layout->addStretch();

    // State indicator
    QString stateIcon;
    QString stateColor = "#72767d";
    if (user.speaking) {
        stateIcon = "🎤";
        stateColor = "#faa61a";  // Orange for speaking
    } else if (user.listening) {
        stateIcon = "🎧";
        stateColor = "#43b581";  // Green for listening
    } else {
        stateIcon = "💤";
        stateColor = "#72767d";  // Gray for idle
    }

    auto* stateLabel = new QLabel(stateIcon, widget);
    stateLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(stateColor));
    layout->addWidget(stateLabel);

    // Hover effect
    widget->setStyleSheet(
        "QWidget:hover { background-color: #2f3136; border-radius: 2px; }"
    );

    return widget;
}

} // namespace voip::ui
