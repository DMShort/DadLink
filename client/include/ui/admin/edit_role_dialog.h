#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QJsonObject>

namespace voip::ui::admin {

/**
 * @brief Dialog for editing an existing role
 */
class EditRoleDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditRoleDialog(const QJsonObject& role, QWidget* parent = nullptr);

    /**
     * @brief Get the updated role data from the form
     * @return JSON object with role update data (only changed fields)
     */
    QJsonObject getUpdatedData() const;

private slots:
    void onAccept();

private:
    void setupUI();
    void loadRoleData();
    bool validateForm();
    uint32_t getSelectedPermissions() const;
    void setPermissionCheckboxes(uint32_t permissions);

    // Original role data
    QJsonObject original_role_;

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

    QPushButton* save_btn_;
    QPushButton* cancel_btn_;
};

} // namespace voip::ui::admin
