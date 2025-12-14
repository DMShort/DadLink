#include "ui/admin/role_manager.h"
#include "ui/admin/create_role_dialog.h"
#include "ui/admin/edit_role_dialog.h"
#include "api/admin_api_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <iostream>

namespace voip::ui::admin {

RoleManager::RoleManager(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    std::cout << "👑 RoleManager created" << std::endl;
}

void RoleManager::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);
    
    // Search bar
    auto* search_layout = new QHBoxLayout();
    
    search_box_ = new QLineEdit(this);
    search_box_->setPlaceholderText("🔍 Search roles...");
    search_layout->addWidget(search_box_);
    
    layout->addLayout(search_layout);
    
    // Role table
    role_table_ = new QTableWidget(this);
    role_table_->setColumnCount(5);
    role_table_->setHorizontalHeaderLabels({"ID", "Name", "Permissions", "Priority", "Organization"});
    role_table_->horizontalHeader()->setStretchLastSection(true);
    role_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    role_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    role_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    role_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    role_table_->setSortingEnabled(true);
    role_table_->setAlternatingRowColors(true);
    
    connect(role_table_, &QTableWidget::cellDoubleClicked,
            this, &RoleManager::onRoleDoubleClicked);
    connect(role_table_, &QTableWidget::itemSelectionChanged,
            this, &RoleManager::onSelectionChanged);
    
    layout->addWidget(role_table_);
    
    // Button bar
    auto* button_layout = new QHBoxLayout();
    
    create_btn_ = new QPushButton("➕ Create Role", this);
    connect(create_btn_, &QPushButton::clicked, this, &RoleManager::onCreateRole);
    button_layout->addWidget(create_btn_);
    
    edit_btn_ = new QPushButton("✏️ Edit", this);
    edit_btn_->setEnabled(false);
    connect(edit_btn_, &QPushButton::clicked, this, &RoleManager::onEditRole);
    button_layout->addWidget(edit_btn_);
    
    delete_btn_ = new QPushButton("🗑️ Delete", this);
    delete_btn_->setEnabled(false);
    connect(delete_btn_, &QPushButton::clicked, this, &RoleManager::onDeleteRole);
    button_layout->addWidget(delete_btn_);
    
    button_layout->addStretch();
    
    refresh_btn_ = new QPushButton("🔄 Refresh", this);
    connect(refresh_btn_, &QPushButton::clicked, this, &RoleManager::onRefreshClicked);
    button_layout->addWidget(refresh_btn_);
    
    layout->addLayout(button_layout);
}

void RoleManager::setApiClient(std::shared_ptr<voip::api::AdminApiClient> client) {
    apiClient_ = client;
    refresh();
}

void RoleManager::refresh() {
    if (!apiClient_) {
        std::cerr << "❌ No API client set for RoleManager" << std::endl;
        return;
    }
    
    std::cout << "🔄 Refreshing role list..." << std::endl;
    loadRoles();
}

void RoleManager::loadRoles() {
    if (!apiClient_) {
        std::cerr << "❌ No API client!" << std::endl;
        return;
    }
    
    std::cout << "📡 Loading roles from API..." << std::endl;
    
    apiClient_->getRoles([this](const QJsonArray& roles) {
        onRolesLoaded(roles);
    });
}

void RoleManager::onRolesLoaded(const QJsonArray& roles) {
    std::cout << "👑 Received " << roles.size() << " roles" << std::endl;
    
    all_roles_ = roles;
    populateRoleTable(roles);
    emit statusMessage(QString("Loaded %1 roles").arg(roles.size()));
}

void RoleManager::populateRoleTable(const QJsonArray& roles) {
    role_table_->setSortingEnabled(false);
    role_table_->setRowCount(0);
    
    for (const QJsonValue& value : roles) {
        QJsonObject role = value.toObject();
        
        int row = role_table_->rowCount();
        role_table_->insertRow(row);
        
        role_table_->setItem(row, 0, new QTableWidgetItem(QString::number(role["id"].toInt())));
        role_table_->setItem(row, 1, new QTableWidgetItem(role["name"].toString()));
        
        uint32_t permissions = role["permissions"].toInt();
        role_table_->setItem(row, 2, new QTableWidgetItem(permissionsToString(permissions)));
        
        role_table_->setItem(row, 3, new QTableWidgetItem(QString::number(role["priority"].toInt())));
        role_table_->setItem(row, 4, new QTableWidgetItem(QString::number(role["org_id"].toInt())));
    }
    
    role_table_->setSortingEnabled(true);
    role_table_->resizeColumnsToContents();
}

