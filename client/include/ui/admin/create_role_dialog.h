#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QJsonObject>
#include <QMap>

namespace voip::ui::admin {

/**
 * @brief Dialog for creating a new role
 */
class CreateRoleDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateRoleDialog(QWidget* parent = nullptr);

    /**
     * @brief Get the role data from the form
     * @return JSON object with role creation data
     */
    QJsonObject getRoleData() const;

private slots:
    void onAccept();
    void onNameChanged(const QString& text);

private:
    void setupUI();
    bool validateForm();
    uint32_t getSelectedPermissions() const;

    // UI Components
    QLineEdit* name_edit_;
    QSpinBox* priority_spin_;

    // Permission checkboxes
    QCheckBox* admin_checkbox_;
    QCheckBox* manage_channels_checkbox_;
    QCheckBox* manage_users_checkbox_;
    QCheckBox* speak_checkbox_;
    QCheckBox* whisper_checkbox_;
    QCheckBox* mute_others_checkbox_;
    QCheckBox* move_users_checkbox_;
    QCheckBox* kick_users_checkbox_;

    QPushButton* create_btn_;
    QPushButton* cancel_btn_;
};

} // namespace voip::ui::admin
