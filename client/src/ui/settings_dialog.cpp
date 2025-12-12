#include "ui/settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

namespace voip::ui {

SettingsDialog::SettingsDialog(audio::AudioEngine* audioEngine, QWidget* parent)
    : QDialog(parent)
    , audioEngine_(audioEngine)
    , selectedInputId_(paNoDevice)
    , selectedOutputId_(paNoDevice)
{
    setupUI();
    loadDevices();
    applyDarkTheme();
}

void SettingsDialog::setupUI() {
    setWindowTitle("Audio Settings");
    setModal(true);
    setMinimumWidth(500);
    setMinimumHeight(400);
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // === Audio Devices Group ===
    auto* devicesGroup = new QGroupBox("Audio Devices", this);
    auto* devicesLayout = new QFormLayout(devicesGroup);
    devicesLayout->setSpacing(10);
    devicesLayout->setContentsMargins(15, 15, 15, 15);
    
    // Input device
    inputDeviceCombo_ = new QComboBox(devicesGroup);
    inputDeviceCombo_->setMinimumWidth(300);
    connect(inputDeviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onInputDeviceChanged);
    
    auto* inputRow = new QHBoxLayout();
    inputRow->addWidget(inputDeviceCombo_);
    testInputButton_ = new QPushButton("🎤 Test", devicesGroup);
    testInputButton_->setFixedWidth(80);
    connect(testInputButton_, &QPushButton::clicked, this, &SettingsDialog::onTestInput);
    inputRow->addWidget(testInputButton_);
    
    auto* inputLabel = new QLabel("Input Device:", devicesGroup);
    inputLabel->setFixedWidth(110);
    devicesLayout->addRow(inputLabel, inputRow);
    
    // Output device
    outputDeviceCombo_ = new QComboBox(devicesGroup);
    outputDeviceCombo_->setMinimumWidth(300);
    connect(outputDeviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onOutputDeviceChanged);
    
    auto* outputRow = new QHBoxLayout();
    outputRow->addWidget(outputDeviceCombo_);
    testOutputButton_ = new QPushButton("🔊 Test", devicesGroup);
    testOutputButton_->setFixedWidth(80);
    connect(testOutputButton_, &QPushButton::clicked, this, &SettingsDialog::onTestOutput);
    outputRow->addWidget(testOutputButton_);
    
    auto* outputLabel = new QLabel("Output Device:", devicesGroup);
    outputLabel->setFixedWidth(110);
    devicesLayout->addRow(outputLabel, outputRow);
    
    mainLayout->addWidget(devicesGroup);
    
    // === Volume Controls Group ===
    auto* volumeGroup = new QGroupBox("Volume", this);
    auto* volumeLayout = new QFormLayout(volumeGroup);
    volumeLayout->setSpacing(10);
    volumeLayout->setContentsMargins(15, 15, 15, 15);
    
    // Input volume
    auto* inputVolRow = new QHBoxLayout();
    inputVolumeSlider_ = new QSlider(Qt::Horizontal, volumeGroup);
    inputVolumeSlider_->setRange(0, 200);  // 0% to 200%
    inputVolumeSlider_->setValue(100);     // Default 100%
    inputVolumeSlider_->setMinimumWidth(250);
    connect(inputVolumeSlider_, &QSlider::valueChanged,
            this, &SettingsDialog::onInputVolumeChanged);
    
    inputVolumeLabel_ = new QLabel("100%", volumeGroup);
    inputVolumeLabel_->setFixedWidth(50);
    inputVolumeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    inputVolRow->addWidget(inputVolumeSlider_);
    inputVolRow->addWidget(inputVolumeLabel_);
    
    auto* inputVolLabel = new QLabel("Input Volume:", volumeGroup);
    inputVolLabel->setFixedWidth(110);
    volumeLayout->addRow(inputVolLabel, inputVolRow);
    
    // Output volume
    auto* outputVolRow = new QHBoxLayout();
    outputVolumeSlider_ = new QSlider(Qt::Horizontal, volumeGroup);
    outputVolumeSlider_->setRange(0, 200);  // 0% to 200%
    outputVolumeSlider_->setValue(80);      // Default 80%
    outputVolumeSlider_->setMinimumWidth(250);
    connect(outputVolumeSlider_, &QSlider::valueChanged,
            this, &SettingsDialog::onOutputVolumeChanged);
    
    outputVolumeLabel_ = new QLabel("80%", volumeGroup);
    outputVolumeLabel_->setFixedWidth(50);
    outputVolumeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    outputVolRow->addWidget(outputVolumeSlider_);
    outputVolRow->addWidget(outputVolumeLabel_);
    
    auto* outputVolLabel = new QLabel("Output Volume:", volumeGroup);
    outputVolLabel->setFixedWidth(110);
    volumeLayout->addRow(outputVolLabel, outputVolRow);
    
    mainLayout->addWidget(volumeGroup);
    
    // === Buttons ===
    mainLayout->addStretch();
    
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    applyButton_ = new QPushButton("✅ Apply", this);
    applyButton_->setMinimumWidth(100);
    connect(applyButton_, &QPushButton::clicked, this, &SettingsDialog::onApply);
    
    cancelButton_ = new QPushButton("❌ Cancel", this);
    cancelButton_->setMinimumWidth(100);
    connect(cancelButton_, &QPushButton::clicked, this, &SettingsDialog::onCancel);
    
    buttonLayout->addWidget(applyButton_);
    buttonLayout->addWidget(cancelButton_);
    
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::loadDevices() {
    if (!audioEngine_) {
        return;
    }
    
    // Load input devices
    inputDevices_ = audioEngine_->enumerate_input_devices();
    inputDeviceCombo_->clear();
    
    for (size_t i = 0; i < inputDevices_.size(); ++i) {
        const auto& device = inputDevices_[i];
        QString displayName = QString::fromStdString(device.name);
        if (device.is_default) {
            displayName += " (Default)";
        }
        inputDeviceCombo_->addItem(displayName, static_cast<int>(device.id));
        
        // Select default device
        if (device.is_default) {
            inputDeviceCombo_->setCurrentIndex(static_cast<int>(i));
            selectedInputId_ = device.id;
        }
    }
    
    // Load output devices
    outputDevices_ = audioEngine_->enumerate_output_devices();
    outputDeviceCombo_->clear();
    
    for (size_t i = 0; i < outputDevices_.size(); ++i) {
        const auto& device = outputDevices_[i];
        QString displayName = QString::fromStdString(device.name);
        if (device.is_default) {
            displayName += " (Default)";
        }
        outputDeviceCombo_->addItem(displayName, static_cast<int>(device.id));
        
        // Select default device
        if (device.is_default) {
            outputDeviceCombo_->setCurrentIndex(static_cast<int>(i));
            selectedOutputId_ = device.id;
        }
    }
}

void SettingsDialog::applyDarkTheme() {
    setStyleSheet(R"(
        QDialog {
            background-color: #2b2d31;
        }
        QGroupBox {
            color: #dcddde;
            border: 1px solid #3e4146;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
            color: #f2f3f5;
        }
        QLabel {
            color: #dcddde;
            background: transparent;
        }
        QComboBox {
            background-color: #1e1f22;
            color: #dcddde;
            border: 1px solid #3e4146;
            border-radius: 3px;
            padding: 5px;
        }
        QComboBox:hover {
            border: 1px solid #4e5157;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #1e1f22;
            color: #dcddde;
            selection-background-color: #5865f2;
        }
        QSlider::groove:horizontal {
            background: #1e1f22;
            height: 6px;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #5865f2;
            width: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #4752c4;
        }
        QPushButton {
            background-color: #5865f2;
            color: white;
            border: none;
            border-radius: 3px;
            padding: 8px 15px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #4752c4;
        }
        QPushButton:pressed {
            background-color: #3c45a5;
        }
    )");
}

DeviceId SettingsDialog::selectedInputDevice() const {
    return selectedInputId_;
}

DeviceId SettingsDialog::selectedOutputDevice() const {
    return selectedOutputId_;
}

float SettingsDialog::inputVolume() const {
    return inputVolumeSlider_->value() / 100.0f;
}

float SettingsDialog::outputVolume() const {
    return outputVolumeSlider_->value() / 100.0f;
}

void SettingsDialog::onInputDeviceChanged(int index) {
    if (index >= 0 && index < static_cast<int>(inputDevices_.size())) {
        selectedInputId_ = inputDevices_[index].id;
    }
}

void SettingsDialog::onOutputDeviceChanged(int index) {
    if (index >= 0 && index < static_cast<int>(outputDevices_.size())) {
        selectedOutputId_ = outputDevices_[index].id;
    }
}

void SettingsDialog::onInputVolumeChanged(int value) {
    inputVolumeLabel_->setText(QString("%1%").arg(value));
}

void SettingsDialog::onOutputVolumeChanged(int value) {
    outputVolumeLabel_->setText(QString("%1%").arg(value));
}

void SettingsDialog::onTestInput() {
    // TODO: Implement input test (show live meter for 3 seconds)
    QMessageBox::information(this, "Input Test",
        "Speak into your microphone. Check the main window's input meter.");
}

void SettingsDialog::onTestOutput() {
    // TODO: Implement output test (play test tone)
    QMessageBox::information(this, "Output Test",
        "A test tone would play here. (Not yet implemented)");
}

void SettingsDialog::onApply() {
    if (!audioEngine_) {
        reject();
        return;
    }
    
    // Apply input device
    if (selectedInputId_ != paNoDevice) {
        auto result = audioEngine_->set_input_device(selectedInputId_);
        if (!result.is_ok()) {
            QMessageBox::warning(this, "Device Error",
                QString("Failed to set input device: %1")
                    .arg(QString::fromStdString(result.error().message())));
        }
    }
    
    // Apply output device
    if (selectedOutputId_ != paNoDevice) {
        auto result = audioEngine_->set_output_device(selectedOutputId_);
        if (!result.is_ok()) {
            QMessageBox::warning(this, "Device Error",
                QString("Failed to set output device: %1")
                    .arg(QString::fromStdString(result.error().message())));
        }
    }
    
    // Apply volumes
    audioEngine_->set_input_volume(inputVolume());
    audioEngine_->set_output_volume(outputVolume());
    
    accept();
}

void SettingsDialog::onCancel() {
    reject();
}

} // namespace voip::ui
