#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include "ui/login_dialog.h"
#include "ui/main_window.h"
#include "session/voice_session.h"
#include "network/websocket_client.h"
#include <iostream>

using namespace voip;

int main(int argc, char *argv[]) {
    // Write to file to see if main() even runs
    FILE* debug_log = fopen("C:\\dev\\VoIP-System\\client\\debug.txt", "w");
    if (debug_log) {
        fprintf(debug_log, "=== main() started ===\n");
        fflush(debug_log);
    }
    
    // Force console output
    freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
    
    std::cout << "=== VoIP Client Starting ===" << std::endl;
    std::cout << "Creating QApplication..." << std::endl;
    std::cout.flush();
    
    if (debug_log) {
        fprintf(debug_log, "About to create QApplication\n");
        fflush(debug_log);
    }
    
    QApplication app(argc, argv);
    
    if (debug_log) {
        fprintf(debug_log, "QApplication created!\n");
        fflush(debug_log);
        fclose(debug_log);
    }
    
    std::cout << "QApplication created successfully!" << std::endl;
    std::cout.flush();
    
    // Set application info
    QApplication::setApplicationName("VoIP Client");
    QApplication::setApplicationVersion("0.1.0");
    
    std::cout << "Application info set" << std::endl;
    std::cout.flush();
    
    // Load dark theme
    QFile styleFile(":/styles/dark_theme.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QString::fromLatin1(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "Dark theme loaded successfully!";
    } else {
        qWarning() << "Failed to load dark theme from resources, using inline fallback";
        
        // Fallback inline stylesheet
        QString fallbackStyle = R"(
            QWidget {
                background-color: #2b2d31;
                color: #dcddde;
                font-family: "Segoe UI", Arial, sans-serif;
                font-size: 10pt;
            }
            QDialog {
                background-color: #2b2d31;
                color: #dcddde;
            }
            QGroupBox {
                background-color: #1e1f22;
                border: 2px solid #3e4045;
                border-radius: 8px;
                margin-top: 16px;
                padding: 16px 12px 12px 12px;
                font-weight: bold;
                color: #b5bac1;
            }
            QLineEdit {
                background-color: #1e1f22;
                border: 2px solid #3e4045;
                border-radius: 6px;
                color: #dcddde;
                padding: 12px 16px;
                min-height: 20px;
                font-size: 11pt;
            }
            QLineEdit:focus {
                border: 2px solid #5865f2;
                background-color: #2b2d31;
            }
            QPushButton {
                background-color: #5865f2;
                color: #ffffff;
                border: none;
                border-radius: 6px;
                padding: 10px 20px;
                font-weight: bold;
                min-height: 16px;
            }
            QPushButton:hover {
                background-color: #4752c4;
            }
            QCheckBox {
                color: #dcddde;
            }
            QLabel {
                color: #dcddde;
                background: transparent;
            }
        )";
        app.setStyleSheet(fallbackStyle);
        qDebug() << "Fallback dark theme applied";
    }
    
    std::cout << "About to create LoginDialog..." << std::endl;
    std::cout.flush();
    
    // Login dialog
    ui::LoginDialog loginDialog;
    
    std::cout << "LoginDialog created!" << std::endl;
    std::cout.flush();
    
    // Handle login button - need to actually authenticate
    static std::shared_ptr<network::WebSocketClient> loginWs;
    static bool loginInProgress = false;
    
    QObject::connect(&loginDialog, &ui::LoginDialog::loginRequested, [&](const QString& username, const QString& password, const QString& server) {
        if (username.isEmpty()) {
            loginDialog.setStatusMessage("Please enter a username", true);
            return;
        }
        
        if (loginInProgress) {
            loginDialog.setStatusMessage("Login already in progress...", false);
            return;
        }
        
        loginInProgress = true;
        
        // Parse server address and port
        QStringList parts = server.split(":");
        std::string serverAddr = parts[0].toStdString();
        uint16_t serverPort = parts.size() > 1 ? parts[1].toUShort() : 9000;
        
        // Create temporary WebSocket client for authentication
        loginWs = std::make_shared<network::WebSocketClient>();
        
        // Set up login callback
        loginWs->set_login_callback([&loginDialog](const protocol::LoginResponse& response) {
            loginInProgress = false;
            
            if (response.success) {
                std::cout << "✅ Authentication successful! User ID: " << response.user_id << std::endl;
                QMetaObject::invokeMethod(&loginDialog, [&loginDialog]() {
                    loginDialog.setStatusMessage("Login successful!", false);
                    loginDialog.accept();
                }, Qt::QueuedConnection);
            } else {
                std::string message = response.error_message.empty() ? "Authentication failed" : response.error_message;
                std::cerr << "❌ Authentication failed: " << message << std::endl;
                QMetaObject::invokeMethod(&loginDialog, [&loginDialog, message]() {
                    loginDialog.setStatusMessage(QString::fromStdString(message), true);
                    QMessageBox::warning(&loginDialog, "Login Failed", 
                        QString("Authentication failed: %1").arg(QString::fromStdString(message)));
                }, Qt::QueuedConnection);
            }
            
            // Disconnect after handling response
            QTimer::singleShot(100, []() {
                if (loginWs) {
                    loginWs->disconnect();
                    loginWs.reset();
                }
            });
        });
        
        // Connect and authenticate
        loginDialog.setStatusMessage("Connecting to server...", false);
        bool useTls = loginDialog.useTls();
        auto connectResult = loginWs->connect(serverAddr, serverPort, useTls);
        if (!connectResult.is_ok()) {
            std::cerr << "❌ Failed to connect: " << connectResult.error().message() << std::endl;
            loginDialog.setStatusMessage("Failed to connect to server", true);
            loginInProgress = false;
            loginWs.reset();
            return;
        }
        
        // Wait a moment for connection, then send authentication
        QTimer::singleShot(500, [username, password]() {
            std::cout << "Sending authentication request..." << std::endl;
            if (loginWs) {
                loginWs->login(username.toStdString(), password.toStdString(), "");
            }
        });
    });
    
    // Handle register button - need to keep WebSocket alive
    static std::shared_ptr<network::WebSocketClient> registrationWs;
    
    QObject::connect(&loginDialog, &ui::LoginDialog::registerRequested, [&](const QString& username, const QString& password, const QString& server) {
        std::cout << "📝 Registration requested for user: " << username.toStdString() << std::endl;
        
        // Parse server address and port
        QStringList parts = server.split(":");
        std::string serverAddr = parts[0].toStdString();
        uint16_t serverPort = parts.size() > 1 ? parts[1].toUShort() : 9000;
        
        // Create WebSocket client for registration (keep it alive as static)
        registrationWs = std::make_shared<network::WebSocketClient>();
        
        // Set up registration callback
        registrationWs->set_register_callback([&loginDialog](bool success, const std::string& message, uint32_t user_id) {
            if (success) {
                std::cout << "✅ Registration successful! User ID: " << user_id << std::endl;
                QMetaObject::invokeMethod(&loginDialog, [&loginDialog, message]() {
                    QMessageBox::information(&loginDialog, "Registration Successful", 
                        QString::fromStdString(message));
                    loginDialog.setStatusMessage("Registration successful! Now you can login.", false);
                }, Qt::QueuedConnection);
            } else {
                std::cerr << "❌ Registration failed: " << message << std::endl;
                QMetaObject::invokeMethod(&loginDialog, [&loginDialog, message]() {
                    QMessageBox::warning(&loginDialog, "Registration Failed", 
                        QString::fromStdString(message));
                    loginDialog.setStatusMessage(QString::fromStdString(message), true);
                }, Qt::QueuedConnection);
            }
            
            // Disconnect after handling response
            QTimer::singleShot(100, []() {
                if (registrationWs) {
                    registrationWs->disconnect();
                    registrationWs.reset();
                }
            });
        });
        
        // Connect and register
        loginDialog.setStatusMessage("Connecting to server...", false);
        auto connectResult = registrationWs->connect(serverAddr, serverPort, false);
        if (!connectResult.is_ok()) {
            std::cerr << "❌ Failed to connect: " << connectResult.error().message() << std::endl;
            loginDialog.setStatusMessage("Failed to connect to server", true);
            registrationWs.reset();
            return;
        }
        
        // Wait a moment for connection, then send registration
        QTimer::singleShot(500, [username, password]() {
            std::cout << "Sending registration request..." << std::endl;
            if (registrationWs) {
                registrationWs->register_account(username.toStdString(), password.toStdString());
            }
        });
    });
    
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }
    
    // Create main window
    auto* mainWindow = new ui::MainWindow();
    mainWindow->setUserInfo(loginDialog.username(), 42);
    mainWindow->setAttribute(Qt::WA_DeleteOnClose);
    
    // Create and connect WebSocket client for control messages
    std::cout << "\n=== CREATING WEBSOCKET CLIENT ===" << std::endl;
    auto wsClient = std::make_shared<network::WebSocketClient>();
    std::cout << "WebSocket client created" << std::endl;
    
    // Set login credentials in MainWindow (it will handle login after connection)
    std::string username = loginDialog.username().toStdString();
    std::string password = loginDialog.password().toStdString();
    std::string serverAddr = loginDialog.serverAddress().toStdString();

    mainWindow->setLoginCredentials(QString::fromStdString(username),
                                    QString::fromStdString(password));

    // Pass server info to MainWindow for admin API configuration
    uint16_t serverPort = loginDialog.serverPort();
    bool useTls = loginDialog.useTls();
    mainWindow->setServerInfo(loginDialog.serverAddress(), serverPort, useTls);
    std::cout << "Server info passed to MainWindow: " << serverAddr << ":" << serverPort
              << " (TLS: " << (useTls ? "enabled" : "disabled") << ")" << std::endl;

    mainWindow->setWebSocketClient(wsClient);  // This sets up all callbacks
    std::cout << "WebSocket client set on main window (callbacks configured)" << std::endl;

    // Connect to WebSocket server (async, will trigger callbacks)
    std::string protocol = useTls ? "wss" : "ws";
    std::cout << "Connecting to WebSocket: " << protocol << "://" << serverAddr << ":" << serverPort << std::endl;
    auto connectResult = wsClient->connect(serverAddr, serverPort, useTls);
    if (!connectResult.is_ok()) {
        std::cout << "WebSocket connect FAILED: " << connectResult.error().message() << std::endl;
    } else {
        std::cout << "WebSocket connect() initiated (async)" << std::endl;
    }
    
    std::cout << "=== WEBSOCKET SETUP COMPLETE (waiting for connection...) ===\n" << std::endl;
    
    // Setup voice session
    session::VoiceSession::Config voiceConfig;
    voiceConfig.server_address = loginDialog.serverAddress().toStdString();
    voiceConfig.server_port = 9001;
    voiceConfig.sample_rate = 48000;
    voiceConfig.frame_size = 960;
    voiceConfig.channels = 1;
    voiceConfig.bitrate = 32000;
    voiceConfig.enable_fec = true;
    voiceConfig.enable_dtx = false;
    voiceConfig.channel_id = 1;
    voiceConfig.user_id = 42;
    voiceConfig.jitter_buffer_frames = 5;
    
    // Create and initialize voice session (but DON'T start yet!)
    // We'll start it AFTER WebSocket connects and joins channel
    auto voiceSession = std::make_shared<session::VoiceSession>();
    auto initResult = voiceSession->initialize(voiceConfig);
    
    if (initResult.is_ok()) {
        // Pass to MainWindow - it will start after channel join
        mainWindow->setVoiceSession(voiceSession);
        std::cout << "✅ Voice session initialized (not started yet)" << std::endl;
    } else {
        std::cerr << "❌ Voice session initialization failed!" << std::endl;
    }
    
    mainWindow->show();
    
    // Run application event loop
    int result = app.exec();
    
    std::cout << "\n=== APPLICATION EXITING ===" << std::endl;
    
    // Cleanup voice session explicitly before MainWindow destructor
    // This ensures proper shutdown order
    if (voiceSession) {
        std::cout << "🧹 Explicit cleanup of voice session..." << std::endl;
        voiceSession->stop();
        voiceSession->shutdown();
        voiceSession.reset();  // Release shared_ptr
    }
    
    // Delete main window explicitly to see destructor output
    std::cout << "🧹 Deleting main window..." << std::endl;
    delete mainWindow;
    mainWindow = nullptr;
    
    std::cout << "✅ Application cleanup complete\n" << std::endl;
    
    return result;
}
