#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

namespace voip::api {
    class AdminApiClient;
}

namespace voip::ui::admin {

/**
 * @brief Dashboard widget showing system overview and statistics
 */
class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    void setApiClient(std::shared_ptr<voip::api::AdminApiClient> client);
    void refresh();

signals:
    void errorOccurred(const QString& error);

private slots:
    void onAutoRefreshTimer();
    void onStatsReceived(const QJsonObject& stats);

private:
    void setupUi();
    void createStatCard(const QString& title, QLabel** value_label, 
                       QLabel** change_label, QGridLayout* grid, int row);
    void updateSystemStats(const QJsonObject& stats);
    void updateUserStats(const QJsonObject& stats);

private:
    std::shared_ptr<voip::api::AdminApiClient> api_client_;
    
    // Stat labels
    QLabel* server_status_;
    QLabel* total_users_value_;
    QLabel* total_users_change_;
    QLabel* active_users_value_;
    QLabel* active_users_change_;
    QLabel* total_channels_value_;
    QLabel* active_sessions_value_;
    QLabel* active_sessions_change_;
    QLabel* avg_latency_value_;
    QLabel* cpu_usage_value_;
    QLabel* memory_usage_value_;
    
    // Auto-refresh timer
    QTimer* refresh_timer_;
    
    // Previous values for change calculation
    int prev_total_users_ = 0;
    int prev_active_sessions_ = 0;
};

} // namespace voip::ui::admin
