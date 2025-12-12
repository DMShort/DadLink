#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <memory>

namespace voip::ui {

/// Login dialog for user authentication
class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog() override = default;

    // Getters for login credentials
    QString username() const;
    QString password() const;
    QString serverAddress() const;
    uint16_t serverPort() const;
    bool rememberCredentials() const;
    bool useTls() const;

    // Setters for pre-filling
    void setUsername(const QString& username);
    void setServerAddress(const QString& address);
    void setServerPort(uint16_t port);
    void setRememberCredentials(bool remember);
    void setUseTls(bool useTls);

    // Status display
    void setStatusMessage(const QString& message, bool isError = false);
    void setConnecting(bool connecting);

signals:
    void loginRequested(const QString& username, const QString& password, const QString& server);
    void registerRequested(const QString& username, const QString& password, const QString& server);
    void cancelRequested();

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onCancelClicked();
    void onInputChanged();

private:
    void setupUI();
    void setupConnections();
    void validateInput();

    // UI Components
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QLineEdit* serverAddressEdit_;
    QLineEdit* serverPortEdit_;
    QCheckBox* rememberCheckBox_;
    QCheckBox* useTlsCheckBox_;
    QPushButton* loginButton_;
    QPushButton* registerButton_;
    QPushButton* cancelButton_;
    QLabel* statusLabel_;
    QLabel* titleLabel_;

    // State
    bool isConnecting_;
};

} // namespace voip::ui
