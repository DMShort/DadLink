#include "ui/admin/user_manager.h"
#include "ui/admin/create_user_dialog.h"
#include "api/admin_api_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <iostream>

namespace voip::ui::admin {

UserManager::UserManager(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    std::cout << "👥 UserManager created" << std::endl;
}

void UserManager::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);
    
    // Search and filter bar
    auto* search_layout = new QHBoxLayout();
    
    search_box_ = new QLineEdit(this);
    search_box_->setPlaceholderText("🔍 Search users...");
    connect(search_box_, &QLineEdit::textChanged, this, &UserManager::onSearchChanged);
    search_layout->addWidget(search_box_, 2);
    
    filter_combo_ = new QComboBox(this);
    filter_combo_->addItem("All Users");
    filter_combo_->addItem("Active");
    filter_combo_->addItem("Inactive");
    filter_combo_->addItem("Banned");
    connect(filter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &UserManager::onFilterChanged);
    search_layout->addWidget(filter_combo_, 1);
    
    layout->addLayout(search_layout);
    
    // User table
    user_table_ = new QTableWidget(this);
    user_table_->setColumnCount(6);
    user_table_->setHorizontalHeaderLabels({"ID", "Username", "Email", "Organization", "Status", "Last Login"});
    user_table_->horizontalHeader()->setStretchLastSection(true);
    user_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    user_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    user_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    user_table_->setSortingEnabled(true);
    
    connect(user_table_, &QTableWidget::cellDoubleClicked, 
            this, &UserManager::onUserDoubleClicked);
    connect(user_table_, &QTableWidget::itemSelectionChanged,
            this, &UserManager::onSelectionChanged);
    
    layout->addWidget(user_table_);
    
    // Button bar
    auto* button_layout = new QHBoxLayout();
    
    create_btn_ = new QPushButton("➕ Create User", this);
    connect(create_btn_, &QPushButton::clicked, this, &UserManager::onCreateUser);
    button_layout->addWidget(create_btn_);
    
    edit_btn_ = new QPushButton("✏️ Edit", this);
    edit_btn_->setEnabled(false);
    connect(edit_btn_, &QPushButton::clicked, this, &UserManager::onEditUser);
    button_layout->addWidget(edit_btn_);
    
    delete_btn_ = new QPushButton("🗑️ Delete", this);
    delete_btn_->setEnabled(false);
    connect(delete_btn_, &QPushButton::clicked, this, &UserManager::onDeleteUser);
    button_layout->addWidget(delete_btn_);
    
    ban_btn_ = new QPushButton("🚫 Ban", this);
    ban_btn_->setEnabled(false);
    connect(ban_btn_, &QPushButton::clicked, this, &UserManager::onBanUser);
    button_layout->addWidget(ban_btn_);
    
    unban_btn_ = new QPushButton("✅ Unban", this);
    unban_btn_->setEnabled(false);
    connect(unban_btn_, &QPushButton::clicked, this, &UserManager::onUnbanUser);
    button_layout->addWidget(unban_btn_);
    
    button_layout->addStretch();
    
    refresh_btn_ = new QPushButton("🔄 Refresh", this);
    connect(refresh_btn_, &QPushButton::clicked, this, &UserManager::onRefreshClicked);
    button_layout->addWidget(refresh_btn_);
    
    layout->addLayout(button_layout);
}

void UserManager::setApiClient(std::shared_ptr<voip::api::AdminApiClient> client) {
    apiClient_ = client;
    refresh();
}

void UserManager::refresh() {
    if (!apiClient_) {
        std::cerr << "❌ No API client set for UserManager" << std::endl;
        return;
    }
    
    std::cout << "🔄 Refreshing user list..." << std::endl;
    loadUsers();
}

void UserManager::loadUsers() {
    apiClient_->getUsers([this](const QJsonArray& users) {
        onUsersLoaded(users);
    });
}

void UserManager::onUsersLoaded(const QJsonArray& users) {
    std::cout << "👥 Received " << users.size() << " users" << std::endl;
    
    all_users_ = users;
    filterUsers();
    emit statusMessage(QString("Loaded %1 users").arg(users.size()));
}

void UserManager::populateUserTable(const QJsonArray& users) {
    user_table_->setSortingEnabled(false);
    user_table_->setRowCount(0);
    
    for (const QJsonValue& value : users) {
        QJsonObject user = value.toObject();
        
        int row = user_table_->rowCount();
        user_table_->insertRow(row);
        
        user_table_->setItem(row, 0, new QTableWidgetItem(QString::number(user["id"].toInt())));
        user_table_->setItem(row, 1, new QTableWidgetItem(user["username"].toString()));
        user_table_->setItem(row, 2, new QTableWidgetItem(user["email"].toString("N/A")));
        user_table_->setItem(row, 3, new QTableWidgetItem(QString::number(user["org_id"].toInt())));
        
        QString status = user["is_active"].toBool() ? "Active" : "Inactive";
        if (user["is_banned"].toBool()) {
            status = "Banned";
        }
        user_table_->setItem(row, 4, new QTableWidgetItem(status));
        
        user_table_->setItem(row, 5, new QTableWidgetItem(user["last_login"].toString("Never")));
    }
    
    user_table_->setSortingEnabled(true);
    user_table_->resizeColumnsToContents();
}

