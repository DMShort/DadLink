#include "ui/admin/admin_panel.h"
#include "ui/admin/dashboard_widget.h"
#include "ui/admin/user_manager.h"
#include "ui/admin/channel_manager.h"
#include "ui/admin/role_manager.h"
#include "api/admin_api_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <iostream>

namespace voip::ui::admin {

AdminPanel::AdminPanel(QWidget* parent)
    : QWidget(parent)
    , api_client_(std::make_shared<voip::api::AdminApiClient>(this))
{
    setupUi();
    setupConnections();
    
    std::cout << "🔧 AdminPanel created" << std::endl;
}

AdminPanel::~AdminPanel() {
    std::cout << "🔧 AdminPanel destroyed" << std::endl;
}

void AdminPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Toolbar
    auto* toolbar_widget = new QWidget(this);
    auto* toolbar_layout = new QHBoxLayout(toolbar_widget);
    toolbar_layout->setContentsMargins(10, 5, 10, 5);
    
    auto* title_label = new QLabel("<h3>🔧 Admin Panel</h3>", this);
    toolbar_layout->addWidget(title_label);
    toolbar_layout->addStretch();
    
    refresh_btn_ = new QPushButton("🔄 Refresh All", this);
    toolbar_layout->addWidget(refresh_btn_);
    
    status_label_ = new QLabel("Ready", this);
    status_label_->setStyleSheet("QLabel { color: #888; padding: 0 10px; }");
    toolbar_layout->addWidget(status_label_);
    
    layout->addWidget(toolbar_widget);
    
    // Add horizontal line
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
    
    // Sub-tabs
    sub_tabs_ = new QTabWidget(this);
    sub_tabs_->setTabPosition(QTabWidget::North);
    
    // Create widgets
    dashboard_ = new DashboardWidget(this);
    user_manager_ = new UserManager(this);
    channel_manager_ = new ChannelManager(this);
    role_manager_ = new RoleManager(this);
    
    // Note: Other widgets are not yet implemented, so we'll add placeholder
    metrics_viewer_ = new QWidget(this); // Placeholder
    audit_log_viewer_ = new QWidget(this); // Placeholder
    
    auto* metrics_label = new QLabel("<center><h3>Metrics Viewer<br/><i>Coming Soon</i></h3></center>", 
                                     metrics_viewer_);
    auto* metrics_layout = new QVBoxLayout(metrics_viewer_);
    metrics_layout->addWidget(metrics_label);
    
    auto* audit_label = new QLabel("<center><h3>Audit Log Viewer<br/><i>Coming Soon</i></h3></center>", 
                                   audit_log_viewer_);
    auto* audit_layout = new QVBoxLayout(audit_log_viewer_);
    audit_layout->addWidget(audit_label);
    
    // Add tabs
    sub_tabs_->addTab(dashboard_, "📊 Dashboard");
    sub_tabs_->addTab(user_manager_, "👥 Users");
    sub_tabs_->addTab(channel_manager_, "🎙️ Channels");
    sub_tabs_->addTab(role_manager_, "🔑 Roles");
    sub_tabs_->addTab(metrics_viewer_, "📈 Metrics");
    sub_tabs_->addTab(audit_log_viewer_, "📋 Audit Log");
    
    layout->addWidget(sub_tabs_);
    
    // Set API client for widgets
    dashboard_->setApiClient(api_client_);
    user_manager_->setApiClient(api_client_);
    channel_manager_->setApiClient(api_client_);
    role_manager_->setApiClient(api_client_);
}

