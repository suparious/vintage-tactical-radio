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
#include "../config/MemoryChannel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>
#include <QStatusBar>
#include <QLineEdit>
#include <QSpinBox>
#include <QFile>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

MainWindow::MainWindow(std::shared_ptr<Settings> settings, QWidget* parent)
    : QMainWindow(parent)
    , settings_(settings)
    , isRunning_(false)
    , currentFrequency_(96900000) // 96.9 MHz
    , currentBand_(2)  // FM band
    , currentTheme_(0) { // Military Olive default
    
    setWindowTitle("Vintage Tactical Radio");
    setWindowIcon(QIcon(":/images/radio-icon.png"));
    
    // Initialize components
    rtlsdr_ = std::make_unique<RTLSDRDevice>();
    dspEngine_ = std::make_unique<DSPEngine>();
    audioOutput_ = std::make_unique<AudioOutput>(this);
    equalizer_ = std::make_unique<VintageEqualizer>(48000, VintageEqualizer::MODERN);
    memoryManager_ = std::make_unique<MemoryChannelManager>();
    
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
    
    // Create theme actions with keyboard shortcuts
    auto* oliveAction = new QAction(tr("Military Olive"), this);
    oliveAction->setShortcut(QKeySequence("F1"));
    oliveAction->setCheckable(true);
    oliveAction->setChecked(true);
    
    auto* navyAction = new QAction(tr("Navy Grey"), this);
    navyAction->setShortcut(QKeySequence("F2"));
    navyAction->setCheckable(true);
    
    auto* nightAction = new QAction(tr("Night Mode"), this);
    nightAction->setShortcut(QKeySequence("F3"));
    nightAction->setCheckable(true);
    
    auto* desertAction = new QAction(tr("Desert Tan"), this);
    desertAction->setShortcut(QKeySequence("F4"));
    desertAction->setCheckable(true);
    
    auto* blackOpsAction = new QAction(tr("Black Ops"), this);
    blackOpsAction->setShortcut(QKeySequence("F5"));
    blackOpsAction->setCheckable(true);
    
    // Create action group for exclusive selection
    auto* themeGroup = new QActionGroup(this);
    themeGroup->addAction(oliveAction);
    themeGroup->addAction(navyAction);
    themeGroup->addAction(nightAction);
    themeGroup->addAction(desertAction);
    themeGroup->addAction(blackOpsAction);
    
    themeMenu->addAction(oliveAction);
    themeMenu->addAction(navyAction);
    themeMenu->addAction(nightAction);
    themeMenu->addAction(desertAction);
    themeMenu->addAction(blackOpsAction);
    
    // Connect theme actions
    connect(oliveAction, &QAction::triggered, [this]() { onThemeChanged(VintageTheme::MILITARY_OLIVE); });
    connect(navyAction, &QAction::triggered, [this]() { onThemeChanged(VintageTheme::NAVY_GREY); });
    connect(nightAction, &QAction::triggered, [this]() { onThemeChanged(VintageTheme::NIGHT_MODE); });
    connect(desertAction, &QAction::triggered, [this]() { onThemeChanged(VintageTheme::DESERT_TAN); });
    connect(blackOpsAction, &QAction::triggered, [this]() { onThemeChanged(VintageTheme::BLACK_OPS); });
    
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
    modeSelector_->addItems({"AM", "FM-Narrow", "FM-Wide", "USB", "LSB", "CW"});
    modeSelector_->setCurrentIndex(2); // FM-Wide
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
    
    // Memory channel panel
    createMemoryPanel();
    
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
    gainKnob_->setRange(0, 49.6);
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
    
    // Add gain range selector
    auto* gainRangeLayout = new QHBoxLayout();
    gainRangeLayout->addWidget(new QLabel(tr("Gain Range:")));
    eqGainRangeCombo_ = new QComboBox();
    eqGainRangeCombo_->addItems({"+/-12dB", "+/-18dB", "+/-24dB", "+/-30dB"});
    gainRangeLayout->addWidget(eqGainRangeCombo_);
    gainRangeLayout->addStretch();
    eqMainLayout->addLayout(gainRangeLayout);
    
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
    sampleRateCombo_->addItems({"44.1 kHz", "48 kHz", "96 kHz", "192 kHz"});
    sampleRateCombo_->setCurrentIndex(1); // 48 kHz
    settingsLayout->addWidget(sampleRateCombo_, 1, 1);
    
    // Sample format
    settingsLayout->addWidget(new QLabel(tr("Sample Format:")), 2, 0);
    sampleFormatCombo_ = new QComboBox();
    sampleFormatCombo_->addItems({"s16le (16-bit)", "s24le (24-bit)"});
    settingsLayout->addWidget(sampleFormatCombo_, 2, 1);
    
    // Dynamic bandwidth
    dynamicBandwidthCheck_ = new QCheckBox(tr("Dynamic Bandwidth"));
    dynamicBandwidthCheck_->setToolTip(tr("Automatically adjust bandwidth based on signal quality"));
    dynamicBandwidthCheck_->setChecked(true);
    settingsLayout->addWidget(dynamicBandwidthCheck_, 3, 0, 1, 2);
    
    // Bandwidth display
    bandwidthLabel_ = new QLabel(tr("Bandwidth: 200 kHz"));
    settingsLayout->addWidget(bandwidthLabel_, 4, 0, 1, 2);
    
    // Bias-T control
    biasTCheck_ = new QCheckBox(tr("Bias-T Power"));
    biasTCheck_->setToolTip(tr("Enable 4.5V power for active antennas"));
    biasTCheck_->setChecked(false);
    settingsLayout->addWidget(biasTCheck_, 5, 0, 1, 2);
    
    // PPM correction
    settingsLayout->addWidget(new QLabel(tr("PPM Correction:")), 6, 0);
    ppmSpin_ = new QSpinBox();
    ppmSpin_->setRange(-100, 100);
    ppmSpin_->setValue(0);
    ppmSpin_->setSuffix(" ppm");
    ppmSpin_->setToolTip(tr("Frequency correction in parts per million"));
    settingsLayout->addWidget(ppmSpin_, 6, 1);
    
    // RTL-SDR sample rate
    settingsLayout->addWidget(new QLabel(tr("RTL-SDR Rate:")), 7, 0);
    rtlSampleRateCombo_ = new QComboBox();
    rtlSampleRateCombo_->addItems({"2.048 MHz", "2.4 MHz (default)", "2.56 MHz", "3.2 MHz"});
    rtlSampleRateCombo_->setCurrentIndex(1); // 2.4 MHz default
    rtlSampleRateCombo_->setToolTip(tr("RTL-SDR sampling rate - 2.4 MHz recommended for stability"));
    settingsLayout->addWidget(rtlSampleRateCombo_, 7, 1);
    
    // Reset all button
    resetAllButton_ = new QPushButton(tr("Reset All to Defaults"));
    settingsLayout->addWidget(resetAllButton_, 8, 0, 1, 2);
    
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
    connect(eqGainRangeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEQGainRangeChanged);
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
    connect(dynamicBandwidthCheck_, &QCheckBox::toggled,
            this, &MainWindow::onDynamicBandwidthChanged);
    connect(biasTCheck_, &QCheckBox::toggled,
            this, &MainWindow::onBiasTChanged);
    connect(ppmSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onPpmChanged);
    connect(rtlSampleRateCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onRtlSampleRateChanged);
    
    // Fine tuning
    connect(tuningKnob_, &VintageKnob::valueChanged,
            [this](double value) {
                double offset = value * 1000; // +/- 100 kHz
                if (rtlsdr_->isOpen()) {
                    rtlsdr_->setCenterFrequency(currentFrequency_ + offset);
                }
            });
    
    // Memory channel controls
    connect(memoryStoreButton_, &QPushButton::clicked,
            this, &MainWindow::onMemoryStore);
    connect(memoryRecallButton_, &QPushButton::clicked,
            this, &MainWindow::onMemoryRecall);
    connect(memoryClearButton_, &QPushButton::clicked,
            this, &MainWindow::onMemoryClear);
    connect(memoryBankCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMemoryChannelChanged);
    connect(memoryChannelSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMemoryChannelChanged);
    connect(quickChannelCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onQuickChannelSelected);
}

