#pragma once

#include <QDialog>
#include <QLabel>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPushButton>

namespace voip {
namespace ui {

/**
 * @brief Dialog for capturing a hotkey press
 *
 * This dialog waits for the user to press a key and captures it.
 * Much better UX than typing key names as text.
 */
class HotkeyInputDialog : public QDialog {
    Q_OBJECT

public:
    explicit HotkeyInputDialog(QWidget* parent = nullptr);

    /**
     * @brief Get the captured key
     * @return The key sequence that was pressed, or empty if canceled
     */
    QKeySequence capturedKey() const { return capturedKey_; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QLabel* instructionLabel_;
    QLabel* capturedLabel_;
    QPushButton* clearButton_;
    QPushButton* okButton_;
    QPushButton* cancelButton_;

    QKeySequence capturedKey_;

    void updateDisplay();
};

} // namespace ui
} // namespace voip
