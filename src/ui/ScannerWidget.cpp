#include "ScannerWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QProgressBar>
#include <QGroupBox>

ScannerWidget::ScannerWidget(QWidget* parent)
    : QWidget(parent)
    , scanner_(nullptr)
    , isScanning_(false)
    , currentMode_(Scanner::ScanMode::OFF) {
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    
    // Control row
    auto* controlLayout = new QHBoxLayout();
    
    scanButton_ = new QPushButton(tr("SCAN"), this);
    scanButton_->setObjectName("scanButton");
    scanButton_->setCheckable(true);
    scanButton_->setFixedWidth(80);
    scanButton_->setToolTip(tr("Start/Stop scanning"));
    connect(scanButton_, &QPushButton::clicked, this, &ScannerWidget::onScanButtonClicked);
    controlLayout->addWidget(scanButton_);
    
    modeCombo_ = new QComboBox(this);
    modeCombo_->addItems({tr("Frequency"), tr("Channel"), tr("Memory"), tr("Band")});
    modeCombo_->setToolTip(tr("Scan mode"));
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScannerWidget::onModeChanged);
    controlLayout->addWidget(modeCombo_);
    
    skipButton_ = new QPushButton(tr("SKIP"), this);
    skipButton_->setFixedWidth(60);
    skipButton_->setEnabled(false);
    skipButton_->setToolTip(tr("Skip current channel"));
    connect(skipButton_, &QPushButton::clicked, this, &ScannerWidget::onSkipClicked);
    controlLayout->addWidget(skipButton_);
    
    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);
    
    // Parameters row
    auto* paramLayout = new QGridLayout();
    
    // Step size
    paramLayout->addWidget(new QLabel(tr("Step:")), 0, 0);
    stepCombo_ = new QComboBox(this);
    stepCombo_->addItems({"5 kHz", "6.25 kHz", "10 kHz", "12.5 kHz", 
                         "25 kHz", "50 kHz", "100 kHz", "200 kHz"});
    stepCombo_->setCurrentIndex(6); // 100 kHz default
    stepCombo_->setToolTip(tr("Frequency step size"));
    connect(stepCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScannerWidget::onStepSizeChanged);
    paramLayout->addWidget(stepCombo_, 0, 1);
    
    // Scan speed
    paramLayout->addWidget(new QLabel(tr("Speed:")), 0, 2);
    speedSlider_ = new QSlider(Qt::Horizontal, this);
    speedSlider_->setRange(1, 50);
    speedSlider_->setValue(10);
    speedSlider_->setFixedWidth(100);
    speedSlider_->setToolTip(tr("Scan speed (channels/second)"));
    connect(speedSlider_, &QSlider::valueChanged, this, &ScannerWidget::onSpeedChanged);
    paramLayout->addWidget(speedSlider_, 0, 3);
    
    speedLabel_ = new QLabel(tr("10 ch/s"), this);
    speedLabel_->setFixedWidth(50);
    paramLayout->addWidget(speedLabel_, 0, 4);
    
    // Signal threshold
    paramLayout->addWidget(new QLabel(tr("Threshold:")), 1, 0);
    thresholdSlider_ = new QSlider(Qt::Horizontal, this);
    thresholdSlider_->setRange(-100, -20);
    thresholdSlider_->setValue(-60);
    thresholdSlider_->setFixedWidth(150);
    thresholdSlider_->setToolTip(tr("Signal threshold (dB)"));
    connect(thresholdSlider_, &QSlider::valueChanged, this, &ScannerWidget::onThresholdChanged);
    paramLayout->addWidget(thresholdSlider_, 1, 1, 1, 3);
    
    thresholdLabel_ = new QLabel(tr("-60 dB"), this);
    thresholdLabel_->setFixedWidth(50);
    paramLayout->addWidget(thresholdLabel_, 1, 4);
    
    mainLayout->addLayout(paramLayout);
    
    // Status row
    auto* statusLayout = new QHBoxLayout();
    
    statusLabel_ = new QLabel(tr("Ready"), this);
    statusLabel_->setObjectName("scanStatus");
    statusLayout->addWidget(statusLabel_);
    
    frequencyLabel_ = new QLabel(tr("---.--- MHz"), this);
    frequencyLabel_->setObjectName("scanFrequency");
    frequencyLabel_->setAlignment(Qt::AlignCenter);
    frequencyLabel_->setFixedWidth(100);
    statusLayout->addWidget(frequencyLabel_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(15);
    statusLayout->addWidget(progressBar_);
    
    mainLayout->addLayout(statusLayout);
    
    // Apply styling
    setStyleSheet(R"(
        QPushButton#scanButton {
            font-weight: bold;
            background-color: #4a4a3a;
            border: 2px solid #8a8a7a;
        }
        QPushButton#scanButton:checked {
            background-color: #00aa00;
            color: white;
            border-color: #00ff00;
        }
        QLabel#scanStatus {
            color: #aaaaaa;
        }
        QLabel#scanFrequency {
            font-family: monospace;
            font-size: 12px;
            font-weight: bold;
            color: #ffcc00;
            background-color: #2a2a1a;
            border: 1px solid #4a4a3a;
            padding: 2px;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background: #4a4a3a;
        }
        QSlider::handle:horizontal {
            background: #8a8a7a;
            width: 12px;
            margin: -4px 0;
        }
        QProgressBar {
            background-color: #2a2a1a;
            border: 1px solid #4a4a3a;
        }
        QProgressBar::chunk {
            background-color: #ffcc00;
        }
    )");
    
    updateScanButton();
}

