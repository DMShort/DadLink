#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QJsonObject>

namespace voip::ui::admin {

/**
 * @brief Dialog for editing an existing user
 */
class EditUserDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditUserDialog(const QJsonObject& user, QWidget* parent = nullptr);
    
    /**
     * @brief Get the updated user data from the form
     * @return JSON object with user update data (only changed fields)
     */
    QJsonObject getUpdatedData() const;

private slots:
    void onAccept();

private:
    void setupUI();
    void loadUserData();
    bool validateForm();

    // Original user data
    QJsonObject original_user_;
    
    // UI Components
    QLineEdit* username_edit_;
    QLineEdit* email_edit_;
    QComboBox* org_combo_;
    QCheckBox* active_checkbox_;
    QCheckBox* banned_checkbox_;
    
    QPushButton* save_btn_;
    QPushButton* cancel_btn_;
    QPushButton* reset_password_btn_;
};

} // namespace voip::ui::admin
