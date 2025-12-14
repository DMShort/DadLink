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
 * @brief Dialog for creating a new channel
 */
class CreateChannelDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateChannelDialog(QWidget* parent = nullptr);

    /**
     * @brief Get the channel data from the form
     * @return JSON object with channel creation data
     */
    QJsonObject getChannelData() const;

private slots:
    void onAccept();
    void onNameChanged(const QString& text);

private:
    void setupUI();
    bool validateForm();

    // UI Components
    QLineEdit* name_edit_;
    QTextEdit* description_edit_;
    QSpinBox* max_users_spin_;
    QCheckBox* persistent_checkbox_;

    QPushButton* create_btn_;
    QPushButton* cancel_btn_;
};

} // namespace voip::ui::admin
