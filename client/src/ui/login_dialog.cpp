#include "ui/login_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QIcon>
#include <QFont>

namespace voip::ui {

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , isConnecting_(false)
{
    setupUI();
    setupConnections();
    
    // Set window properties
    setWindowTitle("VoIP Client - Login");
    setModal(true);
    setFixedSize(480, 500);
    
    // Initialize with defaults
    setServerAddress("127.0.0.1");
    setServerPort(9000);
    
    // Set dialog-level stylesheet for consistent dark theme
    setStyleSheet(R"(
        QDialog {
            background-color: #36393f;
        }
        QGroupBox {
            border: 1px solid #4a4a4a;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
            color: #dcddde;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QLabel {
            color: #dcddde;
        }
        QLineEdit {
            padding: 8px;
            border: 1px solid #4a4a4a;
            border-radius: 3px;
            background: #40444b;
            color: #dcddde;
        }
        QLineEdit:focus {
            border: 1px solid #5865f2;
        }
        QCheckBox {
            color: #dcddde;
        }
        QPushButton {
            padding: 8px 16px;
            border-radius: 3px;
            background: #5865f2;
            color: white;
            border: none;
        }
        QPushButton:hover {
            background: #4752c4;
        }
        QPushButton:disabled {
            background: #4a4a4a;
            color: #888;
        }
    )");
}

void LoginDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Title
    titleLabel_ = new QLabel("🎤 VoIP Client", this);
    QFont titleFont = titleLabel_->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    titleLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel_);

    // Credentials group
    auto* credentialsGroup = new QGroupBox("Account", this);
    auto* credLayout = new QVBoxLayout(credentialsGroup);
    credLayout->setSpacing(25);
    credLayout->setContentsMargins(20, 30, 20, 25);

    // Username row
    auto* usernameRow = new QHBoxLayout();
    usernameRow->setSpacing(15);
    auto* usernameLabel = new QLabel("Username:", credentialsGroup);
    usernameLabel->setFixedWidth(110);
    usernameLabel->setMinimumHeight(30);
    usernameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    usernameEdit_ = new QLineEdit(credentialsGroup);
    usernameEdit_->setPlaceholderText("Enter your username");
    usernameRow->addWidget(usernameLabel);
    usernameRow->addWidget(usernameEdit_);
    credLayout->addLayout(usernameRow);

    // Password row
    auto* passwordRow = new QHBoxLayout();
    passwordRow->setSpacing(15);
    auto* passwordLabel = new QLabel("Password:", credentialsGroup);
    passwordLabel->setFixedWidth(110);
    passwordLabel->setMinimumHeight(30);
    passwordLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    passwordEdit_ = new QLineEdit(credentialsGroup);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("Enter your password");
    passwordRow->addWidget(passwordLabel);
    passwordRow->addWidget(passwordEdit_);
    credLayout->addLayout(passwordRow);

    mainLayout->addWidget(credentialsGroup);

    // Server group
    auto* serverGroup = new QGroupBox("Server", this);
    auto* serverLayout = new QVBoxLayout(serverGroup);
    serverLayout->setSpacing(25);
    serverLayout->setContentsMargins(20, 30, 20, 25);

    // Address row
    auto* addressRow = new QHBoxLayout();
    addressRow->setSpacing(15);
    auto* addressLabel = new QLabel("Address:", serverGroup);
    addressLabel->setFixedWidth(110);
    addressLabel->setMinimumHeight(30);
    addressLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    serverAddressEdit_ = new QLineEdit(serverGroup);
    serverAddressEdit_->setPlaceholderText("127.0.0.1");
    addressRow->addWidget(addressLabel);
    addressRow->addWidget(serverAddressEdit_);
    serverLayout->addLayout(addressRow);

    // Port row
    auto* portRow = new QHBoxLayout();
    portRow->setSpacing(15);
    auto* portLabel = new QLabel("Port:", serverGroup);
    portLabel->setFixedWidth(110);
    portLabel->setMinimumHeight(30);
    portLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    serverPortEdit_ = new QLineEdit(serverGroup);
    serverPortEdit_->setPlaceholderText("9000");
    serverPortEdit_->setMaxLength(5);
    portRow->addWidget(portLabel);
    portRow->addWidget(serverPortEdit_);
    serverLayout->addLayout(portRow);

    mainLayout->addWidget(serverGroup);

    // Remember credentials
    rememberCheckBox_ = new QCheckBox("Remember credentials", this);
    mainLayout->addWidget(rememberCheckBox_);

    // Use TLS/SSL checkbox
    useTlsCheckBox_ = new QCheckBox("Use TLS/SSL (wss://)", this);
    useTlsCheckBox_->setChecked(true);  // Default to TLS since server has certificates
    mainLayout->addWidget(useTlsCheckBox_);

    // Status label
    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    mainLayout->addWidget(statusLabel_);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    cancelButton_ = new QPushButton("Cancel", this);
    cancelButton_->setMinimumWidth(80);
    buttonLayout->addWidget(cancelButton_);

    loginButton_ = new QPushButton("Login", this);
    loginButton_->setMinimumWidth(80);
    loginButton_->setDefault(true);
    loginButton_->setEnabled(false);
    buttonLayout->addWidget(loginButton_);
    
    registerButton_ = new QPushButton("Register New Account", this);
    buttonLayout->addWidget(registerButton_);
    
    mainLayout->addLayout(buttonLayout);

    // Set focus to username
    usernameEdit_->setFocus();
}