void UserManager::filterUsers() {
    QString search_text = search_box_->text().toLower();
    int filter_index = filter_combo_->currentIndex();
    
    QJsonArray filtered;
    
    for (const QJsonValue& value : all_users_) {
        QJsonObject user = value.toObject();
        
        // Apply search filter
        if (!search_text.isEmpty()) {
            QString username = user["username"].toString().toLower();
            QString email = user["email"].toString().toLower();
            if (!username.contains(search_text) && !email.contains(search_text)) {
                continue;
            }
        }
        
        // Apply status filter
        if (filter_index == 1 && !user["is_active"].toBool()) continue;  // Active
        if (filter_index == 2 && user["is_active"].toBool()) continue;   // Inactive
        if (filter_index == 3 && !user["is_banned"].toBool()) continue;  // Banned
        
        filtered.append(user);
    }
    
    filtered_users_ = filtered;
    populateUserTable(filtered);
}

void UserManager::onSearchChanged(const QString& text) {
    filterUsers();
}

void UserManager::onFilterChanged(int index) {
    filterUsers();
}

void UserManager::onSelectionChanged() {
    updateButtonStates();
}

void UserManager::updateButtonStates() {
    bool has_selection = user_table_->currentRow() >= 0;
    
    edit_btn_->setEnabled(has_selection);
    delete_btn_->setEnabled(has_selection);
    ban_btn_->setEnabled(has_selection);
    unban_btn_->setEnabled(has_selection);
}

int UserManager::getSelectedUserId() const {
    int row = user_table_->currentRow();
    if (row < 0) return -1;
    
    QTableWidgetItem* id_item = user_table_->item(row, 0);
    if (!id_item) return -1;
    
    return id_item->text().toInt();
}

void UserManager::onCreateUser() {
    if (!apiClient_) {
        emit errorOccurred("No API client available");
        return;
    }
    
    CreateUserDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject user_data = dialog.getUserData();
        
        std::cout << "Creating user: " << user_data["username"].toString().toStdString() << std::endl;
        
        // Call API to create user
        apiClient_->createUser(user_data, [this, user_data](int user_id) {
            if (user_id > 0) {
                emit statusMessage(QString("User '%1' created successfully (ID: %2)")
                                 .arg(user_data["username"].toString())
                                 .arg(user_id));
                refresh();
            } else {
                emit errorOccurred("Failed to create user");
            }
        });
    }
}

void UserManager::onEditUser() {
    int user_id = getSelectedUserId();
    if (user_id >= 0) {
        showUserDialog(user_id);
    }
}

void UserManager::showUserDialog(int user_id) {
    // TODO: Implement user edit dialog
    if (user_id < 0) {
        emit statusMessage("User creation dialog - Coming Soon");
    } else {
        emit statusMessage(QString("Edit user %1 dialog - Coming Soon").arg(user_id));
    }
}

void UserManager::onDeleteUser() {
    int user_id = getSelectedUserId();
    if (user_id < 0) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete User", 
        QString("Are you sure you want to delete user ID %1?").arg(user_id),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        apiClient_->deleteUser(user_id, [this, user_id](bool success) {
            if (success) {
                emit statusMessage(QString("User %1 deleted successfully").arg(user_id));
                refresh();
            } else {
                emit errorOccurred(QString("Failed to delete user %1").arg(user_id));
            }
        });
    }
}

void UserManager::onBanUser() {
    int user_id = getSelectedUserId();
    if (user_id < 0) return;
    
    apiClient_->banUser(user_id, [this, user_id](bool success) {
        if (success) {
            emit statusMessage(QString("User %1 banned successfully").arg(user_id));
            refresh();
        } else {
            emit errorOccurred(QString("Failed to ban user %1").arg(user_id));
        }
    });
}

void UserManager::onUnbanUser() {
    int user_id = getSelectedUserId();
    if (user_id < 0) return;
    
    apiClient_->unbanUser(user_id, [this, user_id](bool success) {
        if (success) {
            emit statusMessage(QString("User %1 unbanned successfully").arg(user_id));
            refresh();
        } else {
            emit errorOccurred(QString("Failed to unban user %1").arg(user_id));
        }
    });
}

void UserManager::onResetPassword() {
    int user_id = getSelectedUserId();
    if (user_id < 0) return;
    
    bool ok;
    QString new_password = QInputDialog::getText(
        this, "Reset Password",
        "Enter new password:", QLineEdit::Password,
        "", &ok
    );
    
    if (ok && !new_password.isEmpty()) {
        apiClient_->resetPassword(user_id, new_password, [this, user_id](bool success) {
            if (success) {
                emit statusMessage(QString("Password reset for user %1").arg(user_id));
            } else {
                emit errorOccurred(QString("Failed to reset password for user %1").arg(user_id));
            }
        });
    }
}

void UserManager::onUserDoubleClicked(int row, int column) {
    onEditUser();
}

void UserManager::onRefreshClicked() {
    refresh();
}

void UserManager::onUserCreated(int user_id) {
    emit statusMessage(QString("User %1 created successfully").arg(user_id));
    refresh();
}

void UserManager::onUserUpdated() {
    emit statusMessage("User updated successfully");
    refresh();
}

void UserManager::onUserDeleted() {
    emit statusMessage("User deleted successfully");
    refresh();
}

} // namespace voip::ui::admin
