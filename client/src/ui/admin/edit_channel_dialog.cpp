#include "ui/admin/edit_channel_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>

namespace voip::ui::admin {

EditChannelDialog::EditChannelDialog(const QJsonObject& channel, QWidget* parent)
    : QDialog(parent)
    , original_channel_(channel)
{
    setWindowTitle(QString("Edit Channel: %1").arg(channel["name"].toString()));
    setMinimumWidth(450);
    setupUI();
    loadChannelData();
}

void EditChannelDialog::setupUI() {
    auto* main_layout = new QVBoxLayout(this);

    // Channel info section
    auto* info_group = new QGroupBox("Channel Information", this);
    auto* info_layout = new QFormLayout(info_group);
    info_layout->setSpacing(15);

    // Channel ID (read-only)
    auto* id_label = new QLabel(QString::number(original_channel_["id"].toInt()), this);
    id_label->setStyleSheet("color: #888;");
    info_layout->addRow("Channel ID:", id_label);

    // Channel Name
    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText("Enter channel name");
    info_layout->addRow("Channel Name*:", name_edit_);

    // Description
    description_edit_ = new QTextEdit(this);
    description_edit_->setPlaceholderText("Enter channel description");
    description_edit_->setMaximumHeight(100);
    info_layout->addRow("Description:", description_edit_);

    // Max Users
    max_users_spin_ = new QSpinBox(this);
    max_users_spin_->setRange(0, 10000);
    max_users_spin_->setSpecialValueText("Unlimited");
    info_layout->addRow("Max Users:", max_users_spin_);

    // Persistent
    persistent_checkbox_ = new QCheckBox("Channel persists when empty", this);
    info_layout->addRow("", persistent_checkbox_);

    main_layout->addWidget(info_group);

    main_layout->addSpacing(20);

    // Buttons
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();

    cancel_btn_ = new QPushButton("Cancel", this);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
    button_layout->addWidget(cancel_btn_);

    save_btn_ = new QPushButton("Save Changes", this);
    save_btn_->setDefault(true);
    connect(save_btn_, &QPushButton::clicked, this, &EditChannelDialog::onAccept);
    button_layout->addWidget(save_btn_);

    main_layout->addLayout(button_layout);
}

void EditChannelDialog::loadChannelData() {
    // Load channel data into form
    name_edit_->setText(original_channel_["name"].toString());
    description_edit_->setPlainText(original_channel_["description"].toString());
    max_users_spin_->setValue(original_channel_["max_users"].toInt());
    persistent_checkbox_->setChecked(original_channel_["persistent"].toBool());
}

void EditChannelDialog::onAccept() {
    if (!validateForm()) {
        return;
    }

    accept();
}

bool EditChannelDialog::validateForm() {
    // Validate channel name
    if (name_edit_->text().trimmed().length() < 2) {
        QMessageBox::warning(this, "Validation Error",
                           "Channel name must be at least 2 characters long.");
        name_edit_->setFocus();
        return false;
    }

    return true;
}

QJsonObject EditChannelDialog::getUpdatedData() const {
    QJsonObject data;

    // Only include changed fields
    QString name = name_edit_->text().trimmed();
    if (name != original_channel_["name"].toString()) {
        data["name"] = name;
    }

    QString description = description_edit_->toPlainText().trimmed();
    QString original_desc = original_channel_["description"].toString();
    if (description != original_desc) {
        // Server expects string, not null
        data["description"] = description;
    }

    int max_users = max_users_spin_->value();
    if (max_users != original_channel_["max_users"].toInt()) {
        data["max_users"] = max_users;
    }

    // Note: persistent field not supported by server API yet

    return data;
}

} // namespace voip::ui::admin