void MainWindow::applyTheme() {
    // Load saved theme or use default
    currentTheme_ = settings_->getValue("theme", 0).toInt();
    VintageTheme::applyTheme(this, static_cast<VintageTheme::Theme>(currentTheme_));
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
        
        // Apply selected sample rate
        const uint32_t rtlRates[] = {2048000, 2400000, 2560000, 3200000};
        int rateIndex = rtlSampleRateCombo_->currentIndex();
        if (rateIndex >= 0 && rateIndex < 4) {
            rtlsdr_->setSampleRate(rtlRates[rateIndex]);
            dspEngine_->setSampleRate(rtlRates[rateIndex]);
        }
        
        rtlsdr_->setGain(gainKnob_->value() * 10); // Convert to tenths of dB
        rtlsdr_->setFrequencyCorrection(ppmSpin_->value()); // Apply PPM correction
        
        // Apply bias-T if enabled
        if (biasTCheck_->isChecked()) {
            rtlsdr_->setBiasT(true);
        }
        
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
        
        // Start periodic bandwidth updates if dynamic bandwidth is enabled
        if (dynamicBandwidthCheck_->isChecked()) {
            QTimer* bandwidthTimer = new QTimer(this);
            connect(bandwidthTimer, &QTimer::timeout, this, &MainWindow::updateBandwidthDisplay);
            bandwidthTimer->start(500); // Update every 500ms
            bandwidthTimer->setProperty("bandwidthUpdateTimer", true);
        }
        
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
    
    // Stop bandwidth update timer
    QList<QTimer*> timers = findChildren<QTimer*>();
    for (QTimer* timer : timers) {
        if (timer->property("bandwidthUpdateTimer").toBool()) {
            timer->stop();
            timer->deleteLater();
        }
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
    
    // Reset fine tuning when main frequency changes
    if (tuningKnob_->value() != 0) {
        tuningKnob_->setValue(0);
    }
    
    if (rtlsdr_->isOpen()) {
        rtlsdr_->setCenterFrequency(frequency);
    }
    
    // Update band selector if frequency moved out of current band
    if (frequency >= 88e6 && frequency <= 108e6) {
        bandSelector_->setCurrentIndex(2); // FM
    } else if (frequency >= 530e3 && frequency <= 1700e3) {
        bandSelector_->setCurrentIndex(0); // MW
    }
    
    // Apply optimal gain for the frequency
    applyOptimalGain(frequency);
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
        updateBandwidthDisplay();
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

void MainWindow::onEQGainRangeChanged(int index) {
    const float gainRanges[] = {12.0f, 18.0f, 24.0f, 30.0f};
    if (index >= 0 && index < 4) {
        float maxGain = gainRanges[index];
        equalizer_->setMaxGain(maxGain);
        
        // Update all EQ knob ranges
        for (int i = 0; i < 7; i++) {
            float currentValue = eqKnobs_[i]->value();
            eqKnobs_[i]->setRange(-maxGain, maxGain);
            // Preserve current value if within new range
            if (currentValue >= -maxGain && currentValue <= maxGain) {
                eqKnobs_[i]->setValue(currentValue);
            } else {
                // Clamp to new range
                eqKnobs_[i]->setValue(std::max(-maxGain, std::min(maxGain, (float)currentValue)));
            }
        }
    }
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
    const int rates[] = {44100, 48000, 96000, 192000};
    if (index >= 0 && index < 4) {
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
        modeSelector_->setCurrentIndex(2); // FM-Wide
        bandSelector_->setCurrentIndex(2); // FM band
        
        // Reset audio settings
        sampleRateCombo_->setCurrentIndex(1); // 48000 Hz
        sampleFormatCombo_->setCurrentIndex(0); // 16-bit
        
        // Reset dynamic bandwidth
        dynamicBandwidthCheck_->setChecked(true);
        
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
    settings_->setValue("theme", currentTheme_);
    
    for (int i = 0; i < 7; i++) {
        settings_->setValue(QString("eq_band_%1").arg(i), eqKnobs_[i]->value());
    }
    
    settings_->setValue("dynamic_bandwidth", dynamicBandwidthCheck_->isChecked());
    
    // Save memory channels
    QString memoryFile = settings_->getConfigPath() + "/memory_channels.json";
    memoryManager_->saveToFile(memoryFile);
    
    settings_->save();
}

void MainWindow::loadSettings() {
    currentFrequency_ = settings_->getValue("frequency", 96900000.0).toDouble();
    frequencyDial_->setFrequency(currentFrequency_);
    
    modeSelector_->setCurrentIndex(settings_->getValue("mode", 2).toInt());
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
    
    dynamicBandwidthCheck_->setChecked(settings_->getValue("dynamic_bandwidth", true).toBool());
    
    // Load memory channels
    QString memoryFile = settings_->getConfigPath() + "/memory_channels.json";
    if (QFile::exists(memoryFile)) {
        memoryManager_->loadFromFile(memoryFile);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    
    if (isRunning_) {
        stopRadio();
    }
    
    event->accept();
}

void MainWindow::onDynamicBandwidthChanged(bool checked) {
    if (dspEngine_) {
        dspEngine_->setDynamicBandwidth(checked);
    }
    
    // Handle timer start/stop if radio is running
    if (isRunning_) {
        QList<QTimer*> timers = findChildren<QTimer*>();
        for (QTimer* timer : timers) {
            if (timer->property("bandwidthUpdateTimer").toBool()) {
                timer->stop();
                timer->deleteLater();
            }
        }
        
        if (checked) {
            QTimer* bandwidthTimer = new QTimer(this);
            connect(bandwidthTimer, &QTimer::timeout, this, &MainWindow::updateBandwidthDisplay);
            bandwidthTimer->start(500); // Update every 500ms
            bandwidthTimer->setProperty("bandwidthUpdateTimer", true);
        }
    }
    
    updateBandwidthDisplay();
}

void MainWindow::updateBandwidthDisplay() {
    if (dspEngine_) {
        uint32_t bandwidth = dspEngine_->getBandwidth();
        QString text = QString("Bandwidth: %1 kHz").arg(bandwidth / 1000.0, 0, 'f', 1);
        if (bandwidthLabel_) {
            bandwidthLabel_->setText(text);
        }
    }
}

void MainWindow::onBiasTChanged(bool checked) {
    if (rtlsdr_->isOpen()) {
        rtlsdr_->setBiasT(checked);
        updateStatus(checked ? tr("Bias-T enabled") : tr("Bias-T disabled"));
    }
}

void MainWindow::applyOptimalGain(double frequency) {
    // Get optimal settings for the frequency
    auto optimalSettings = RTLSDRDevice::getOptimalSettings(frequency);
    
    // Convert from tenths of dB to dB for the UI
    double optimalGainDb = optimalSettings.gain / 10.0;
    
    // Update gain knob
    gainKnob_->setValue(optimalGainDb);
    
    // Apply gain if device is open
    if (rtlsdr_->isOpen()) {
        rtlsdr_->setGain(optimalSettings.gain);
    }
    
    // Update status with band info
    updateStatus(tr("Tuned to %1 - %2")
                .arg(frequency / 1e6, 0, 'f', 3)
                .arg(QString::fromStdString(optimalSettings.description)));
}

void MainWindow::onPpmChanged(int value) {
    if (rtlsdr_->isOpen()) {
        rtlsdr_->setFrequencyCorrection(value);
        updateStatus(tr("PPM correction set to %1").arg(value));
    }
}

void MainWindow::onRtlSampleRateChanged(int index) {
    const uint32_t rtlRates[] = {2048000, 2400000, 2560000, 3200000};
    if (index >= 0 && index < 4) {
        if (rtlsdr_->isOpen()) {
            rtlsdr_->setSampleRate(rtlRates[index]);
            dspEngine_->setSampleRate(rtlRates[index]);
            updateStatus(tr("RTL-SDR sample rate set to %1 MHz").arg(rtlRates[index] / 1e6, 0, 'f', 1));
        }
    }
}

void MainWindow::createMemoryPanel() {
    auto* memoryGroup = new QGroupBox(tr("MEMORY CHANNELS"));
    memoryGroup->setObjectName("memoryPanel");
    auto* memoryLayout = new QGridLayout(memoryGroup);
    
    // Quick channels
    memoryLayout->addWidget(new QLabel(tr("Quick Access:")), 0, 0);
    quickChannelCombo_ = new QComboBox();
    quickChannelCombo_->addItem(tr("-- Select Quick Channel --"));
    auto quickChannels = memoryManager_->getQuickChannels();
    for (const auto& ch : quickChannels) {
        quickChannelCombo_->addItem(QString("%1 - %2 MHz")
            .arg(ch.name())
            .arg(ch.frequency() / 1e6, 0, 'f', 3));
    }
    memoryLayout->addWidget(quickChannelCombo_, 0, 1, 1, 2);
    
    // Memory bank/channel selection
    memoryLayout->addWidget(new QLabel(tr("Bank:")), 1, 0);
    memoryBankCombo_ = new QComboBox();
    for (int i = 0; i < MemoryChannelManager::NUM_BANKS; i++) {
        memoryBankCombo_->addItem(QString("Bank %1").arg(i));
    }
    memoryLayout->addWidget(memoryBankCombo_, 1, 1);
    
    memoryLayout->addWidget(new QLabel(tr("Channel:")), 2, 0);
    memoryChannelSpin_ = new QSpinBox();
    memoryChannelSpin_->setRange(0, MemoryChannelManager::CHANNELS_PER_BANK - 1);
    memoryChannelSpin_->setWrapping(true);
    memoryLayout->addWidget(memoryChannelSpin_, 2, 1);
    
    // Memory buttons
    memoryStoreButton_ = new QPushButton(tr("STORE"));
    memoryStoreButton_->setObjectName("memoryButton");
    memoryStoreButton_->setToolTip(tr("Store current frequency to selected memory channel"));
    memoryLayout->addWidget(memoryStoreButton_, 1, 2);
    
    memoryRecallButton_ = new QPushButton(tr("RECALL"));
    memoryRecallButton_->setObjectName("memoryButton");
    memoryRecallButton_->setToolTip(tr("Recall frequency from selected memory channel"));
    memoryLayout->addWidget(memoryRecallButton_, 2, 2);
    
    memoryClearButton_ = new QPushButton(tr("CLEAR"));
    memoryClearButton_->setObjectName("memoryButton");
    memoryClearButton_->setToolTip(tr("Clear selected memory channel"));
    memoryLayout->addWidget(memoryClearButton_, 3, 2);
    
    // Memory info display
    auto* memoryInfoLabel = new QLabel(tr("Memory: Empty"));
    memoryInfoLabel->setObjectName("memoryInfo");
    memoryLayout->addWidget(memoryInfoLabel, 3, 0, 1, 2);
    
    centralWidget()->layout()->addWidget(memoryGroup);
}

void MainWindow::onThemeChanged(int theme) {
    currentTheme_ = theme;
    VintageTheme::applyTheme(this, static_cast<VintageTheme::Theme>(theme));
    
    // Save theme preference
    settings_->setValue("theme", theme);
    
    // Update status
    updateStatus(tr("Theme changed to %1").arg(VintageTheme::getThemeName(static_cast<VintageTheme::Theme>(theme))));
}

void MainWindow::onMemoryStore() {
    int bank = memoryBankCombo_->currentIndex();
    int channel = memoryChannelSpin_->value();
    int index = bank * MemoryChannelManager::CHANNELS_PER_BANK + channel;
    
    // Create memory channel with current settings
    MemoryChannel mem(index, currentFrequency_);
    mem.setName(QString("CH %1-%2").arg(bank).arg(channel));
    mem.setMode(modeSelector_->currentText());
    mem.setBandwidth(dspEngine_->getBandwidth());
    mem.setGain(gainKnob_->value());
    mem.setSquelch(squelchKnob_->value());
    
    memoryManager_->setChannel(index, mem);
    
    updateStatus(tr("Stored %1 MHz to memory %2-%3")
                .arg(currentFrequency_ / 1e6, 0, 'f', 3)
                .arg(bank)
                .arg(channel));
    
    onMemoryChannelChanged();
}

void MainWindow::onMemoryRecall() {
    int bank = memoryBankCombo_->currentIndex();
    int channel = memoryChannelSpin_->value();
    int index = bank * MemoryChannelManager::CHANNELS_PER_BANK + channel;
    
    MemoryChannel mem = memoryManager_->getChannel(index);
    if (mem.isEmpty()) {
        updateStatus(tr("Memory %1-%2 is empty").arg(bank).arg(channel));
        return;
    }
    
    // Recall all settings
    onFrequencyChanged(mem.frequency());
    
    // Find and set mode
    int modeIndex = modeSelector_->findText(mem.mode());
    if (modeIndex >= 0) {
        modeSelector_->setCurrentIndex(modeIndex);
    }
    
    gainKnob_->setValue(mem.gain());
    squelchKnob_->setValue(mem.squelch());
    
    updateStatus(tr("Recalled %1 from memory %2-%3")
                .arg(mem.name())
                .arg(bank)
                .arg(channel));
}

void MainWindow::onMemoryClear() {
    int bank = memoryBankCombo_->currentIndex();
    int channel = memoryChannelSpin_->value();
    int index = bank * MemoryChannelManager::CHANNELS_PER_BANK + channel;
    
    memoryManager_->clearChannel(index);
    
    updateStatus(tr("Cleared memory %1-%2").arg(bank).arg(channel));
    onMemoryChannelChanged();
}

void MainWindow::onMemoryChannelChanged() {
    int bank = memoryBankCombo_->currentIndex();
    int channel = memoryChannelSpin_->value();
    int index = bank * MemoryChannelManager::CHANNELS_PER_BANK + channel;
    
    MemoryChannel mem = memoryManager_->getChannel(index);
    
    // Update memory info label
    auto* memoryInfoLabel = findChild<QLabel*>("memoryInfo");
    if (memoryInfoLabel) {
        if (mem.isEmpty()) {
            memoryInfoLabel->setText(tr("Memory %1-%2: Empty").arg(bank).arg(channel));
        } else {
            memoryInfoLabel->setText(tr("Memory %1-%2: %3 - %4 MHz")
                                   .arg(bank)
                                   .arg(channel)
                                   .arg(mem.name())
                                   .arg(mem.frequency() / 1e6, 0, 'f', 3));
        }
    }
}

void MainWindow::onQuickChannelSelected(int index) {
    if (index <= 0) return; // Skip the "-- Select --" item
    
    auto quickChannels = memoryManager_->getQuickChannels();
    if (index - 1 < static_cast<int>(quickChannels.size())) {
        const auto& ch = quickChannels[index - 1];
        onFrequencyChanged(ch.frequency());
        updateStatus(tr("Tuned to %1").arg(ch.name()));
        
        // Reset combo box to show "-- Select --"
        quickChannelCombo_->setCurrentIndex(0);
    }
}
