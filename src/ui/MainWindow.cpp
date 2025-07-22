#include "MainWindow.h"
#include "VintageKnob.h"
#include "VintageMeter.h"
#include "FrequencyDial.h"
#include "SpectrumDisplay.h"
#include "VintageTheme.h"
#include "../config/Settings.h"
#include "../core/RTLSDRDevice.h"
#include "../core/DSPEngine.h"
#include "../audio/AudioOutput.h"
#include "../audio/VintageEqualizer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>
#include <QStatusBar>
#include <QLineEdit>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

MainWindow::MainWindow(std::shared_ptr<Settings> settings, QWidget* parent)
    : QMainWindow(parent)
    , settings_(settings)
    , isRunning_(false)
    , currentFrequency_(96900000) // 96.9 MHz
    , currentBand_(2) { // FM band
    
    setWindowTitle("Vintage Tactical Radio");
    setWindowIcon(QIcon(":/images/radio-icon.png"));
    
    // Initialize components
    rtlsdr_ = std::make_unique<RTLSDRDevice>();
    dspEngine_ = std::make_unique<DSPEngine>();
    audioOutput_ = std::make_unique<AudioOutput>(this);
    equalizer_ = std::make_unique<VintageEqualizer>(48000, VintageEqualizer::MODERN);
    
    setupUI();
    connectSignals();
    applyTheme();
    
    initializeDevices();
    loadSettings();
    
    // Set initial DSP callbacks
    dspEngine_->setAudioCallback([this](const float* data, size_t length) {
        // Process through equalizer
        std::vector<float> eqBuffer(length);
        equalizer_->process(data, eqBuffer.data(), length);
        
        // Send to audio output
        audioOutput_->writeAudio(eqBuffer.data(), length);
    });
    
    dspEngine_->setSignalCallback([this](float strength) {
        QMetaObject::invokeMethod(this, [this, strength]() {
            onSignalStrengthChanged(strength);
        }, Qt::QueuedConnection);
    });
    
    dspEngine_->setSpectrumCallback([this](const float* data, size_t length) {
        // Copy data for thread safety
        std::vector<float> spectrumData(data, data + length);
        QMetaObject::invokeMethod(this, [this, spectrumData]() {
            onSpectrumData(spectrumData.data(), spectrumData.size());
        }, Qt::QueuedConnection);
    });
}

MainWindow::~MainWindow() {
    if (isRunning_) {
        stopRadio();
    }
}

void MainWindow::setupUI() {
    // Create central widget
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    createMenuBar();
    createCentralWidget();
    
    // Set window size
    resize(1280, 960);
    setMinimumSize(800, 600);
}

