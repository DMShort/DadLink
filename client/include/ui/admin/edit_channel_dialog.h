#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QJsonObject>

namespace voip::ui::admin {

/**
 * @brief Dialog for editing an existing channel
 */
class EditChannelDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditChannelDialog(const QJsonObject& channel, QWidget* parent = nullptr);

    /**
     * @brief Get the updated channel data from the form
     * @return JSON object with channel update data (only changed fields)
     */
    QJsonObject getUpdatedData() const;

private slots:
    void onAccept();

private:
    void setupUI();
    void loadChannelData();
    bool validateForm();

    // Original channel data
    QJsonObject original_channel_;

    // UI Components
    QLineEdit* name_edit_;
    QTextEdit* description_edit_;
    QSpinBox* max_users_spin_;
    QCheckBox* persistent_checkbox_;

    QPushButton* save_btn_;
    QPushButton* cancel_btn_;
};

} // namespace voip::ui::admin
