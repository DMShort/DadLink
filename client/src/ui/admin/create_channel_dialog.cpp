#include "ui/admin/create_channel_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

namespace voip::ui::admin {

CreateChannelDialog::CreateChannelDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create New Channel");
    setMinimumWidth(450);
    setupUI();
}

void CreateChannelDialog::setupUI() {
    auto* main_layout = new QVBoxLayout(this);

    // Form layout
    auto* form_layout = new QFormLayout();
    form_layout->setSpacing(15);

    // Channel Name
    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText("Enter channel name");
    connect(name_edit_, &QLineEdit::textChanged, this, &CreateChannelDialog::onNameChanged);
    form_layout->addRow("Channel Name*:", name_edit_);

    // Description
    description_edit_ = new QTextEdit(this);
    description_edit_->setPlaceholderText("Enter channel description (optional)");
    description_edit_->setMaximumHeight(100);
    form_layout->addRow("Description:", description_edit_);

    // Max Users
    max_users_spin_ = new QSpinBox(this);
    max_users_spin_->setRange(0, 10000);
    max_users_spin_->setValue(50);
    max_users_spin_->setSpecialValueText("Unlimited");
    form_layout->addRow("Max Users:", max_users_spin_);

    // Persistent
    persistent_checkbox_ = new QCheckBox("Channel persists when empty", this);
    persistent_checkbox_->setChecked(true);
    form_layout->addRow("", persistent_checkbox_);

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

    create_btn_ = new QPushButton("Create Channel", this);
    create_btn_->setDefault(true);
    create_btn_->setEnabled(false);
    connect(create_btn_, &QPushButton::clicked, this, &CreateChannelDialog::onAccept);
    button_layout->addWidget(create_btn_);

    main_layout->addLayout(button_layout);

    // Set focus
    name_edit_->setFocus();
}

void CreateChannelDialog::onNameChanged(const QString& text) {
    // Enable create button if name is valid
    create_btn_->setEnabled(text.trimmed().length() >= 2);
}

void CreateChannelDialog::onAccept() {
    if (!validateForm()) {
        return;
    }

    accept();
}

bool CreateChannelDialog::validateForm() {
    // Validate channel name
    if (name_edit_->text().trimmed().length() < 2) {
        QMessageBox::warning(this, "Validation Error",
                           "Channel name must be at least 2 characters long.");
        name_edit_->setFocus();
        return false;
    }

    return true;
}

QJsonObject CreateChannelDialog::getChannelData() const {
    QJsonObject data;
    data["name"] = name_edit_->text().trimmed();
    data["org_id"] = 1;  // Default organization

    // Description is required by server (not optional)
    QString description = description_edit_->toPlainText().trimmed();
    data["description"] = description.isEmpty() ? "" : description;

    // Max users is optional, only send if > 0
    int max_users = max_users_spin_->value();
    if (max_users > 0) {
        data["max_users"] = max_users;
    }

    // Note: parent_id is optional, not sending it means top-level channel
    // Note: persistent field not supported by server API yet

    return data;
}

} // namespace voip::ui::admin
