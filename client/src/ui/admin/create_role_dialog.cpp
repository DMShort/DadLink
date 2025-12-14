#include "ui/admin/create_role_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

namespace voip::ui::admin {

CreateRoleDialog::CreateRoleDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create New Role");
    setMinimumWidth(500);
    setupUI();
}

void CreateRoleDialog::setupUI() {
    auto* main_layout = new QVBoxLayout(this);

    // Basic info section
    auto* info_group = new QGroupBox("Role Information", this);
    auto* info_layout = new QFormLayout(info_group);
    info_layout->setSpacing(15);

    // Role Name
    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText("Enter role name");
    connect(name_edit_, &QLineEdit::textChanged, this, &CreateRoleDialog::onNameChanged);
    info_layout->addRow("Role Name*:", name_edit_);

    // Priority
    priority_spin_ = new QSpinBox(this);
    priority_spin_->setRange(0, 1000);
    priority_spin_->setValue(100);
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
    speak_checkbox_->setChecked(true);  // Default enabled
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

    // Info label
    auto* info_label = new QLabel("* Required fields", this);
    info_label->setStyleSheet("color: #888; font-size: 10px;");
    main_layout->addWidget(info_label);

    main_layout->addSpacing(10);

    // Buttons
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();

    cancel_btn_ = new QPushButton("Cancel", this);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
    button_layout->addWidget(cancel_btn_);

    create_btn_ = new QPushButton("Create Role", this);
    create_btn_->setDefault(true);
    create_btn_->setEnabled(false);
    connect(create_btn_, &QPushButton::clicked, this, &CreateRoleDialog::onAccept);
    button_layout->addWidget(create_btn_);

    main_layout->addLayout(button_layout);

    // Set focus
    name_edit_->setFocus();
}

void CreateRoleDialog::onNameChanged(const QString& text) {
    // Enable create button if name is valid
    create_btn_->setEnabled(text.trimmed().length() >= 2);
}

void CreateRoleDialog::onAccept() {
    if (!validateForm()) {
        return;
    }

    accept();
}

bool CreateRoleDialog::validateForm() {
    // Validate role name
    if (name_edit_->text().trimmed().length() < 2) {
        QMessageBox::warning(this, "Validation Error",
                           "Role name must be at least 2 characters long.");
        name_edit_->setFocus();
        return false;
    }

    return true;
}

uint32_t CreateRoleDialog::getSelectedPermissions() const {
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

QJsonObject CreateRoleDialog::getRoleData() const {
    QJsonObject data;
    data["name"] = name_edit_->text().trimmed();
    data["permissions"] = static_cast<int>(getSelectedPermissions());
    data["priority"] = priority_spin_->value();
    data["org_id"] = 1;  // Default organization

    return data;
}

} // namespace voip::ui::admin