void AdminPanel::setupConnections() {
    connect(refresh_btn_, &QPushButton::clicked, this, &AdminPanel::onRefreshClicked);
    connect(sub_tabs_, &QTabWidget::currentChanged, this, &AdminPanel::onTabChanged);
    
    // Connect status messages
    connect(user_manager_, &UserManager::statusMessage, 
            [this](const QString& msg) {
                status_label_->setText(msg);
                status_label_->setStyleSheet("QLabel { color: #00ff00; padding: 0 10px; }");
            });
    
    connect(user_manager_, &UserManager::errorOccurred, 
            [this](const QString& error) {
                status_label_->setText("Error: " + error);
                status_label_->setStyleSheet("QLabel { color: #ff0000; padding: 0 10px; }");
            });
    
    // Connect channel manager signals
    connect(channel_manager_, &ChannelManager::statusMessage,
            [this](const QString& msg) {
                status_label_->setText(msg);
                status_label_->setStyleSheet("QLabel { color: #00ff00; padding: 0 10px; }");
            });
    
    connect(channel_manager_, &ChannelManager::errorOccurred,
            [this](const QString& error) {
                status_label_->setText("Error: " + error);
                status_label_->setStyleSheet("QLabel { color: #ff0000; padding: 0 10px; }");
            });
    
    // Connect role manager signals
    connect(role_manager_, &RoleManager::statusMessage,
            [this](const QString& msg) {
                status_label_->setText(msg);
                status_label_->setStyleSheet("QLabel { color: #00ff00; padding: 0 10px; }");
            });
    
    connect(role_manager_, &RoleManager::errorOccurred,
            [this](const QString& error) {
                status_label_->setText("Error: " + error);
                status_label_->setStyleSheet("QLabel { color: #ff0000; padding: 0 10px; }");
            });
    
    // Connect API client signals
    connect(api_client_.get(), &voip::api::AdminApiClient::requestStarted,
            [this]() {
                status_label_->setText("Loading...");
                status_label_->setStyleSheet("QLabel { color: #ffaa00; padding: 0 10px; }");
            });
    
    connect(api_client_.get(), &voip::api::AdminApiClient::requestFinished,
            [this]() {
                status_label_->setText("Ready");
                status_label_->setStyleSheet("QLabel { color: #888; padding: 0 10px; }");
            });
    
    connect(api_client_.get(), &voip::api::AdminApiClient::error,
            [this](const QString& error, int status_code) {
                QString msg = QString("API Error [%1]: %2").arg(status_code).arg(error);
                status_label_->setText(msg);
                status_label_->setStyleSheet("QLabel { color: #ff0000; padding: 0 10px; }");
                emit errorOccurred(msg);
            });
}

void AdminPanel::setUserContext(uint32_t user_id, uint32_t org_id, uint32_t permissions) {
    user_id_ = user_id;
    org_id_ = org_id;
    permissions_ = permissions;
    
    std::cout << "🔧 Admin context set: user=" << user_id 
              << " org=" << org_id 
              << " perms=0x" << std::hex << permissions << std::dec << std::endl;
    
    // Initial refresh
    refreshAll();
}

void AdminPanel::setApiClient(std::shared_ptr<voip::api::AdminApiClient> client) {
    api_client_ = client;
    dashboard_->setApiClient(client);
    user_manager_->setApiClient(client);
    channel_manager_->setApiClient(client);
    role_manager_->setApiClient(client);
}

void AdminPanel::refreshAll() {
    std::cout << "🔄 Refreshing all admin data..." << std::endl;
    
    int current_tab = sub_tabs_->currentIndex();
    updateTabData(current_tab);
    
    emit statusMessage("Refreshed all data");
}

void AdminPanel::onRefreshClicked() {
    refreshAll();
}

void AdminPanel::onTabChanged(int index) {
    updateTabData(index);
}

void AdminPanel::updateTabData(int index) {
    switch (index) {
        case 0: // Dashboard
            dashboard_->refresh();
            break;
        case 1: // Users
            user_manager_->refresh();
            break;
        case 2: // Channels
            channel_manager_->refresh();
            break;
        case 3: // Roles
            role_manager_->refresh();
            break;
        case 4: // Metrics
            // metrics_viewer_->refresh();
            break;
        case 5: // Audit Log
            // audit_log_viewer_->refresh();
            break;
    }
}

} // namespace voip::ui::admin
