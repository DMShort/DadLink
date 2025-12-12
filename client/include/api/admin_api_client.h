#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <memory>

namespace voip::api {

/**
 * @brief HTTP API client for admin operations
 * 
 * Provides async methods for all admin API endpoints.
 * Uses Qt's signal/slot mechanism for async responses.
 */
class AdminApiClient : public QObject {
    Q_OBJECT

public:
    explicit AdminApiClient(QObject* parent = nullptr);
    ~AdminApiClient() override;

    /**
     * @brief Set the base URL for API requests
     * @param url Base URL (e.g., "http://localhost:9000")
     */
    void setBaseUrl(const QString& url);
    
    /**
     * @brief Set JWT authentication token
     * @param token JWT bearer token
     */
    void setAuthToken(const QString& token);
    
    // Organizations
    void getOrganizations(std::function<void(const QJsonArray&)> callback);
    void getOrganization(int id, std::function<void(const QJsonObject&)> callback);
    void createOrganization(const QJsonObject& org, std::function<void(int)> callback);
    void updateOrganization(int id, const QJsonObject& org, std::function<void(bool)> callback);
    void deleteOrganization(int id, std::function<void(bool)> callback);
    
    // Users
    void getUsers(std::function<void(const QJsonArray&)> callback);
    void getUser(int id, std::function<void(const QJsonObject&)> callback);
    void createUser(const QJsonObject& user, std::function<void(int)> callback);
    void updateUser(int id, const QJsonObject& user, std::function<void(bool)> callback);
    void deleteUser(int id, std::function<void(bool)> callback);
    void banUser(int id, std::function<void(bool)> callback);
    void unbanUser(int id, std::function<void(bool)> callback);
    void resetPassword(int id, const QString& new_password, std::function<void(bool)> callback);
    
    // Channels
    void getChannels(std::function<void(const QJsonArray&)> callback);
    void getChannel(int id, std::function<void(const QJsonObject&)> callback);
    void createChannel(const QJsonObject& channel, std::function<void(int)> callback);
    void updateChannel(int id, const QJsonObject& channel, std::function<void(bool)> callback);
    void deleteChannel(int id, std::function<void(bool)> callback);
    
    // Roles
    void getRoles(std::function<void(const QJsonArray&)> callback);
    void getRole(int id, std::function<void(const QJsonObject&)> callback);
    void createRole(const QJsonObject& role, std::function<void(int)> callback);
    void updateRole(int id, const QJsonObject& role, std::function<void(bool)> callback);
    void deleteRole(int id, std::function<void(bool)> callback);
    
    // Metrics
    void getSystemMetrics(std::function<void(const QJsonObject&)> callback);
    void getUserMetrics(std::function<void(const QJsonObject&)> callback);
    void getVoiceMetrics(std::function<void(const QJsonObject&)> callback);
    
    // Audit Log
    void getAuditLog(int limit, int offset, std::function<void(const QJsonArray&)> callback);
    void searchAuditLog(const QJsonObject& filters, std::function<void(const QJsonArray&)> callback);

signals:
    void requestStarted();
    void requestFinished();
    void error(const QString& message, int status_code);

private:
    void handleReply(QNetworkReply* reply, std::function<void(const QJsonDocument&)> callback);
    QNetworkRequest createRequest(const QString& endpoint);
    void sendGetRequest(const QString& endpoint, std::function<void(const QJsonDocument&)> callback);
    void sendPostRequest(const QString& endpoint, const QJsonObject& data, 
                        std::function<void(const QJsonDocument&)> callback);
    void sendPutRequest(const QString& endpoint, const QJsonObject& data,
                       std::function<void(const QJsonDocument&)> callback);
    void sendDeleteRequest(const QString& endpoint, std::function<void(const QJsonDocument&)> callback);

private:
    QNetworkAccessManager* network_;
    QString base_url_;
    QString auth_token_;
    int pending_requests_ = 0;
};

} // namespace voip::api
