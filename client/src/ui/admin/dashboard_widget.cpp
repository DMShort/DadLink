#include "ui/admin/dashboard_widget.h"
#include "api/admin_api_client.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFrame>
#include <iostream>

namespace voip::ui::admin {

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
    , refresh_timer_(new QTimer(this))
{
    setupUi();
    
    // Auto-refresh every 5 seconds
    connect(refresh_timer_, &QTimer::timeout, this, &DashboardWidget::onAutoRefreshTimer);
    refresh_timer_->start(5000);
    
    std::cout << "📊 Dashboard widget created" << std::endl;
}

void DashboardWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Title
    auto* title = new QLabel("<h2>📊 System Dashboard</h2>", this);
    layout->addWidget(title);
    
    // System Status Card
    auto* status_group = new QGroupBox("System Status", this);
    auto* status_layout = new QHBoxLayout(status_group);
    
    server_status_ = new QLabel("● <b>Online</b>", this);
    server_status_->setStyleSheet("QLabel { color: #00ff00; }");
    QFont status_font = server_status_->font();
    status_font.setPointSize(14);
    server_status_->setFont(status_font);
    status_layout->addWidget(server_status_);
    status_layout->addStretch();
    
    layout->addWidget(status_group);
    
    // Stats Grid
    auto* stats_group = new QGroupBox("Quick Statistics", this);
    auto* stats_grid = new QGridLayout(stats_group);
    stats_grid->setSpacing(15);
    
    // Create stat cards
    createStatCard("Total Users", &total_users_value_, &total_users_change_, stats_grid, 0);
    createStatCard("Active Users", &active_users_value_, &active_users_change_, stats_grid, 1);
    createStatCard("Total Channels", &total_channels_value_, nullptr, stats_grid, 2);
    createStatCard("Active Sessions", &active_sessions_value_, &active_sessions_change_, stats_grid, 3);
    createStatCard("Avg Latency", &avg_latency_value_, nullptr, stats_grid, 4);
    createStatCard("CPU Usage", &cpu_usage_value_, nullptr, stats_grid, 5);
    createStatCard("Memory Usage", &memory_usage_value_, nullptr, stats_grid, 6);
    
    layout->addWidget(stats_group);
    
    // Refresh button
    auto* refresh_btn = new QPushButton("🔄 Refresh Now", this);
    connect(refresh_btn, &QPushButton::clicked, this, &DashboardWidget::refresh);
    layout->addWidget(refresh_btn);
    
    layout->addStretch();
}

void DashboardWidget::createStatCard(const QString& title, QLabel** value_label,
                                      QLabel** change_label, QGridLayout* grid, int row) {
    // Card frame
    auto* card = new QFrame(this);
    card->setFrameStyle(QFrame::Box | QFrame::Plain);
    card->setLineWidth(1);
    
    auto* card_layout = new QVBoxLayout(card);
    card_layout->setContentsMargins(15, 10, 15, 10);
    
    // Title
    auto* title_label = new QLabel(title, this);
    QFont title_font = title_label->font();
    title_font.setPointSize(9);
    title_font.setBold(false);
    title_label->setFont(title_font);
    title_label->setStyleSheet("QLabel { color: #888; }");
    card_layout->addWidget(title_label);
    
    // Value
    *value_label = new QLabel("--", this);
    QFont value_font = (*value_label)->font();
    value_font.setPointSize(24);
    value_font.setBold(true);
    (*value_label)->setFont(value_font);
    card_layout->addWidget(*value_label);
    
    // Change indicator (if requested)
    if (change_label) {
        *change_label = new QLabel("", this);
        QFont change_font = (*change_label)->font();
        change_font.setPointSize(9);
        (*change_label)->setFont(change_font);
        card_layout->addWidget(*change_label);
    }
    
    grid->addWidget(card, row / 3, row % 3);
}

void DashboardWidget::setApiClient(std::shared_ptr<voip::api::AdminApiClient> client) {
    api_client_ = client;
    refresh();
}

void DashboardWidget::refresh() {
    if (!api_client_) {
        std::cerr << "❌ No API client set for dashboard" << std::endl;
        return;
    }
    
    std::cout << "🔄 Refreshing dashboard..." << std::endl;
    
    // Get system metrics
    api_client_->getSystemMetrics([this](const QJsonObject& metrics) {
        onStatsReceived(metrics);
    });
}

void DashboardWidget::onAutoRefreshTimer() {
    refresh();
}

void DashboardWidget::onStatsReceived(const QJsonObject& stats) {
    std::cout << "📊 Received system stats" << std::endl;
    
    // Update system stats
    if (stats.contains("server_status")) {
        QString status = stats["server_status"].toString();
        if (status == "online") {
            server_status_->setText("● <b>Online</b>");
            server_status_->setStyleSheet("QLabel { color: #00ff00; }");
        } else {
            server_status_->setText("● <b>Offline</b>");
            server_status_->setStyleSheet("QLabel { color: #ff0000; }");
        }
    }
    
    // Update user stats
    int total_users = stats["total_users"].toInt(0);
    int active_users = stats["active_users"].toInt(0);
    int total_channels = stats["total_channels"].toInt(0);
    int active_sessions = stats["active_sessions"].toInt(0);
    
    total_users_value_->setText(QString::number(total_users));
    active_users_value_->setText(QString::number(active_users));
    total_channels_value_->setText(QString::number(total_channels));
    active_sessions_value_->setText(QString::number(active_sessions));
    
    // Calculate changes
    if (prev_total_users_ > 0) {
        int change = total_users - prev_total_users_;
        if (change > 0) {
            total_users_change_->setText(QString("↑ +%1").arg(change));
            total_users_change_->setStyleSheet("QLabel { color: #00ff00; }");
        } else if (change < 0) {
            total_users_change_->setText(QString("↓ %1").arg(change));
            total_users_change_->setStyleSheet("QLabel { color: #ff0000; }");
        } else {
            total_users_change_->setText("—");
            total_users_change_->setStyleSheet("QLabel { color: #888; }");
        }
    }
    
    if (prev_active_sessions_ > 0) {
        int change = active_sessions - prev_active_sessions_;
        if (change > 0) {
            active_sessions_change_->setText(QString("↑ +%1").arg(change));
            active_sessions_change_->setStyleSheet("QLabel { color: #00ff00; }");
        } else if (change < 0) {
            active_sessions_change_->setText(QString("↓ %1").arg(change));
            active_sessions_change_->setStyleSheet("QLabel { color: #ff0000; }");
        } else {
            active_sessions_change_->setText("—");
            active_sessions_change_->setStyleSheet("QLabel { color: #888; }");
        }
    }
    
    // Update performance metrics
    if (stats.contains("avg_latency_ms")) {
        int latency = stats["avg_latency_ms"].toInt(0);
        avg_latency_value_->setText(QString("%1 ms").arg(latency));
    }
    
    if (stats.contains("cpu_usage_percent")) {
        double cpu = stats["cpu_usage_percent"].toDouble(0);
        cpu_usage_value_->setText(QString("%1%").arg(cpu, 0, 'f', 1));
    }
    
    if (stats.contains("memory_usage_mb")) {
        int memory = stats["memory_usage_mb"].toInt(0);
        memory_usage_value_->setText(QString("%1 MB").arg(memory));
    }
    
    // Save for next comparison
    prev_total_users_ = total_users;
    prev_active_sessions_ = active_sessions;
}

} // namespace voip::ui::admin