void MainWindow::createMenuBar() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    
    auto* exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    
    auto* themeMenu = viewMenu->addMenu(tr("&Theme"));
    auto* oliveAction = new QAction(tr("Military Olive"), this);
    auto* navyAction = new QAction(tr("Navy Grey"), this);
    auto* nightAction = new QAction(tr("Night Mode"), this);
    themeMenu->addAction(oliveAction);
    themeMenu->addAction(navyAction);
    themeMenu->addAction(nightAction);
    
    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    auto* aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About Vintage Tactical Radio"),
            tr("Vintage Tactical Radio v1.0.0\n\n"
               "RTL-SDR based military-style radio receiver\n\n"
               "© 2025 Vintage Radio Project"));
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::createCentralWidget() {
    auto* centralWidget = this->centralWidget();
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Top section: Frequency display and spectrum
    auto* topLayout = new QHBoxLayout();
    
    // Frequency dial group
    auto* freqGroup = new QGroupBox(tr("FREQUENCY"));
    freqGroup->setObjectName("frequencyGroup");
    auto* freqLayout = new QVBoxLayout(freqGroup);
    
    frequencyDial_ = new FrequencyDial(this);
    frequencyDial_->setFrequency(currentFrequency_);
    freqLayout->addWidget(frequencyDial_);
    
    // Add direct frequency input
    auto* freqInputLayout = new QHBoxLayout();
    auto* freqInputLabel = new QLabel(tr("Direct Input:"));
    auto* freqInput = new QLineEdit();
    freqInput->setPlaceholderText("96.9");
    freqInput->setMaximumWidth(80);
    
    auto* freqUnitCombo = new QComboBox();
    freqUnitCombo->addItems({"MHz", "kHz"});
    freqUnitCombo->setMaximumWidth(60);
    
    auto* freqGoButton = new QPushButton(tr("Go"));
    freqGoButton->setMaximumWidth(40);
    
    // Connect direct frequency input
    auto tuneToFrequency = [this, freqInput, freqUnitCombo]() {
        bool ok;
        double value = freqInput->text().toDouble(&ok);
        if (ok) {
            double multiplier = (freqUnitCombo->currentIndex() == 0) ? 1e6 : 1e3;
            onFrequencyChanged(value * multiplier);
        }
    };
    
    connect(freqGoButton, &QPushButton::clicked, tuneToFrequency);
    connect(freqInput, &QLineEdit::returnPressed, tuneToFrequency);
    
    freqInputLayout->addWidget(freqInputLabel);
    freqInputLayout->addWidget(freqInput);
    freqInputLayout->addWidget(freqUnitCombo);
    freqInputLayout->addWidget(freqGoButton);
    freqInputLayout->addStretch();
    
    freqLayout->addLayout(freqInputLayout);
    
    topLayout->addWidget(freqGroup);
    
    // Spectrum display group
    auto* spectrumGroup = new QGroupBox(tr("SPECTRUM DISPLAY"));
    spectrumGroup->setObjectName("spectrumGroup");
    auto* spectrumLayout = new QVBoxLayout(spectrumGroup);
    
    spectrumDisplay_ = new SpectrumDisplay(this);
    spectrumLayout->addWidget(spectrumDisplay_);
    
    // S-Meter
    signalMeter_ = new VintageMeter(this);
    signalMeter_->setRange(-100, 0);
    signalMeter_->setLabel("S-METER");
    spectrumLayout->addWidget(signalMeter_);
    
    topLayout->addWidget(spectrumGroup, 2);
    
    mainLayout->addLayout(topLayout);
    
    // Middle section: Mode and band selectors
    auto* modeLayout = new QHBoxLayout();
    
    // Device selector
    auto* deviceGroup = new QGroupBox(tr("DEVICE"));
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    deviceCombo_ = new QComboBox();
    deviceLayout->addWidget(deviceCombo_);
    modeLayout->addWidget(deviceGroup);
    
    // Mode selector
    auto* modeGroup = new QGroupBox(tr("MODE SELECT"));
    auto* modeGroupLayout = new QVBoxLayout(modeGroup);
    modeSelector_ = new QComboBox();
    modeSelector_->addItems({"AM", "FM", "SSB", "CW"});
    modeSelector_->setCurrentIndex(1); // FM
    modeGroupLayout->addWidget(modeSelector_);
    modeLayout->addWidget(modeGroup);
    
    // Band selector
    auto* bandGroup = new QGroupBox(tr("BAND SELECTOR"));
    auto* bandLayout = new QVBoxLayout(bandGroup);
    bandSelector_ = new QComboBox();
    bandSelector_->addItems({"MW", "SW", "FM", "VHF", "UHF"});
    bandSelector_->setCurrentIndex(2); // FM
    bandLayout->addWidget(bandSelector_);
    modeLayout->addWidget(bandGroup);
    
    mainLayout->addLayout(modeLayout);
    
    // Control panel
    createControlPanel();
    
    // EQ panel
    createEQPanel();
    
    // Settings panel
    createSettingsPanel();
    
    // Status bar
    statusLabel_ = new QLabel(tr("Ready"));
    statusBar()->addWidget(statusLabel_);
}

