#include "ui/admin/edit_role_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

namespace voip::ui::admin {

EditRoleDialog::EditRoleDialog(const QJsonObject& role, QWidget* parent)
    : QDialog(parent)
    , original_role_(role)
{
    setWindowTitle(QString("Edit Role: %1").arg(role["name"].toString()));
    setMinimumWidth(500);
    setupUI();
    loadRoleData();
}

void EditRoleDialog::setupUI() {
    auto* main_layout = new QVBoxLayout(this);

    // Basic info section
    auto* info_group = new QGroupBox("Role Information", this);
    auto* info_layout = new QFormLayout(info_group);
    info_layout->setSpacing(15);

    // Role ID (read-only)
    auto* id_label = new QLabel(QString::number(original_role_["id"].toInt()), this);
    id_label->setStyleSheet("color: #888;");
    info_layout->addRow("Role ID:", id_label);

    // Role Name
    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText("Enter role name");
    info_layout->addRow("Role Name*:", name_edit_);

    // Priority
    priority_spin_ = new QSpinBox(this);
    priority_spin_->setRange(0, 1000);
    priority_spin_->setToolTip("Higher priority roles override lower priority roles");
    info_layout->addRow("Priority:", priority_spin_);

    main_layout->addWidget(info_group);

    // Permissions section
    auto* perm_group = new QGroupBox("Permissions", this);
    auto* perm_layout = new QVBoxLayout(perm_group);

    admin_checkbox_ = new QCheckBox("Administrator (full access)", this);
    perm_layout->addWidget(admin_checkbox_);

    manage_channels_checkbox_ = new QCheckBox("Manage Channels (create, edit, delete)", this);
    perm_layout->addWidget(manage_channels_checkbox_);

    manage_users_checkbox_ = new QCheckBox("Manage Users (ban, roles)", this);
    perm_layout->addWidget(manage_users_checkbox_);

    speak_checkbox_ = new QCheckBox("Speak in channels", this);
    perm_layout->addWidget(speak_checkbox_);

    whisper_checkbox_ = new QCheckBox("Whisper to other users", this);
    perm_layout->addWidget(whisper_checkbox_);

    mute_others_checkbox_ = new QCheckBox("Mute other users", this);
    perm_layout->addWidget(mute_others_checkbox_);

    move_users_checkbox_ = new QCheckBox("Move users between channels", this);
    perm_layout->addWidget(move_users_checkbox_);

    kick_users_checkbox_ = new QCheckBox("Kick users from channels", this);
    perm_layout->addWidget(kick_users_checkbox_);

    main_layout->addWidget(perm_group);

    main_layout->addSpacing(10);

    // Buttons
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();

    cancel_btn_ = new QPushButton("Cancel", this);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
    button_layout->addWidget(cancel_btn_);

    save_btn_ = new QPushButton("Save Changes", this);
    save_btn_->setDefault(true);
    connect(save_btn_, &QPushButton::clicked, this, &EditRoleDialog::onAccept);
    button_layout->addWidget(save_btn_);

    main_layout->addLayout(button_layout);
}

void EditRoleDialog::loadRoleData() {
    // Load role data into form
    name_edit_->setText(original_role_["name"].toString());
    priority_spin_->setValue(original_role_["priority"].toInt());

    // Load permissions
    uint32_t permissions = original_role_["permissions"].toInt();
    setPermissionCheckboxes(permissions);
}

void EditRoleDialog::setPermissionCheckboxes(uint32_t permissions) {
    admin_checkbox_->setChecked(permissions & 0x01);
    manage_channels_checkbox_->setChecked(permissions & 0x02);
    manage_users_checkbox_->setChecked(permissions & 0x04);
    speak_checkbox_->setChecked(permissions & 0x08);
    whisper_checkbox_->setChecked(permissions & 0x10);
    mute_others_checkbox_->setChecked(permissions & 0x20);
    move_users_checkbox_->setChecked(permissions & 0x40);
    kick_users_checkbox_->setChecked(permissions & 0x80);
}

void EditRoleDialog::onAccept() {
    if (!validateForm()) {
        return;
    }

    accept();
}

bool EditRoleDialog::validateForm() {
    // Validate role name
    if (name_edit_->text().trimmed().length() < 2) {
        QMessageBox::warning(this, "Validation Error",
                           "Role name must be at least 2 characters long.");
        name_edit_->setFocus();
        return false;
    }

    return true;
}

uint32_t EditRoleDialog::getSelectedPermissions() const {
    uint32_t permissions = 0;

    if (admin_checkbox_->isChecked()) permissions |= 0x01;
    if (manage_channels_checkbox_->isChecked()) permissions |= 0x02;
    if (manage_users_checkbox_->isChecked()) permissions |= 0x04;
    if (speak_checkbox_->isChecked()) permissions |= 0x08;
    if (whisper_checkbox_->isChecked()) permissions |= 0x10;
    if (mute_others_checkbox_->isChecked()) permissions |= 0x20;
    if (move_users_checkbox_->isChecked()) permissions |= 0x40;
    if (kick_users_checkbox_->isChecked()) permissions |= 0x80;

    return permissions;
}

QJsonObject EditRoleDialog::getUpdatedData() const {
    QJsonObject data;

    // Only include changed fields
    QString name = name_edit_->text().trimmed();
    if (name != original_role_["name"].toString()) {
        data["name"] = name;
    }

    int priority = priority_spin_->value();
    if (priority != original_role_["priority"].toInt()) {
        data["priority"] = priority;
    }

    uint32_t permissions = getSelectedPermissions();
    if (permissions != static_cast<uint32_t>(original_role_["permissions"].toInt())) {
        data["permissions"] = static_cast<int>(permissions);
    }

    return data;
}

} // namespace voip::ui::admin
