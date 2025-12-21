#include "ui/hotkey_input_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeySequence>

namespace voip {
namespace ui {

HotkeyInputDialog::HotkeyInputDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Capture Hotkey");
    setMinimumWidth(350);
    setModal(true);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(20, 20, 20, 20);

    // Instructions
    instructionLabel_ = new QLabel("Press any key to set as hotkey:", this);
    instructionLabel_->setStyleSheet("font-size: 11pt; color: #dcddde;");
    layout->addWidget(instructionLabel_);

    // Captured key display
    capturedLabel_ = new QLabel("[None]", this);
    capturedLabel_->setAlignment(Qt::AlignCenter);
    capturedLabel_->setStyleSheet(
        "background-color: #1e1f22;"
        "border: 2px solid #5865f2;"
        "border-radius: 8px;"
        "padding: 20px;"
        "font-size: 18pt;"
        "font-weight: bold;"
        "color: #ffffff;"
        "min-height: 60px;"
    );
    layout->addWidget(capturedLabel_);

    // Hint label
    auto* hintLabel = new QLabel("Supported keys: F1-F12, 0-9, A-Z, Space, etc.", this);
    hintLabel->setStyleSheet("font-size: 9pt; color: #72767d;");
    hintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(hintLabel);

    layout->addSpacing(10);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();

    clearButton_ = new QPushButton("Clear", this);
    clearButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4f545c;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #5d6269;"
        "}"
    );
    connect(clearButton_, &QPushButton::clicked, this, [this]() {
        capturedKey_ = QKeySequence();
        updateDisplay();
    });

    cancelButton_ = new QPushButton("Cancel", this);
    cancelButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4f545c;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #5d6269;"
        "}"
    );
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);

    okButton_ = new QPushButton("OK", this);
    okButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #5865f2;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4752c4;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #2f3136;"
        "  color: #72767d;"
        "}"
    );
    okButton_->setEnabled(false);
    connect(okButton_, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(clearButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(okButton_);

    layout->addLayout(buttonLayout);

    // Set focus to dialog so it captures key events
    setFocus();
}

void HotkeyInputDialog::keyPressEvent(QKeyEvent* event) {
    // Ignore modifier-only keys
    if (event->key() == Qt::Key_Control ||
        event->key() == Qt::Key_Shift ||
        event->key() == Qt::Key_Alt ||
        event->key() == Qt::Key_Meta) {
        QDialog::keyPressEvent(event);
        return;
    }

    // Ignore Escape (let it cancel the dialog)
    if (event->key() == Qt::Key_Escape) {
        QDialog::keyPressEvent(event);
        return;
    }

    // Ignore Enter/Return (let it confirm)
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (okButton_->isEnabled()) {
            accept();
        }
        return;
    }

    // Capture the key
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    // Keep KeypadModifier for numpad key differentiation, remove others we don't want
    modifiers &= (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier);

    // Create key sequence
    capturedKey_ = QKeySequence(key | modifiers);

    updateDisplay();
    event->accept();
}

void HotkeyInputDialog::updateDisplay() {
    if (capturedKey_.isEmpty()) {
        capturedLabel_->setText("[None]");
        capturedLabel_->setStyleSheet(
            "background-color: #1e1f22;"
            "border: 2px solid #72767d;"
            "border-radius: 8px;"
            "padding: 20px;"
            "font-size: 18pt;"
            "font-weight: bold;"
            "color: #72767d;"
            "min-height: 60px;"
        );
        okButton_->setEnabled(false);
    } else {
        capturedLabel_->setText(capturedKey_.toString());
        capturedLabel_->setStyleSheet(
            "background-color: #1e1f22;"
            "border: 2px solid #5865f2;"
            "border-radius: 8px;"
            "padding: 20px;"
            "font-size: 18pt;"
            "font-weight: bold;"
            "color: #5865f2;"
            "min-height: 60px;"
        );
        okButton_->setEnabled(true);
    }
}

} // namespace ui
} // namespace voip