void MainWindow::createControlPanel() {
    auto* controlGroup = new QGroupBox(tr("CONTROL PANEL"));
    controlGroup->setObjectName("controlPanel");
    auto* controlLayout = new QHBoxLayout(controlGroup);
    
    // Volume
    auto* volumeLayout = new QVBoxLayout();
    volumeKnob_ = new VintageKnob(this);
    volumeKnob_->setRange(0, 100);
    volumeKnob_->setValue(75);
    volumeKnob_->setLabel("VOLUME");
    volumeLayout->addWidget(volumeKnob_);
    controlLayout->addLayout(volumeLayout);
    
    // Squelch
    auto* squelchLayout = new QVBoxLayout();
    squelchKnob_ = new VintageKnob(this);
    squelchKnob_->setRange(-100, 0);
    squelchKnob_->setValue(-20);
    squelchKnob_->setLabel("SQUELCH");
    squelchLayout->addWidget(squelchKnob_);
    controlLayout->addLayout(squelchLayout);
    
    // Gain
    auto* gainLayout = new QVBoxLayout();
    gainKnob_ = new VintageKnob(this);
    gainKnob_->setRange(0, 49);
    gainKnob_->setValue(25);
    gainKnob_->setLabel("RF GAIN");
    gainLayout->addWidget(gainKnob_);
    controlLayout->addLayout(gainLayout);
    
    // Fine tune
    auto* tuneLayout = new QVBoxLayout();
    tuningKnob_ = new VintageKnob(this);
    tuningKnob_->setRange(-100, 100);
    tuningKnob_->setValue(0);
    tuningKnob_->setLabel("FINE TUNE");
    tuningKnob_->setWrapping(false);
    tuneLayout->addWidget(tuningKnob_);
    controlLayout->addLayout(tuneLayout);
    
    // Start/Stop button
    auto* buttonLayout = new QVBoxLayout();
    startStopButton_ = new QPushButton(tr("START"));
    startStopButton_->setObjectName("startStopButton");
    startStopButton_->setCheckable(true);
    buttonLayout->addWidget(startStopButton_);
    buttonLayout->addStretch();
    controlLayout->addLayout(buttonLayout);
    
    centralWidget()->layout()->addWidget(controlGroup);
}

void MainWindow::createEQPanel() {
    auto* eqGroup = new QGroupBox(tr("7-BAND EQUALIZER"));
    eqGroup->setObjectName("equalizerPanel");
    auto* eqMainLayout = new QVBoxLayout(eqGroup);
    
    // EQ mode and preset selectors
    auto* eqControlLayout = new QHBoxLayout();
    
    eqModeCombo_ = new QComboBox();
    eqModeCombo_->addItems({"Modern", "Nostalgic"});
    eqControlLayout->addWidget(new QLabel(tr("Mode:")));
    eqControlLayout->addWidget(eqModeCombo_);
    
    eqPresetCombo_ = new QComboBox();
    auto presetNames = equalizer_->getPresetNames();
    for (const auto& name : presetNames) {
        eqPresetCombo_->addItem(QString::fromStdString(name));
    }
    eqControlLayout->addWidget(new QLabel(tr("Preset:")));
    eqControlLayout->addWidget(eqPresetCombo_);
    
    resetEQButton_ = new QPushButton(tr("Reset EQ"));
    eqControlLayout->addWidget(resetEQButton_);
    
    eqControlLayout->addStretch();
    eqMainLayout->addLayout(eqControlLayout);
    
    // EQ knobs
    auto* eqKnobLayout = new QHBoxLayout();
    
    const QStringList freqLabels = {"50Hz", "125Hz", "315Hz", "750Hz", "2.2kHz", "6kHz", "16kHz"};
    eqKnobs_.resize(7);
    
    for (int i = 0; i < 7; i++) {
        auto* knobLayout = new QVBoxLayout();
        eqKnobs_[i] = new VintageKnob(this);
        eqKnobs_[i]->setRange(-12, 12);
        eqKnobs_[i]->setValue(0);
        eqKnobs_[i]->setLabel(freqLabels[i]);
        knobLayout->addWidget(eqKnobs_[i]);
        eqKnobLayout->addLayout(knobLayout);
    }
    
    eqMainLayout->addLayout(eqKnobLayout);
    
    centralWidget()->layout()->addWidget(eqGroup);
}

void MainWindow::createSettingsPanel() {
    auto* settingsGroup = new QGroupBox(tr("SETTINGS"));
    settingsGroup->setObjectName("settingsPanel");
    auto* settingsLayout = new QGridLayout(settingsGroup);
    
    // Audio device
    settingsLayout->addWidget(new QLabel(tr("Audio Device:")), 0, 0);
    audioDeviceCombo_ = new QComboBox();
    settingsLayout->addWidget(audioDeviceCombo_, 0, 1);
    
    // Sample rate
    settingsLayout->addWidget(new QLabel(tr("Sample Rate:")), 1, 0);
    sampleRateCombo_ = new QComboBox();
    sampleRateCombo_->addItems({"44100 Hz", "48000 Hz", "192000 Hz"});
    sampleRateCombo_->setCurrentIndex(1); // 48000 Hz
    settingsLayout->addWidget(sampleRateCombo_, 1, 1);
    
    // Sample format
    settingsLayout->addWidget(new QLabel(tr("Sample Format:")), 2, 0);
    sampleFormatCombo_ = new QComboBox();
    sampleFormatCombo_->addItems({"16-bit", "24-bit"});
    settingsLayout->addWidget(sampleFormatCombo_, 2, 1);
    
    // Reset all button
    resetAllButton_ = new QPushButton(tr("Reset All to Defaults"));
    settingsLayout->addWidget(resetAllButton_, 3, 0, 1, 2);
    
    centralWidget()->layout()->addWidget(settingsGroup);
}

