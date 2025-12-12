#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>

namespace voip::api {
    class AdminApiClient;
}

namespace voip::ui::admin {

/**
 * @brief Widget for managing roles and permissions
 */
class RoleManager : public QWidget {
    Q_OBJECT

public:
    explicit RoleManager(QWidget* parent = nullptr);
    
    void setApiClient(std::shared_ptr<voip::api::AdminApiClient> client);
    void refresh();

signals:
    void statusMessage(const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onCreateRole();
    void onEditRole();
    void onDeleteRole();
    void onRefreshClicked();
    void onSelectionChanged();
    void onRoleDoubleClicked(int row, int column);

private:
    void setupUi();
    void loadRoles();
    void onRolesLoaded(const QJsonArray& roles);
    void populateRoleTable(const QJsonArray& roles);
    void updateButtonStates();
    int getSelectedRoleId() const;
    QString permissionsToString(uint32_t permissions) const;
    
    // UI Components
    QTableWidget* role_table_;
    QLineEdit* search_box_;
    QPushButton* create_btn_;
    QPushButton* edit_btn_;
    QPushButton* delete_btn_;
    QPushButton* refresh_btn_;
    
    // Data
    std::shared_ptr<voip::api::AdminApiClient> apiClient_;
    QJsonArray all_roles_;
};

} // namespace voip::ui::admin
