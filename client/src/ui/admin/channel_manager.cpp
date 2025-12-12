#include "ui/admin/channel_manager.h"
#include "api/admin_api_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <iostream>

namespace voip::ui::admin {

ChannelManager::ChannelManager(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    std::cout << "🎙️ ChannelManager created" << std::endl;
}

void ChannelManager::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);
    
    // Search bar
    auto* search_layout = new QHBoxLayout();
    
    search_box_ = new QLineEdit(this);
    search_box_->setPlaceholderText("🔍 Search channels...");
    search_layout->addWidget(search_box_);
    
    layout->addLayout(search_layout);
    
    // Channel tree
    channel_tree_ = new QTreeWidget(this);
    channel_tree_->setHeaderLabels({"Channel Name", "Description", "Users", "Max Users"});
    channel_tree_->setColumnWidth(0, 250);
    channel_tree_->setColumnWidth(1, 350);
    channel_tree_->setColumnWidth(2, 80);
    channel_tree_->setColumnWidth(3, 100);
    channel_tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    channel_tree_->setAlternatingRowColors(true);
    
    connect(channel_tree_, &QTreeWidget::itemSelectionChanged,
            this, &ChannelManager::onSelectionChanged);
    connect(channel_tree_, &QTreeWidget::itemDoubleClicked,
            this, &ChannelManager::onChannelDoubleClicked);
    
    layout->addWidget(channel_tree_);
    
    // Button bar
    auto* button_layout = new QHBoxLayout();
    
    create_btn_ = new QPushButton("➕ Create Channel", this);
    connect(create_btn_, &QPushButton::clicked, this, &ChannelManager::onCreateChannel);
    button_layout->addWidget(create_btn_);
    
    edit_btn_ = new QPushButton("✏️ Edit", this);
    edit_btn_->setEnabled(false);
    connect(edit_btn_, &QPushButton::clicked, this, &ChannelManager::onEditChannel);
    button_layout->addWidget(edit_btn_);
    
    delete_btn_ = new QPushButton("🗑️ Delete", this);
    delete_btn_->setEnabled(false);
    connect(delete_btn_, &QPushButton::clicked, this, &ChannelManager::onDeleteChannel);
    button_layout->addWidget(delete_btn_);
    
    button_layout->addStretch();
    
    refresh_btn_ = new QPushButton("🔄 Refresh", this);
    connect(refresh_btn_, &QPushButton::clicked, this, &ChannelManager::onRefreshClicked);
    button_layout->addWidget(refresh_btn_);
    
    layout->addLayout(button_layout);
}

void ChannelManager::setApiClient(std::shared_ptr<voip::api::AdminApiClient> client) {
    apiClient_ = client;
    refresh();
}

void ChannelManager::refresh() {
    if (!apiClient_) {
        std::cerr << "❌ No API client set for ChannelManager" << std::endl;
        return;
    }
    
    std::cout << "🔄 Refreshing channel list..." << std::endl;
    loadChannels();
}

void ChannelManager::loadChannels() {
    if (!apiClient_) {
        std::cerr << "❌ No API client!" << std::endl;
        return;
    }
    
    std::cout << "📡 Loading channels from API..." << std::endl;
    
    apiClient_->getChannels([this](const QJsonArray& channels) {
        onChannelsLoaded(channels);
    });
}

void ChannelManager::onChannelsLoaded(const QJsonArray& channels) {
    std::cout << "🎙️ Received " << channels.size() << " channels" << std::endl;
    
    all_channels_ = channels;
    buildChannelTree(channels);
    emit statusMessage(QString("Loaded %1 channels").arg(channels.size()));
}

void ChannelManager::buildChannelTree(const QJsonArray& channels) {
    channel_tree_->clear();
    
    // Build hierarchical tree structure
    QMap<int, QTreeWidgetItem*> channel_items;
    
    // First pass: create all items
    for (const QJsonValue& value : channels) {
        QJsonObject channel = value.toObject();
        
        auto* item = new QTreeWidgetItem();
        item->setText(0, channel["name"].toString());
        item->setText(1, channel["description"].toString());
        item->setText(2, QString::number(channel["current_users"].toInt()));
        item->setText(3, QString::number(channel["max_users"].toInt()));
        item->setData(0, Qt::UserRole, channel["id"].toInt());
        
        channel_items[channel["id"].toInt()] = item;
    }
    
    // Second pass: build hierarchy
    for (const QJsonValue& value : channels) {
        QJsonObject channel = value.toObject();
        int channel_id = channel["id"].toInt();
        int parent_id = channel["parent_id"].toInt(-1);
        
        QTreeWidgetItem* item = channel_items[channel_id];
        
        if (parent_id > 0 && channel_items.contains(parent_id)) {
            // Add as child to parent
            channel_items[parent_id]->addChild(item);
        } else {
            // Add as top-level item
            channel_tree_->addTopLevelItem(item);
        }
    }
    
    channel_tree_->expandAll();
}

void ChannelManager::populateChannelTree(const QJsonArray& channels) {
    buildChannelTree(channels);
}

void ChannelManager::onCreateChannel() {
    // TODO: Show create channel dialog
    emit statusMessage("Create channel dialog - Coming Soon");
}

void ChannelManager::onEditChannel() {
    // TODO: Show edit channel dialog
    int channel_id = getSelectedChannelId();
    if (channel_id >= 0) {
        emit statusMessage(QString("Edit channel %1 dialog - Coming Soon").arg(channel_id));
    }
}

void ChannelManager::onDeleteChannel() {
    int channel_id = getSelectedChannelId();
    if (channel_id < 0) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Channel",
        QString("Are you sure you want to delete channel ID %1?").arg(channel_id),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes && apiClient_) {
        apiClient_->deleteChannel(channel_id, [this, channel_id](bool success) {
            if (success) {
                emit statusMessage(QString("Channel %1 deleted").arg(channel_id));
                refresh();
            } else {
                emit errorOccurred(QString("Failed to delete channel %1").arg(channel_id));
            }
        });
    }
}

void ChannelManager::onRefreshClicked() {
    refresh();
}

void ChannelManager::onSelectionChanged() {
    updateButtonStates();
}

void ChannelManager::updateButtonStates() {
    bool has_selection = channel_tree_->currentItem() != nullptr;
    
    edit_btn_->setEnabled(has_selection);
    delete_btn_->setEnabled(has_selection);
}

int ChannelManager::getSelectedChannelId() const {
    QTreeWidgetItem* item = channel_tree_->currentItem();
    if (!item) return -1;
    
    return item->data(0, Qt::UserRole).toInt();
}

void ChannelManager::onChannelDoubleClicked(QTreeWidgetItem* item, int column) {
    onEditChannel();
}

} // namespace voip::ui::admin