QString RoleManager::permissionsToString(uint32_t permissions) const {
    QStringList perm_list;
    
    // Map permission bits to names
    if (permissions & 0x01) perm_list << "Admin";
    if (permissions & 0x02) perm_list << "ManageChannels";
    if (permissions & 0x04) perm_list << "ManageUsers";
    if (permissions & 0x08) perm_list << "Speak";
    if (permissions & 0x10) perm_list << "Whisper";
    if (permissions & 0x20) perm_list << "MuteOthers";
    if (permissions & 0x40) perm_list << "MoveUsers";
    if (permissions & 0x80) perm_list << "KickUsers";
    
    if (perm_list.isEmpty()) {
        return "None";
    }
    
    return perm_list.join(", ");
}

void RoleManager::onCreateRole() {
    if (!apiClient_) {
        emit errorOccurred("No API client available");
        return;
    }

    CreateRoleDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject role_data = dialog.getRoleData();

        std::cout << "Creating role: " << role_data["name"].toString().toStdString() << std::endl;

        apiClient_->createRole(role_data, [this, role_data](int role_id) {
            if (role_id > 0) {
                emit statusMessage(QString("Role '%1' created successfully (ID: %2)")
                                 .arg(role_data["name"].toString())
                                 .arg(role_id));
                refresh();
            } else {
                emit errorOccurred("Failed to create role");
            }
        });
    }
}

void RoleManager::onEditRole() {
    int role_id = getSelectedRoleId();
    if (role_id < 0) return;

    if (!apiClient_) {
        emit errorOccurred("No API client available");
        return;
    }

    // Find the role in the cached data
    QJsonObject role_obj;
    for (const QJsonValue& value : all_roles_) {
        QJsonObject role = value.toObject();
        if (role["id"].toInt() == role_id) {
            role_obj = role;
            break;
        }
    }

    if (role_obj.isEmpty()) {
        emit errorOccurred(QString("Role %1 not found").arg(role_id));
        return;
    }

    // Create and show edit dialog
    EditRoleDialog dialog(role_obj, this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject updated_data = dialog.getUpdatedData();

        if (!updated_data.isEmpty()) {
            std::cout << "Updating role " << role_id << std::endl;

            apiClient_->updateRole(role_id, updated_data, [this, role_id](bool success) {
                if (success) {
                    emit statusMessage(QString("Role %1 updated successfully").arg(role_id));
                    refresh();
                } else {
                    emit errorOccurred(QString("Failed to update role %1").arg(role_id));
                }
            });
        } else {
            emit statusMessage("No changes made");
        }
    }
}

void RoleManager::onDeleteRole() {
    int role_id = getSelectedRoleId();
    if (role_id < 0) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Role",
        QString("Are you sure you want to delete role ID %1?").arg(role_id),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes && apiClient_) {
        apiClient_->deleteRole(role_id, [this, role_id](bool success) {
            if (success) {
                emit statusMessage(QString("Role %1 deleted").arg(role_id));
                refresh();
            } else {
                emit errorOccurred(QString("Failed to delete role %1").arg(role_id));
            }
        });
    }
}

void RoleManager::onRefreshClicked() {
    refresh();
}

void RoleManager::onSelectionChanged() {
    updateButtonStates();
}

void RoleManager::updateButtonStates() {
    bool has_selection = role_table_->currentRow() >= 0;
    
    edit_btn_->setEnabled(has_selection);
    delete_btn_->setEnabled(has_selection);
}

int RoleManager::getSelectedRoleId() const {
    int row = role_table_->currentRow();
    if (row < 0) return -1;
    
    QTableWidgetItem* id_item = role_table_->item(row, 0);
    if (!id_item) return -1;
    
    return id_item->text().toInt();
}

void RoleManager::onRoleDoubleClicked(int row, int column) {
    onEditRole();
}

} // namespace voip::ui::admin
