#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>

namespace voip::api {
    class AdminApiClient;
}

namespace voip::ui::admin {

/**
 * @brief Widget for managing channels with tree view
 */
class ChannelManager : public QWidget {
    Q_OBJECT

public:
    explicit ChannelManager(QWidget* parent = nullptr);
    
    void setApiClient(std::shared_ptr<voip::api::AdminApiClient> client);
    void refresh();

signals:
    void statusMessage(const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onCreateChannel();
    void onEditChannel();
    void onDeleteChannel();
    void onRefreshClicked();
    void onSelectionChanged();
    void onChannelDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();
    void loadChannels();
    void onChannelsLoaded(const QJsonArray& channels);
    void populateChannelTree(const QJsonArray& channels);
    void buildChannelTree(const QJsonArray& channels);
    void updateButtonStates();
    int getSelectedChannelId() const;
    
    // UI Components
    QTreeWidget* channel_tree_;
    QLineEdit* search_box_;
    QPushButton* create_btn_;
    QPushButton* edit_btn_;
    QPushButton* delete_btn_;
    QPushButton* refresh_btn_;
    
    // Data
    std::shared_ptr<voip::api::AdminApiClient> apiClient_;
    QJsonArray all_channels_;
};

} // namespace voip::ui::admin
