#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QJsonObject>

namespace voip::ui::admin {

/**
 * @brief Dialog for creating a new user
 */
class CreateUserDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateUserDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Get the user data from the form
     * @return JSON object with user creation data
     */
    QJsonObject getUserData() const;

private slots:
    void onAccept();
    void onUsernameChanged(const QString& text);

private:
    void setupUI();
    bool validateForm();

    // UI Components
    QLineEdit* username_edit_;
    QLineEdit* email_edit_;
    QLineEdit* password_edit_;
    QLineEdit* confirm_password_edit_;
    QComboBox* org_combo_;
    
    QPushButton* create_btn_;
    QPushButton* cancel_btn_;
};

} // namespace voip::ui::admin