void LoginDialog::setupConnections() {
    connect(loginButton_, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    
    // Enable login button when all fields are filled
    connect(usernameEdit_, &QLineEdit::textChanged, this, &LoginDialog::onInputChanged);
    connect(passwordEdit_, &QLineEdit::textChanged, this, &LoginDialog::onInputChanged);
    connect(serverAddressEdit_, &QLineEdit::textChanged, this, &LoginDialog::onInputChanged);
    connect(serverPortEdit_, &QLineEdit::textChanged, this, &LoginDialog::onInputChanged);
    
    // Allow Enter key to submit
    connect(usernameEdit_, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::onLoginClicked() {
    if (isConnecting_) {
        return;
    }

    // Validate input
    if (username().isEmpty() || password().isEmpty() || 
        serverAddress().isEmpty() || serverPort() == 0) {
        setStatusMessage("Please fill in all fields", true);
        return;
    }

    setConnecting(true);
    emit loginRequested(username(), password(), serverAddress() + ":" + QString::number(serverPort()));
}

void LoginDialog::onRegisterClicked() {
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();
    QString server = serverAddressEdit_->text().trimmed();
    
    if (username.isEmpty() || password.isEmpty() || server.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please fill in all fields");
        return;
    }
    
    // Validate username
    if (username.length() < 3 || username.length() > 20) {
        QMessageBox::warning(this, "Invalid Username", "Username must be 3-20 characters");
        return;
    }
    
    // Validate password
    if (password.length() < 3) {
        QMessageBox::warning(this, "Invalid Password", "Password must be at least 3 characters");
        return;
    }
    
    emit registerRequested(username, password, server + ":" + QString::number(serverPort()));
    // Don't accept() yet - wait for server response
}

void LoginDialog::onCancelClicked() {
    if (isConnecting_) {
        setConnecting(false);
    }
    emit cancelRequested();
    reject();
}

void LoginDialog::onInputChanged() {
    validateInput();
    statusLabel_->clear();
}

void LoginDialog::validateInput() {
    bool valid = !usernameEdit_->text().isEmpty() &&
                 !passwordEdit_->text().isEmpty() &&
                 !serverAddressEdit_->text().isEmpty() &&
                 !serverPortEdit_->text().isEmpty();
    
    loginButton_->setEnabled(valid && !isConnecting_);
}

QString LoginDialog::username() const {
    return usernameEdit_->text().trimmed();
}

QString LoginDialog::password() const {
    return passwordEdit_->text();
}

QString LoginDialog::serverAddress() const {
    return serverAddressEdit_->text().trimmed();
}

uint16_t LoginDialog::serverPort() const {
    bool ok;
    uint16_t port = serverPortEdit_->text().toUShort(&ok);
    return ok ? port : 0;
}

bool LoginDialog::rememberCredentials() const {
    return rememberCheckBox_->isChecked();
}

bool LoginDialog::useTls() const {
    return useTlsCheckBox_->isChecked();
}

void LoginDialog::setUsername(const QString& username) {
    usernameEdit_->setText(username);
}

void LoginDialog::setServerAddress(const QString& address) {
    serverAddressEdit_->setText(address);
}

void LoginDialog::setServerPort(uint16_t port) {
    serverPortEdit_->setText(QString::number(port));
}

void LoginDialog::setRememberCredentials(bool remember) {
    rememberCheckBox_->setChecked(remember);
}

void LoginDialog::setUseTls(bool useTls) {
    useTlsCheckBox_->setChecked(useTls);
}

void LoginDialog::setStatusMessage(const QString& message, bool isError) {
    statusLabel_->setText(message);
    
    if (isError) {
        statusLabel_->setStyleSheet("QLabel { color: #d32f2f; font-weight: bold; }");
    } else {
        statusLabel_->setStyleSheet("QLabel { color: #388e3c; font-style: italic; }");
    }
}

void LoginDialog::setConnecting(bool connecting) {
    isConnecting_ = connecting;
    
    usernameEdit_->setEnabled(!connecting);
    passwordEdit_->setEnabled(!connecting);
    serverAddressEdit_->setEnabled(!connecting);
    serverPortEdit_->setEnabled(!connecting);
    rememberCheckBox_->setEnabled(!connecting);
    
    if (connecting) {
        loginButton_->setText("Connecting...");
        loginButton_->setEnabled(false);
        setStatusMessage("Connecting to server...", false);
    } else {
        loginButton_->setText("Connect");
        validateInput();
        statusLabel_->clear();
    }
}

} // namespace voip::ui
