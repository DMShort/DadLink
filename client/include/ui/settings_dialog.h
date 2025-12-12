#pragma once

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include "audio/audio_engine.h"
#include "common/types.h"

namespace voip::ui {

/**
 * SettingsDialog - Audio device and volume configuration
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(audio::AudioEngine* audioEngine, QWidget* parent = nullptr);
    
    // Get selected device IDs
    [[nodiscard]] DeviceId selectedInputDevice() const;
    [[nodiscard]] DeviceId selectedOutputDevice() const;
    
    // Get volume settings (0.0 - 2.0)
    [[nodiscard]] float inputVolume() const;
    [[nodiscard]] float outputVolume() const;
    
private slots:
    void onInputDeviceChanged(int index);
    void onOutputDeviceChanged(int index);
    void onInputVolumeChanged(int value);
    void onOutputVolumeChanged(int value);
    void onTestInput();
    void onTestOutput();
    void onApply();
    void onCancel();
    
private:
    void setupUI();
    void loadDevices();
    void applyDarkTheme();
    
    // Audio engine reference
    audio::AudioEngine* audioEngine_;
    
    // Device selection
    QComboBox* inputDeviceCombo_;
    QComboBox* outputDeviceCombo_;
    
    // Volume controls
    QSlider* inputVolumeSlider_;
    QSlider* outputVolumeSlider_;
    QLabel* inputVolumeLabel_;
    QLabel* outputVolumeLabel_;
    
    // Buttons
    QPushButton* testInputButton_;
    QPushButton* testOutputButton_;
    QPushButton* applyButton_;
    QPushButton* cancelButton_;
    
    // Device lists
    std::vector<AudioDevice> inputDevices_;
    std::vector<AudioDevice> outputDevices_;
    
    // Current selections
    DeviceId selectedInputId_;
    DeviceId selectedOutputId_;
};

} // namespace voip::ui
