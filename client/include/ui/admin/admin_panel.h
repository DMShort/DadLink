#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <memory>

// Forward declarations
namespace voip::api {
    class AdminApiClient;
}

namespace voip::ui::admin {
class DashboardWidget;
class UserManager;
class ChannelManager;
class RoleManager;
class ChannelEditor;
class RoleEditor;
class MetricsViewer;
class AuditLogViewer;

/**
 * @brief Main admin panel container widget
 * 
 * This widget contains all admin functionality organized in tabs.
 * Only visible to users with admin permissions.
 */
class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(QWidget* parent = nullptr);
    ~AdminPanel() override;

    /**
     * @brief Set the user context for admin operations
     * @param user_id Current admin user ID
     * @param org_id Current organization ID
     * @param permissions User permission bitflags
     */
    void setUserContext(uint32_t user_id, uint32_t org_id, uint32_t permissions);
    
    /**
     * @brief Set API client for backend communication
     * @param client Shared API client instance
     */
    void setApiClient(std::shared_ptr<voip::api::AdminApiClient> client);
    
    /**
     * @brief Refresh all admin data
     */
    void refreshAll();
    
    /**
     * @brief Get the API client
     * @return Shared pointer to API client
     */
    std::shared_ptr<voip::api::AdminApiClient> getApiClient() const { return api_client_; }

    /**
     * @brief Get the channel manager
     * @return Pointer to channel manager widget
     */
    ChannelManager* getChannelManager() const { return channel_manager_; }

signals:
    void statusMessage(const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onRefreshClicked();
    void onTabChanged(int index);

private:
    void setupUi();
    void setupConnections();
    void updateTabData(int index);

private:
    // UI Components
    QTabWidget* sub_tabs_;
    QPushButton* refresh_btn_;
    QLabel* status_label_;
    
    // Admin widgets
    DashboardWidget* dashboard_;
    UserManager* user_manager_;
    ChannelManager* channel_manager_;
    RoleManager* role_manager_;
    QWidget* metrics_viewer_;
    QWidget* audit_log_viewer_;
    
    // API client
    std::shared_ptr<voip::api::AdminApiClient> api_client_;
    
    // Context
    uint32_t user_id_ = 0;
    uint32_t org_id_ = 0;
    uint32_t permissions_ = 0;
};

} // namespace voip::ui::admin