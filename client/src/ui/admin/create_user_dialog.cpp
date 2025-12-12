#include "ui/admin/create_user_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QRegularExpression>

namespace voip::ui::admin {

CreateUserDialog::CreateUserDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create New User");
    setMinimumWidth(400);
    setupUI();
}

void CreateUserDialog::setupUI() {
    auto* main_layout = new QVBoxLayout(this);
    
    // Form layout
    auto* form_layout = new QFormLayout();
    form_layout->setSpacing(15);
    
    // Username
    username_edit_ = new QLineEdit(this);
    username_edit_->setPlaceholderText("Enter username (3-20 characters)");
    QRegularExpression username_regex("^[a-zA-Z0-9_]{3,20}$");
    username_edit_->setValidator(new QRegularExpressionValidator(username_regex, this));
    connect(username_edit_, &QLineEdit::textChanged, this, &CreateUserDialog::onUsernameChanged);
    form_layout->addRow("Username*:", username_edit_);
    
    // Email
    email_edit_ = new QLineEdit(this);
    email_edit_->setPlaceholderText("user@example.com (optional)");
    form_layout->addRow("Email:", email_edit_);
    
    // Password
    password_edit_ = new QLineEdit(this);
    password_edit_->setEchoMode(QLineEdit::Password);
    password_edit_->setPlaceholderText("Enter password (min 6 characters)");
    form_layout->addRow("Password*:", password_edit_);
    
    // Confirm Password
    confirm_password_edit_ = new QLineEdit(this);
    confirm_password_edit_->setEchoMode(QLineEdit::Password);
    confirm_password_edit_->setPlaceholderText("Re-enter password");
    form_layout->addRow("Confirm Password*:", confirm_password_edit_);
    
    // Organization
    org_combo_ = new QComboBox(this);
    org_combo_->addItem("Demo Organization", 1);
    form_layout->addRow("Organization*:", org_combo_);
    
    main_layout->addLayout(form_layout);
    
    // Info label
    auto* info_label = new QLabel("* Required fields", this);
    info_label->setStyleSheet("color: #888; font-size: 10px;");
    main_layout->addWidget(info_label);
    
    main_layout->addSpacing(20);
    
    // Buttons
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    cancel_btn_ = new QPushButton("Cancel", this);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
    button_layout->addWidget(cancel_btn_);
    
    create_btn_ = new QPushButton("Create User", this);
    create_btn_->setDefault(true);
    create_btn_->setEnabled(false);
    connect(create_btn_, &QPushButton::clicked, this, &CreateUserDialog::onAccept);
    button_layout->addWidget(create_btn_);
    
    main_layout->addLayout(button_layout);
    
    // Set focus
    username_edit_->setFocus();
}

void CreateUserDialog::onUsernameChanged(const QString& text) {
    // Enable create button if username is valid
    create_btn_->setEnabled(text.length() >= 3);
}

void CreateUserDialog::onAccept() {
    if (!validateForm()) {
        return;
    }
    
    accept();
}

bool CreateUserDialog::validateForm() {
    // Validate username
    if (username_edit_->text().trimmed().length() < 3) {
        QMessageBox::warning(this, "Validation Error", 
                           "Username must be at least 3 characters long.");
        username_edit_->setFocus();
        return false;
    }
    
    // Validate password
    QString password = password_edit_->text();
    if (password.length() < 6) {
        QMessageBox::warning(this, "Validation Error", 
                           "Password must be at least 6 characters long.");
        password_edit_->setFocus();
        return false;
    }
    
    // Confirm password match
    if (password != confirm_password_edit_->text()) {
        QMessageBox::warning(this, "Validation Error", 
                           "Passwords do not match.");
        confirm_password_edit_->setFocus();
        return false;
    }
    
    return true;
}

QJsonObject CreateUserDialog::getUserData() const {
    QJsonObject data;
    data["username"] = username_edit_->text().trimmed();
    data["password"] = password_edit_->text();
    data["org_id"] = org_combo_->currentData().toInt();
    
    QString email = email_edit_->text().trimmed();
    if (!email.isEmpty()) {
        data["email"] = email;
    }
    
    return data;
}

} // namespace voip::ui::admin