void MainWindow::connectSignals() {
    // Device control
    connect(deviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDeviceChanged);
    connect(startStopButton_, &QPushButton::clicked,
            this, &MainWindow::onStartStop);
    
    // Frequency control
    connect(frequencyDial_, &FrequencyDial::frequencyChanged,
            this, &MainWindow::onFrequencyChanged);
    connect(bandSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBandChanged);
    connect(modeSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);
    
    // Control knobs
    connect(volumeKnob_, &VintageKnob::valueChanged,
            this, &MainWindow::onVolumeChanged);
    connect(squelchKnob_, &VintageKnob::valueChanged,
            this, &MainWindow::onSquelchChanged);
    connect(gainKnob_, &VintageKnob::valueChanged,
            this, &MainWindow::onGainChanged);
    
    // EQ controls
    connect(eqModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEQModeChanged);
    connect(eqPresetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEQPresetChanged);
    connect(resetEQButton_, &QPushButton::clicked,
            this, &MainWindow::onEQResetClicked);
    
    for (int i = 0; i < 7; i++) {
        connect(eqKnobs_[i], &VintageKnob::valueChanged,
                [this, i](double value) { onEQBandChanged(i, value); });
    }
    
    // Settings
    connect(audioDeviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAudioDeviceChanged);
    connect(sampleRateCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSampleRateChanged);
    connect(sampleFormatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSampleFormatChanged);
    connect(resetAllButton_, &QPushButton::clicked,
            this, &MainWindow::onResetAllClicked);
    
    // Fine tuning
    connect(tuningKnob_, &VintageKnob::valueChanged,
            [this](double value) {
                double offset = value * 1000; // +/- 100 kHz
                onFrequencyChanged(currentFrequency_ + offset);
            });
}

void MainWindow::applyTheme() {
    VintageTheme::applyTheme(this, VintageTheme::MILITARY_OLIVE);
}

void MainWindow::initializeDevices() {
    // Populate RTL-SDR devices
    auto devices = rtlsdr_->getDeviceList();
    deviceCombo_->clear();
    for (const auto& device : devices) {
        deviceCombo_->addItem(QString::fromStdString(device));
    }
    
    if (devices.empty()) {
        deviceCombo_->addItem(tr("No RTL-SDR devices found"));
        startStopButton_->setEnabled(false);
    }
    
    // Populate audio devices
    auto audioDevices = audioOutput_->getDevices();
    audioDeviceCombo_->clear();
    for (const auto& device : audioDevices) {
        audioDeviceCombo_->addItem(device.name);
        if (device.isDefault) {
            audioDeviceCombo_->setCurrentIndex(audioDeviceCombo_->count() - 1);
        }
    }
}

void MainWindow::onDeviceChanged(int index) {
    Q_UNUSED(index);
    // Device change handled when starting radio
}

void MainWindow::onStartStop() {
    if (isRunning_) {
        stopRadio();
        startStopButton_->setText(tr("START"));
    } else {
        startRadio();
        startStopButton_->setText(tr("STOP"));
    }
}

void MainWindow::startRadio() {
    if (deviceCombo_->count() == 0 || rtlsdr_->getDeviceCount() == 0) {
        QMessageBox::warning(this, tr("No Device"),
                           tr("No RTL-SDR device found. Please connect a device."));
        return;
    }
    
    try {
        // Open RTL-SDR device
        if (!rtlsdr_->open(deviceCombo_->currentIndex())) {
            throw std::runtime_error("Failed to open RTL-SDR device");
        }
        
        // Configure RTL-SDR
        rtlsdr_->setCenterFrequency(currentFrequency_);
        rtlsdr_->setSampleRate(2400000); // 2.4 MHz
        rtlsdr_->setGain(gainKnob_->value() * 10); // Convert to tenths of dB
        
        // Set RTL-SDR data callback
        rtlsdr_->setDataCallback([this](const uint8_t* data, size_t length) {
            dspEngine_->processIQ(data, length);
        });
        
        // Configure DSP
        dspEngine_->setSampleRate(2400000);
        dspEngine_->setMode(static_cast<DSPEngine::Mode>(modeSelector_->currentIndex()));
        dspEngine_->setSquelch(squelchKnob_->value());
        
        // Start audio output
        audioOutput_->start();
        
        // Start DSP engine
        dspEngine_->start();
        
        // Start RTL-SDR streaming
        rtlsdr_->startStreaming();
        
        isRunning_ = true;
        updateStatus(tr("Radio started"));
        
#ifdef HAS_SPDLOG
        spdlog::info("Radio started successfully");
#endif
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Start Error"),
                            tr("Failed to start radio: %1").arg(e.what()));
        stopRadio();
    }
}

