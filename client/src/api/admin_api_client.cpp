#include "api/admin_api_client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>

namespace voip::api {

AdminApiClient::AdminApiClient(QObject* parent)
    : QObject(parent)
    , network_(new QNetworkAccessManager(this))
    , base_url_("https://localhost:9000")
{
    // Ignore SSL errors for self-signed certificates in development
    connect(network_, &QNetworkAccessManager::sslErrors,
            [](QNetworkReply* reply, const QList<QSslError>& errors) {
                std::cout << "⚠️  Ignoring SSL errors for development:" << std::endl;
                for (const auto& error : errors) {
                    std::cout << "   - " << error.errorString().toStdString() << std::endl;
                }
                reply->ignoreSslErrors();
            });

    std::cout << "📡 AdminApiClient created" << std::endl;
}

AdminApiClient::~AdminApiClient() {
    std::cout << "📡 AdminApiClient destroyed" << std::endl;
}

void AdminApiClient::setBaseUrl(const QString& url) {
    base_url_ = url;
    std::cout << "📡 API base URL set to: " << url.toStdString() << std::endl;
}

void AdminApiClient::setAuthToken(const QString& token) {
    auth_token_ = token;
    std::cout << "📡 Auth token set (length: " << token.length() << ")" << std::endl;
}

QNetworkRequest AdminApiClient::createRequest(const QString& endpoint) {
    QUrl url(base_url_ + endpoint);
    QNetworkRequest request(url);
    
    // Set headers
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Add auth token if available
    if (!auth_token_.isEmpty()) {
        QString auth_header = "Bearer " + auth_token_;
        request.setRawHeader("Authorization", auth_header.toUtf8());
    }
    
    return request;
}

void AdminApiClient::handleReply(QNetworkReply* reply, 
                                  std::function<void(const QJsonDocument&)> callback) {
    reply->deleteLater();
    pending_requests_--;
    
    if (pending_requests_ == 0) {
        emit requestFinished();
    }
    
    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        QString error_msg = reply->errorString();
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        std::cerr << "❌ API Error: " << error_msg.toStdString() 
                  << " (Status: " << status_code << ")" << std::endl;
        
        emit error(error_msg, status_code);
        return;
    }
    
    // Parse response
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() && !data.isEmpty()) {
        std::cerr << "❌ Failed to parse JSON response" << std::endl;
        emit error("Invalid JSON response", 0);
        return;
    }
    
    // Call success callback
    if (callback) {
        callback(doc);
    }
}

void AdminApiClient::sendGetRequest(const QString& endpoint,
                                     std::function<void(const QJsonDocument&)> callback) {
    pending_requests_++;
    emit requestStarted();
    
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = network_->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        handleReply(reply, callback);
    });
}

void AdminApiClient::sendPostRequest(const QString& endpoint, const QJsonObject& data,
                                      std::function<void(const QJsonDocument&)> callback) {
    pending_requests_++;
    emit requestStarted();
    
    QNetworkRequest request = createRequest(endpoint);
    QJsonDocument doc(data);
    QByteArray json_data = doc.toJson();
    
    QNetworkReply* reply = network_->post(request, json_data);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        handleReply(reply, callback);
    });
}

void AdminApiClient::sendPutRequest(const QString& endpoint, const QJsonObject& data,
                                     std::function<void(const QJsonDocument&)> callback) {
    pending_requests_++;
    emit requestStarted();
    
    QNetworkRequest request = createRequest(endpoint);
    QJsonDocument doc(data);
    QByteArray json_data = doc.toJson();
    
    QNetworkReply* reply = network_->put(request, json_data);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        handleReply(reply, callback);
    });
}

void AdminApiClient::sendDeleteRequest(const QString& endpoint,
                                        std::function<void(const QJsonDocument&)> callback) {
    pending_requests_++;
    emit requestStarted();
    
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = network_->deleteResource(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        handleReply(reply, callback);
    });
}

// ==================== Organizations ====================

void AdminApiClient::getOrganizations(std::function<void(const QJsonArray&)> callback) {
    sendGetRequest("/api/admin/organizations", [callback](const QJsonDocument& doc) {
        if (callback && doc.isArray()) {
            callback(doc.array());
        }
    });
}

void AdminApiClient::getOrganization(int id, std::function<void(const QJsonObject&)> callback) {
    QString endpoint = QString("/api/admin/organizations/%1").arg(id);
    sendGetRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

void AdminApiClient::createOrganization(const QJsonObject& org, std::function<void(int)> callback) {
    sendPostRequest("/api/admin/organizations", org, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            int id = doc.object()["id"].toInt();
            callback(id);
        }
    });
}