void ScannerWidget::setScanner(Scanner* scanner) {
    if (scanner_) {
        // Disconnect old signals
        disconnect(scanner_, nullptr, this, nullptr);
    }
    
    scanner_ = scanner;
    
    if (scanner_) {
        // Connect signals
        connect(scanner_, &Scanner::scanStarted,
                this, &ScannerWidget::onScanStarted);
        connect(scanner_, &Scanner::scanStopped,
                this, &ScannerWidget::onScanStopped);
        connect(scanner_, &Scanner::frequencyChanged,
                this, &ScannerWidget::onFrequencyChanged);
        connect(scanner_, &Scanner::channelFound,
                this, &ScannerWidget::onChannelFound);
        connect(scanner_, &Scanner::signalDetected,
                this, &ScannerWidget::onSignalDetected);
        connect(scanner_, &Scanner::scanProgress,
                this, &ScannerWidget::onScanProgress);
        
        // Update initial parameters
        Scanner::ScanParameters params;
        params.scanSpeedHz = speedSlider_->value();
        params.signalThreshold = thresholdSlider_->value();
        scanner_->setScanParameters(params);
    }
}

void ScannerWidget::setMemoryChannels(const std::vector<Scanner::Channel>& channels) {
    if (scanner_) {
        scanner_->setMemoryChannels(channels);
    }
}

void ScannerWidget::onScanButtonClicked() {
    if (!scanner_) {
        return;
    }
    
    if (!isScanning_) {
        // Start scanning
        Scanner::ScanMode mode = static_cast<Scanner::ScanMode>(modeCombo_->currentIndex() + 1);
        scanner_->startScan(mode);
    } else {
        // Stop scanning
        scanner_->stopScan();
    }
}

void ScannerWidget::onModeChanged(int index) {
    currentMode_ = static_cast<Scanner::ScanMode>(index + 1);
    emit scanModeChanged(currentMode_);
    
    // Enable/disable step size based on mode
    bool enableStep = (currentMode_ == Scanner::ScanMode::FREQUENCY || 
                      currentMode_ == Scanner::ScanMode::BAND);
    stepCombo_->setEnabled(enableStep);
}

void ScannerWidget::onStepSizeChanged(int index) {
    if (!scanner_) {
        return;
    }
    
    const double stepSizes[] = {5e3, 6.25e3, 10e3, 12.5e3, 25e3, 50e3, 100e3, 200e3};
    
    if (index >= 0 && index < 8) {
        Scanner::ScanParameters params;
        scanner_->setScanParameters(params); // Get current
        params.stepSize = stepSizes[index];
        scanner_->setScanParameters(params);
        emit scanParametersChanged();
    }
}

void ScannerWidget::onSpeedChanged(int value) {
    speedLabel_->setText(QString("%1 ch/s").arg(value));
    
    if (scanner_) {
        Scanner::ScanParameters params;
        scanner_->setScanParameters(params); // Get current
        params.scanSpeedHz = value;
        scanner_->setScanParameters(params);
        emit scanParametersChanged();
    }
}

void ScannerWidget::onThresholdChanged(int value) {
    thresholdLabel_->setText(QString("%1 dB").arg(value));
    
    if (scanner_) {
        Scanner::ScanParameters params;
        scanner_->setScanParameters(params); // Get current
        params.signalThreshold = value;
        scanner_->setScanParameters(params);
        emit scanParametersChanged();
    }
}

void ScannerWidget::onSkipClicked() {
    if (scanner_ && isScanning_) {
        scanner_->skipChannel();
    }
}

void ScannerWidget::onScanStarted(Scanner::ScanMode mode) {
    Q_UNUSED(mode);
    isScanning_ = true;
    skipButton_->setEnabled(true);
    modeCombo_->setEnabled(false);
    statusLabel_->setText(tr("Scanning..."));
    updateScanButton();
}

void ScannerWidget::onScanStopped() {
    isScanning_ = false;
    skipButton_->setEnabled(false);
    modeCombo_->setEnabled(true);
    statusLabel_->setText(tr("Stopped"));
    progressBar_->setValue(0);
    scanButton_->setChecked(false);
    updateScanButton();
}

void ScannerWidget::onFrequencyChanged(double frequency) {
    frequencyLabel_->setText(QString("%1 MHz").arg(frequency / 1e6, 0, 'f', 3));
}

void ScannerWidget::onChannelFound(double frequency, const QString& name) {
    Q_UNUSED(frequency);
    statusLabel_->setText(tr("Channel: %1").arg(name));
}

void ScannerWidget::onSignalDetected(double frequency, double strength) {
    statusLabel_->setText(tr("Signal found! %1 dB").arg(strength, 0, 'f', 1));
    frequencyLabel_->setText(QString("%1 MHz").arg(frequency / 1e6, 0, 'f', 3));
}

void ScannerWidget::onScanProgress(int percent) {
    progressBar_->setValue(percent);
}

void ScannerWidget::updateScanButton() {
    if (isScanning_) {
        scanButton_->setText(tr("STOP"));
    } else {
        scanButton_->setText(tr("SCAN"));
    }
}

void ScannerWidget::updateControls() {
    // Update controls based on scanner state
    if (scanner_) {
        // Could update UI to reflect scanner's current parameters
    }
}