void MainWindow::stopRadio() {
    if (rtlsdr_->isStreaming()) {
        rtlsdr_->stopStreaming();
    }
    
    if (dspEngine_->isRunning()) {
        dspEngine_->stop();
    }
    
    audioOutput_->stop();
    
    if (rtlsdr_->isOpen()) {
        rtlsdr_->close();
    }
    
    isRunning_ = false;
    updateStatus(tr("Radio stopped"));
    
#ifdef HAS_SPDLOG
    spdlog::info("Radio stopped");
#endif
}

void MainWindow::onFrequencyChanged(double frequency) {
    currentFrequency_ = frequency;
    frequencyDial_->setFrequency(frequency);
    
    if (rtlsdr_->isOpen()) {
        rtlsdr_->setCenterFrequency(frequency);
    }
    
    // Update band selector if frequency moved out of current band
    if (frequency >= 88e6 && frequency <= 108e6) {
        bandSelector_->setCurrentIndex(2); // FM
    } else if (frequency >= 530e3 && frequency <= 1700e3) {
        bandSelector_->setCurrentIndex(0); // MW
    }
}

void MainWindow::onBandChanged(int band) {
    currentBand_ = band;
    updateFrequencyForBand(band);
}

void MainWindow::updateFrequencyForBand(int band) {
    double freq = currentFrequency_;
    
    switch (band) {
        case 0: // MW
            freq = 1000000; // 1 MHz
            break;
        case 1: // SW
            freq = 9750000; // 9.75 MHz
            break;
        case 2: // FM
            freq = 96900000; // 96.9 MHz
            break;
        case 3: // VHF
            freq = 156800000; // Marine Ch 16
            break;
        case 4: // UHF
            freq = 446000000; // PMR446
            break;
    }
    
    onFrequencyChanged(freq);
}

void MainWindow::onModeChanged(int mode) {
    if (dspEngine_) {
        dspEngine_->setMode(static_cast<DSPEngine::Mode>(mode));
    }
}

void MainWindow::onVolumeChanged(double value) {
    audioOutput_->setVolume(value / 100.0f);
}

void MainWindow::onSquelchChanged(double value) {
    if (dspEngine_) {
        dspEngine_->setSquelch(value);
    }
}

void MainWindow::onGainChanged(double value) {
    if (rtlsdr_->isOpen()) {
        rtlsdr_->setGain(value * 10); // Convert to tenths of dB
    }
}

void MainWindow::onEQModeChanged(int mode) {
    equalizer_->setMode(static_cast<VintageEqualizer::Mode>(mode));
    
    // Update frequency labels
    const QStringList modernLabels = {"50Hz", "125Hz", "315Hz", "750Hz", "2.2kHz", "6kHz", "16kHz"};
    const QStringList nostalgicLabels = {"60Hz", "150Hz", "400Hz", "1kHz", "2.4kHz", "6kHz", "15kHz"};
    const auto& labels = (mode == 0) ? modernLabels : nostalgicLabels;
    
    for (int i = 0; i < 7; i++) {
        eqKnobs_[i]->setLabel(labels[i]);
    }
}

void MainWindow::onEQPresetChanged(int preset) {
    auto presetNames = equalizer_->getPresetNames();
    if (preset >= 0 && static_cast<size_t>(preset) < presetNames.size()) {
        equalizer_->loadPreset(presetNames[preset]);
        
        // Update knobs to reflect preset
        for (int i = 0; i < 7; i++) {
            eqKnobs_[i]->setValue(equalizer_->getBandGain(i));
        }
    }
}

void MainWindow::onEQBandChanged(int band, double value) {
    equalizer_->setBandGain(band, value);
}

