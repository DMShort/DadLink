#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>

namespace voip::api {
    class AdminApiClient;
}

namespace voip::ui::admin {

/**
 * @brief User management widget for CRUD operations on users
 */
class UserManager : public QWidget {
    Q_OBJECT

public:
    explicit UserManager(QWidget* parent = nullptr);
    ~UserManager() override = default;

    void setApiClient(std::shared_ptr<voip::api::AdminApiClient> client);
    void refresh();

signals:
    void statusMessage(const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onCreateUser();
    void onEditUser();
    void onDeleteUser();
    void onBanUser();
    void onUnbanUser();
    void onResetPassword();
    void onSearchChanged(const QString& text);
    void onFilterChanged(int index);
    void onUserDoubleClicked(int row, int column);
    void onRefreshClicked();
    void onSelectionChanged();
    
    // API response handlers
    void onUsersLoaded(const QJsonArray& users);
    void onUserCreated(int user_id);
    void onUserUpdated();
    void onUserDeleted();

private:
    void setupUi();
    void loadUsers();
    void filterUsers();
    void updateButtonStates();
    void showUserDialog(int user_id = -1);
    void populateUserTable(const QJsonArray& users);
    int getSelectedUserId() const;

private:
    std::shared_ptr<voip::api::AdminApiClient> apiClient_;
    
    // UI Components
    QLineEdit* search_box_;
    QComboBox* filter_combo_;
    QTableWidget* user_table_;
    
    QPushButton* create_btn_;
    QPushButton* edit_btn_;
    QPushButton* delete_btn_;
    QPushButton* ban_btn_;
    QPushButton* unban_btn_;
    QPushButton* reset_pass_btn_;
    QPushButton* refresh_btn_;
    
    // Cached data
    QJsonArray all_users_;
    QJsonArray filtered_users_;
};

} // namespace voip::ui::admin
