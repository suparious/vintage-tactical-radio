#include "SettingsDialog.h"
#include "../config/Settings.h"
#include "../audio/AudioOutput.h"
#include "../core/RTLSDRDevice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QMessageBox>

SettingsDialog::SettingsDialog(std::shared_ptr<Settings> settings,
                             AudioOutput* audioOutput,
                             RTLSDRDevice* rtlsdr,
                             QWidget* parent)
    : QDialog(parent)
    , settings_(settings)
    , audioOutput_(audioOutput)
    , rtlsdr_(rtlsdr) {
    
    setWindowTitle(tr("Settings"));
    setModal(true);
    setMinimumWidth(500);
    
    setupUI();
    connectSignals();
    populateAudioDevices();
    loadSettings();
    
    // Store initial values for cancel functionality
    initialSampleRate_ = sampleRateCombo_->currentIndex();
    initialSampleFormat_ = sampleFormatCombo_->currentIndex();
    initialDynamicBandwidth_ = dynamicBandwidthCheck_->isChecked();
    initialBiasT_ = biasTCheck_->isChecked();
    initialPpm_ = ppmSpin_->value();
    initialRtlSampleRate_ = rtlSampleRateCombo_->currentIndex();
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Create settings sections
    createAudioSettings();
    createRtlSdrSettings();
    createGeneralSettings();
    
    // Button box
    auto* buttonBox = new QDialogButtonBox(this);
    okButton_ = buttonBox->addButton(QDialogButtonBox::Ok);
    applyButton_ = buttonBox->addButton(QDialogButtonBox::Apply);
    cancelButton_ = buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    // Connect button signals
    connect(okButton_, &QPushButton::clicked, this, &SettingsDialog::onAcceptClicked);
    connect(applyButton_, &QPushButton::clicked, this, &SettingsDialog::onApplyClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::createAudioSettings() {
    auto* audioGroup = new QGroupBox(tr("Audio Settings"), this);
    auto* audioLayout = new QGridLayout(audioGroup);
    
    // Audio device
    audioLayout->addWidget(new QLabel(tr("Audio Device:")), 0, 0);
    audioDeviceCombo_ = new QComboBox();
    audioLayout->addWidget(audioDeviceCombo_, 0, 1);
    
    // Sample rate
    audioLayout->addWidget(new QLabel(tr("Sample Rate:")), 1, 0);
    sampleRateCombo_ = new QComboBox();
    sampleRateCombo_->addItems({"44.1 kHz", "48 kHz", "96 kHz", "192 kHz"});
    sampleRateCombo_->setCurrentIndex(1); // 48 kHz default
    audioLayout->addWidget(sampleRateCombo_, 1, 1);
    
    // Sample format
    audioLayout->addWidget(new QLabel(tr("Sample Format:")), 2, 0);
    sampleFormatCombo_ = new QComboBox();
    sampleFormatCombo_->addItems({"s16le (16-bit)", "s24le (24-bit)"});
    audioLayout->addWidget(sampleFormatCombo_, 2, 1);
    
    layout()->addWidget(audioGroup);
}

void SettingsDialog::createRtlSdrSettings() {
    auto* rtlGroup = new QGroupBox(tr("RTL-SDR Settings"), this);
    auto* rtlLayout = new QGridLayout(rtlGroup);
    
    // Bias-T control
    biasTCheck_ = new QCheckBox(tr("Bias-T Power (4.5V for active antennas)"));
    biasTCheck_->setToolTip(tr("Enable 4.5V power supply on antenna connector"));
    rtlLayout->addWidget(biasTCheck_, 0, 0, 1, 2);
    
    // PPM correction
    rtlLayout->addWidget(new QLabel(tr("PPM Correction:")), 1, 0);
    ppmSpin_ = new QSpinBox();
    ppmSpin_->setRange(-100, 100);
    ppmSpin_->setValue(0);
    ppmSpin_->setSuffix(" ppm");
    ppmSpin_->setToolTip(tr("Frequency correction in parts per million"));
    rtlLayout->addWidget(ppmSpin_, 1, 1);
    
    // RTL-SDR sample rate
    rtlLayout->addWidget(new QLabel(tr("RTL-SDR Sample Rate:")), 2, 0);
    rtlSampleRateCombo_ = new QComboBox();
    rtlSampleRateCombo_->addItems({"2.048 MHz", "2.4 MHz (recommended)", "2.56 MHz", "3.2 MHz"});
    rtlSampleRateCombo_->setCurrentIndex(1); // 2.4 MHz default
    rtlSampleRateCombo_->setToolTip(tr("Higher rates may cause USB drops. 2.4 MHz recommended for stability."));
    rtlLayout->addWidget(rtlSampleRateCombo_, 2, 1);
    
    layout()->addWidget(rtlGroup);
}

void SettingsDialog::createGeneralSettings() {
    auto* generalGroup = new QGroupBox(tr("General Settings"), this);
    auto* generalLayout = new QVBoxLayout(generalGroup);
    
    // Dynamic bandwidth
    dynamicBandwidthCheck_ = new QCheckBox(tr("Dynamic Bandwidth"));
    dynamicBandwidthCheck_->setToolTip(tr("Automatically adjust bandwidth based on signal quality"));
    dynamicBandwidthCheck_->setChecked(true);
    generalLayout->addWidget(dynamicBandwidthCheck_);
    
    // Bandwidth display (read-only)
    bandwidthLabel_ = new QLabel(tr("Current Bandwidth: 200 kHz"));
    bandwidthLabel_->setStyleSheet("QLabel { color: #666; padding-left: 20px; }");
    generalLayout->addWidget(bandwidthLabel_);
    
    // Reset all button
    resetAllButton_ = new QPushButton(tr("Reset All to Defaults"));
    resetAllButton_->setToolTip(tr("Reset all settings to factory defaults"));
    generalLayout->addWidget(resetAllButton_);
    
    layout()->addWidget(generalGroup);
}

void SettingsDialog::connectSignals() {
    // Audio settings
    connect(audioDeviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                settings_->setValue("audio_device_index", index);
                emit audioDeviceChanged(index);
            });
    connect(sampleRateCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::sampleRateChanged);
    connect(sampleFormatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::sampleFormatChanged);
    
    // RTL-SDR settings
    connect(biasTCheck_, &QCheckBox::toggled,
            this, &SettingsDialog::biasTChanged);
    connect(ppmSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::ppmChanged);
    connect(rtlSampleRateCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::rtlSampleRateChanged);
    
    // General settings
    connect(dynamicBandwidthCheck_, &QCheckBox::toggled,
            this, &SettingsDialog::dynamicBandwidthChanged);
    connect(resetAllButton_, &QPushButton::clicked,
            this, &SettingsDialog::onResetAllClicked);
}

void SettingsDialog::populateAudioDevices() {
    if (!audioOutput_) return;
    
    auto audioDevices = audioOutput_->getDevices();
    audioDeviceCombo_->clear();
    
    int savedIndex = settings_->getValue("audio_device_index", -1).toInt();
    int defaultIndex = 0;
    
    for (size_t i = 0; i < audioDevices.size(); ++i) {
        audioDeviceCombo_->addItem(audioDevices[i].name);
        if (audioDevices[i].isDefault) {
            defaultIndex = i;
        }
    }
    
    // Set the saved device if valid, otherwise use default
    if (savedIndex >= 0 && savedIndex < audioDeviceCombo_->count()) {
        audioDeviceCombo_->setCurrentIndex(savedIndex);
    } else {
        audioDeviceCombo_->setCurrentIndex(defaultIndex);
    }
}

void SettingsDialog::loadSettings() {
    // Audio settings
    sampleRateCombo_->setCurrentIndex(settings_->getValue("audio_sample_rate", 1).toInt());
    sampleFormatCombo_->setCurrentIndex(settings_->getValue("audio_sample_format", 0).toInt());
    
    // RTL-SDR settings
    biasTCheck_->setChecked(settings_->getValue("rtl_bias_t", false).toBool());
    ppmSpin_->setValue(settings_->getValue("rtl_ppm", 0).toInt());
    rtlSampleRateCombo_->setCurrentIndex(settings_->getValue("rtl_sample_rate", 1).toInt());
    
    // General settings
    dynamicBandwidthCheck_->setChecked(settings_->getValue("dynamic_bandwidth", true).toBool());
}

void SettingsDialog::saveSettings() {
    // Audio settings
    settings_->setValue("audio_sample_rate", sampleRateCombo_->currentIndex());
    settings_->setValue("audio_sample_format", sampleFormatCombo_->currentIndex());
    
    // RTL-SDR settings
    settings_->setValue("rtl_bias_t", biasTCheck_->isChecked());
    settings_->setValue("rtl_ppm", ppmSpin_->value());
    settings_->setValue("rtl_sample_rate", rtlSampleRateCombo_->currentIndex());
    
    // General settings
    settings_->setValue("dynamic_bandwidth", dynamicBandwidthCheck_->isChecked());
    
    settings_->save();
}

void SettingsDialog::onResetAllClicked() {
    auto reply = QMessageBox::question(this, tr("Reset All Settings"),
                                     tr("Are you sure you want to reset all settings to defaults?"),
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Reset all controls to defaults
        sampleRateCombo_->setCurrentIndex(1); // 48 kHz
        sampleFormatCombo_->setCurrentIndex(0); // 16-bit
        biasTCheck_->setChecked(false);
        ppmSpin_->setValue(0);
        rtlSampleRateCombo_->setCurrentIndex(1); // 2.4 MHz
        dynamicBandwidthCheck_->setChecked(true);
        
        // Emit reset signal
        emit resetAllClicked();
    }
}

void SettingsDialog::onApplyClicked() {
    saveSettings();
}

void SettingsDialog::onAcceptClicked() {
    saveSettings();
    accept();
}

void SettingsDialog::updateBandwidthDisplay(const QString& text) {
    if (bandwidthLabel_) {
        bandwidthLabel_->setText(text);
    }
}

int SettingsDialog::getSampleRate() const {
    const int rates[] = {44100, 48000, 96000, 192000};
    int index = sampleRateCombo_->currentIndex();
    return (index >= 0 && index < 4) ? rates[index] : 48000;
}

int SettingsDialog::getSampleFormat() const {
    return sampleFormatCombo_->currentIndex();
}

bool SettingsDialog::getDynamicBandwidth() const {
    return dynamicBandwidthCheck_->isChecked();
}

bool SettingsDialog::getBiasT() const {
    return biasTCheck_->isChecked();
}

int SettingsDialog::getPpm() const {
    return ppmSpin_->value();
}

int SettingsDialog::getRtlSampleRate() const {
    const int rates[] = {2048000, 2400000, 2560000, 3200000};
    int index = rtlSampleRateCombo_->currentIndex();
    return (index >= 0 && index < 4) ? rates[index] : 2400000;
}