void MainWindow::onEQResetClicked() {
    equalizer_->reset();
    for (int i = 0; i < 7; i++) {
        eqKnobs_[i]->setValue(0);
    }
    eqPresetCombo_->setCurrentIndex(0); // Flat preset
}

void MainWindow::onAudioDeviceChanged(int index) {
    if (index >= 0 && index < audioDeviceCombo_->count()) {
        auto devices = audioOutput_->getDevices();
        if (static_cast<size_t>(index) < devices.size()) {
            audioOutput_->setDevice(devices[index].id);
        }
    }
}

void MainWindow::onSampleRateChanged(int index) {
    const int rates[] = {44100, 48000, 192000};
    if (index >= 0 && index < 3) {
        audioOutput_->setSampleRate(rates[index]);
    }
}

void MainWindow::onSampleFormatChanged(int index) {
    if (index == 0) {
        audioOutput_->setSampleFormat(QAudioFormat::Int16);
    } else {
        audioOutput_->setSampleFormat(QAudioFormat::Int32);
    }
}

void MainWindow::onResetAllClicked() {
    auto reply = QMessageBox::question(this, tr("Reset All Settings"),
                                     tr("Are you sure you want to reset all settings to defaults?"),
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Reset all controls to defaults
        volumeKnob_->setValue(75);
        squelchKnob_->setValue(-20);
        gainKnob_->setValue(25);
        tuningKnob_->setValue(0);
        
        // Reset EQ
        onEQResetClicked();
        eqModeCombo_->setCurrentIndex(0);
        
        // Reset mode and band
        modeSelector_->setCurrentIndex(1); // FM
        bandSelector_->setCurrentIndex(2); // FM band
        
        // Reset audio settings
        sampleRateCombo_->setCurrentIndex(1); // 48000 Hz
        sampleFormatCombo_->setCurrentIndex(0); // 16-bit
        
        updateStatus(tr("All settings reset to defaults"));
    }
}

void MainWindow::onSignalStrengthChanged(float strength) {
    signalMeter_->setValue(strength);
    
    // Update squelch indicator
    if (dspEngine_->isSquelched()) {
        signalMeter_->setProperty("squelched", true);
    } else {
        signalMeter_->setProperty("squelched", false);
    }
}

void MainWindow::onSpectrumData(const float* data, size_t length) {
    spectrumDisplay_->updateSpectrum(data, length);
}

void MainWindow::updateStatus(const QString& message) {
    statusLabel_->setText(message);
}

void MainWindow::saveSettings() {
    settings_->setValue("frequency", currentFrequency_);
    settings_->setValue("mode", modeSelector_->currentIndex());
    settings_->setValue("band", bandSelector_->currentIndex());
    settings_->setValue("volume", volumeKnob_->value());
    settings_->setValue("squelch", squelchKnob_->value());
    settings_->setValue("gain", gainKnob_->value());
    settings_->setValue("eq_mode", eqModeCombo_->currentIndex());
    settings_->setValue("eq_preset", eqPresetCombo_->currentIndex());
    
    for (int i = 0; i < 7; i++) {
        settings_->setValue(QString("eq_band_%1").arg(i), eqKnobs_[i]->value());
    }
    
    settings_->save();
}

void MainWindow::loadSettings() {
    currentFrequency_ = settings_->getValue("frequency", 96900000.0).toDouble();
    frequencyDial_->setFrequency(currentFrequency_);
    
    modeSelector_->setCurrentIndex(settings_->getValue("mode", 1).toInt());
    bandSelector_->setCurrentIndex(settings_->getValue("band", 2).toInt());
    volumeKnob_->setValue(settings_->getValue("volume", 75.0).toDouble());
    squelchKnob_->setValue(settings_->getValue("squelch", -20.0).toDouble());
    gainKnob_->setValue(settings_->getValue("gain", 25.0).toDouble());
    eqModeCombo_->setCurrentIndex(settings_->getValue("eq_mode", 0).toInt());
    eqPresetCombo_->setCurrentIndex(settings_->getValue("eq_preset", 0).toInt());
    
    for (int i = 0; i < 7; i++) {
        double value = settings_->getValue(QString("eq_band_%1").arg(i), 0.0).toDouble();
        eqKnobs_[i]->setValue(value);
        equalizer_->setBandGain(i, value);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    
    if (isRunning_) {
        stopRadio();
    }
    
    event->accept();
}
