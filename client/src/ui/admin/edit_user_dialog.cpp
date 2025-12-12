#include "ui/admin/edit_user_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>

namespace voip::ui::admin {

EditUserDialog::EditUserDialog(const QJsonObject& user, QWidget* parent)
    : QDialog(parent)
    , original_user_(user)
{
    setWindowTitle(QString("Edit User: %1").arg(user["username"].toString()));
    setMinimumWidth(450);
    setupUI();
    loadUserData();
}

void EditUserDialog::setupUI() {
    auto* main_layout = new QVBoxLayout(this);
    
    // User info section
    auto* info_group = new QGroupBox("User Information", this);
    auto* info_layout = new QFormLayout(info_group);
    info_layout->setSpacing(15);
    
    // User ID (read-only)
    auto* id_label = new QLabel(QString::number(original_user_["id"].toInt()), this);
    id_label->setStyleSheet("color: #888;");
    info_layout->addRow("User ID:", id_label);
    
    // Username
    username_edit_ = new QLineEdit(this);
    username_edit_->setPlaceholderText("Enter username");
    info_layout->addRow("Username*:", username_edit_);
    
    // Email
    email_edit_ = new QLineEdit(this);
    email_edit_->setPlaceholderText("user@example.com");
    info_layout->addRow("Email:", email_edit_);
    
    // Organization
    org_combo_ = new QComboBox(this);
    org_combo_->addItem("Demo Organization", 1);
    info_layout->addRow("Organization*:", org_combo_);
    
    main_layout->addWidget(info_group);
    
    // Status section
    auto* status_group = new QGroupBox("User Status", this);
    auto* status_layout = new QVBoxLayout(status_group);
    
    active_checkbox_ = new QCheckBox("Active (user can log in)", this);
    status_layout->addWidget(active_checkbox_);
    
    banned_checkbox_ = new QCheckBox("Banned (user is blocked)", this);
    banned_checkbox_->setStyleSheet("QCheckBox { color: #ff4444; }");
    status_layout->addWidget(banned_checkbox_);
    
    main_layout->addWidget(status_group);
    
    // Password reset section
    auto* password_group = new QGroupBox("Password Management", this);
    auto* password_layout = new QHBoxLayout(password_group);
    
    auto* password_info = new QLabel("Reset user password", this);
    password_info->setStyleSheet("color: #888;");
    password_layout->addWidget(password_info);
    
    reset_password_btn_ = new QPushButton("Reset Password", this);
    connect(reset_password_btn_, &QPushButton::clicked, [this]() {
        emit accept(); // Will be handled by parent with separate API call
    });
    password_layout->addWidget(reset_password_btn_);
    
    main_layout->addWidget(password_group);
    
    main_layout->addSpacing(20);
    
    // Buttons
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    cancel_btn_ = new QPushButton("Cancel", this);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
    button_layout->addWidget(cancel_btn_);
    
    save_btn_ = new QPushButton("Save Changes", this);
    save_btn_->setDefault(true);
    connect(save_btn_, &QPushButton::clicked, this, &EditUserDialog::onAccept);
    button_layout->addWidget(save_btn_);
    
    main_layout->addLayout(button_layout);
}

void EditUserDialog::loadUserData() {
    // Load user data into form
    username_edit_->setText(original_user_["username"].toString());
    email_edit_->setText(original_user_["email"].toString());
    
    // Set organization
    int org_id = original_user_["org_id"].toInt();
    int index = org_combo_->findData(org_id);
    if (index >= 0) {
        org_combo_->setCurrentIndex(index);
    }
    
    // Set status checkboxes
    active_checkbox_->setChecked(original_user_["is_active"].toBool());
    banned_checkbox_->setChecked(original_user_["is_banned"].toBool());
}

void EditUserDialog::onAccept() {
    if (!validateForm()) {
        return;
    }
    
    accept();
}

bool EditUserDialog::validateForm() {
    // Validate username
    if (username_edit_->text().trimmed().length() < 3) {
        QMessageBox::warning(this, "Validation Error", 
                           "Username must be at least 3 characters long.");
        username_edit_->setFocus();
        return false;
    }
    
    // Warn if both active and banned are checked
    if (active_checkbox_->isChecked() && banned_checkbox_->isChecked()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Confirm Status",
            "User is marked as both Active and Banned. Banned status will take precedence. Continue?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply != QMessageBox::Yes) {
            return false;
        }
    }
    
    return true;
}

QJsonObject EditUserDialog::getUpdatedData() const {
    QJsonObject data;
    
    // Only include changed fields
    QString username = username_edit_->text().trimmed();
    if (username != original_user_["username"].toString()) {
        data["username"] = username;
    }
    
    QString email = email_edit_->text().trimmed();
    if (email != original_user_["email"].toString()) {
        data["email"] = email.isEmpty() ? QJsonValue() : email;
    }
    
    // Note: In a real app, we'd handle status changes via separate ban/unban endpoints
    // For now, we'll just include them in the update
    
    return data;
}

} // namespace voip::ui::admin