void AdminApiClient::updateOrganization(int id, const QJsonObject& org, 
                                         std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/organizations/%1").arg(id);
    sendPutRequest(endpoint, org, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::deleteOrganization(int id, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/organizations/%1").arg(id);
    sendDeleteRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

// ==================== Users ====================

void AdminApiClient::getUsers(std::function<void(const QJsonArray&)> callback) {
    sendGetRequest("/api/admin/users", [callback](const QJsonDocument& doc) {
        if (callback && doc.isArray()) {
            callback(doc.array());
        }
    });
}

void AdminApiClient::getUser(int id, std::function<void(const QJsonObject&)> callback) {
    QString endpoint = QString("/api/admin/users/%1").arg(id);
    sendGetRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

void AdminApiClient::createUser(const QJsonObject& user, std::function<void(int)> callback) {
    sendPostRequest("/api/admin/users", user, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            int id = doc.object()["id"].toInt();
            callback(id);
        }
    });
}

void AdminApiClient::updateUser(int id, const QJsonObject& user, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/users/%1").arg(id);
    sendPutRequest(endpoint, user, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::deleteUser(int id, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/users/%1").arg(id);
    sendDeleteRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::banUser(int id, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/users/%1/ban").arg(id);
    sendPostRequest(endpoint, QJsonObject(), [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::unbanUser(int id, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/users/%1/unban").arg(id);
    sendPostRequest(endpoint, QJsonObject(), [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::resetPassword(int id, const QString& new_password, 
                                    std::function<void(bool)> callback) {
    QJsonObject data;
    data["password"] = new_password;
    
    QString endpoint = QString("/api/admin/users/%1/reset-password").arg(id);
    sendPostRequest(endpoint, data, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

// ==================== Channels ====================

void AdminApiClient::getChannels(std::function<void(const QJsonArray&)> callback) {
    sendGetRequest("/api/admin/channels", [callback](const QJsonDocument& doc) {
        if (callback && doc.isArray()) {
            callback(doc.array());
        }
    });
}

void AdminApiClient::getChannel(int id, std::function<void(const QJsonObject&)> callback) {
    QString endpoint = QString("/api/admin/channels/%1").arg(id);
    sendGetRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

void AdminApiClient::createChannel(const QJsonObject& channel, std::function<void(int)> callback) {
    sendPostRequest("/api/admin/channels", channel, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            int id = doc.object()["id"].toInt();
            callback(id);
        }
    });
}

void AdminApiClient::updateChannel(int id, const QJsonObject& channel, 
                                    std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/channels/%1").arg(id);
    sendPutRequest(endpoint, channel, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::deleteChannel(int id, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/channels/%1").arg(id);
    sendDeleteRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

// ==================== Roles ====================

void AdminApiClient::getRoles(std::function<void(const QJsonArray&)> callback) {
    sendGetRequest("/api/admin/roles", [callback](const QJsonDocument& doc) {
        if (callback && doc.isArray()) {
            callback(doc.array());
        }
    });
}

void AdminApiClient::getRole(int id, std::function<void(const QJsonObject&)> callback) {
    QString endpoint = QString("/api/admin/roles/%1").arg(id);
    sendGetRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

void AdminApiClient::createRole(const QJsonObject& role, std::function<void(int)> callback) {
    sendPostRequest("/api/admin/roles", role, [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            int id = doc.object()["id"].toInt();
            callback(id);
        }
    });
}

void AdminApiClient::updateRole(int id, const QJsonObject& role, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/roles/%1").arg(id);
    sendPutRequest(endpoint, role, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

void AdminApiClient::deleteRole(int id, std::function<void(bool)> callback) {
    QString endpoint = QString("/api/admin/roles/%1").arg(id);
    sendDeleteRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback) {
            callback(doc.object()["success"].toBool());
        }
    });
}

// ==================== Metrics ====================

void AdminApiClient::getSystemMetrics(std::function<void(const QJsonObject&)> callback) {
    sendGetRequest("/api/admin/metrics/system", [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

void AdminApiClient::getUserMetrics(std::function<void(const QJsonObject&)> callback) {
    sendGetRequest("/api/admin/metrics/users", [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

void AdminApiClient::getVoiceMetrics(std::function<void(const QJsonObject&)> callback) {
    sendGetRequest("/api/admin/metrics/voice", [callback](const QJsonDocument& doc) {
        if (callback && doc.isObject()) {
            callback(doc.object());
        }
    });
}

// ==================== Audit Log ====================

void AdminApiClient::getAuditLog(int limit, int offset, 
                                  std::function<void(const QJsonArray&)> callback) {
    QString endpoint = QString("/api/admin/audit-log?limit=%1&offset=%2").arg(limit).arg(offset);
    sendGetRequest(endpoint, [callback](const QJsonDocument& doc) {
        if (callback && doc.isArray()) {
            callback(doc.array());
        }
    });
}

void AdminApiClient::searchAuditLog(const QJsonObject& filters,
                                     std::function<void(const QJsonArray&)> callback) {
    sendPostRequest("/api/admin/audit-log/search", filters, [callback](const QJsonDocument& doc) {
        if (callback && doc.isArray()) {
            callback(doc.array());
        }
    });
}

} // namespace voip::api
