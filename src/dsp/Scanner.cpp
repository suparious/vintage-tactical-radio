#include "Scanner.h"
#include "../core/RTLSDRDevice.h"
#include "../core/DSPEngine.h"
#include <algorithm>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

Scanner::Scanner(QObject* parent)
    : QObject(parent)
    , rtlsdr_(nullptr)
    , dspEngine_(nullptr)
    , isScanning_(false)
    , isPaused_(false)
    , currentMode_(ScanMode::OFF)
    , scanDirection_(ScanDirection::UP)
    , currentFrequency_(0)
    , currentChannelIndex_(0)
    , lastSignalStrength_(-100)
    , noiseFloor_(-80)
    , signalDetectCount_(0)
    , priorityCheckInterval_(2000)
    , returningFromPriority_(false)
    , savedFrequency_(0) {
    
    // Initialize scan parameters with defaults
    params_.startFreq = 88e6;     // 88 MHz
    params_.endFreq = 108e6;      // 108 MHz
    params_.stepSize = 100e3;     // 100 kHz
    params_.dwellTimeMs = 2000;   // 2 seconds
    params_.resumeTimeMs = 3000;  // 3 seconds
    params_.signalThreshold = -60; // -60 dB
    params_.scanSpeedHz = 10;     // 10 channels/second
    
    // Setup timers
    scanTimer_ = new QTimer(this);
    connect(scanTimer_, &QTimer::timeout, this, &Scanner::onScanTimer);
    
    dwellTimer_ = new QTimer(this);
    dwellTimer_->setSingleShot(true);
    connect(dwellTimer_, &QTimer::timeout, this, &Scanner::resumeScan);
    
    priorityTimer_ = new QTimer(this);
    connect(priorityTimer_, &QTimer::timeout, this, &Scanner::onPriorityTimer);
}

Scanner::~Scanner() {
    stopScan();
}

void Scanner::setScanParameters(const ScanParameters& params) {
    params_ = params;
    
    // Update scan timer interval based on speed
    if (params_.scanSpeedHz > 0) {
        int intervalMs = 1000 / params_.scanSpeedHz;
        scanTimer_->setInterval(intervalMs);
    }
}

void Scanner::setChannels(const std::vector<Channel>& channels) {
    channels_ = channels;
}

void Scanner::setMemoryChannels(const std::vector<Channel>& channels) {
    memoryChannels_ = channels;
}

void Scanner::startScan(ScanMode mode, ScanDirection direction) {
    if (isScanning_) {
        stopScan();
    }
    
    currentMode_ = mode;
    scanDirection_ = direction;
    isScanning_ = true;
    isPaused_ = false;
    signalDetectCount_ = 0;
    
    // Set initial frequency based on mode
    switch (mode) {
        case ScanMode::FREQUENCY:
            currentFrequency_ = (direction == ScanDirection::UP) ? 
                params_.startFreq : params_.endFreq;
            break;
            
        case ScanMode::CHANNEL:
            currentChannelIndex_ = (direction == ScanDirection::UP) ? 
                0 : channels_.size() - 1;
            if (!channels_.empty()) {
                currentFrequency_ = channels_[currentChannelIndex_].frequency;
            }
            break;
            
        case ScanMode::MEMORY:
            currentChannelIndex_ = (direction == ScanDirection::UP) ? 
                0 : memoryChannels_.size() - 1;
            if (!memoryChannels_.empty()) {
                currentFrequency_ = memoryChannels_[currentChannelIndex_].frequency;
            }
            break;
            
        case ScanMode::BAND:
            // Similar to frequency scan but with band limits
            currentFrequency_ = (direction == ScanDirection::UP) ? 
                params_.startFreq : params_.endFreq;
            break;
            
        default:
            break;
    }
    
    // Connect to DSP signal strength callback
    if (dspEngine_) {
        dspEngine_->setSignalCallback([this](float strength) {
            onSignalStrength(strength);
        });
    }
    
    // Start scan timer
    scanTimer_->start();
    
    // Start priority timer if we have priority channels
    if (!priorityChannels_.empty()) {
        priorityTimer_->start(priorityCheckInterval_);
    }
    
    emit scanStarted(mode);
    emit frequencyChanged(currentFrequency_);
    
#ifdef HAS_SPDLOG
    spdlog::info("Scanner started - Mode: {}, Direction: {}", 
                static_cast<int>(mode), 
                (direction == ScanDirection::UP) ? "UP" : "DOWN");
#endif
}

void Scanner::stopScan() {
    if (!isScanning_) {
        return;
    }
    
    isScanning_ = false;
    isPaused_ = false;
    currentMode_ = ScanMode::OFF;
    
    scanTimer_->stop();
    dwellTimer_->stop();
    priorityTimer_->stop();
    
    emit scanStopped();
    
#ifdef HAS_SPDLOG
    spdlog::info("Scanner stopped");
#endif
}

void Scanner::pauseScan() {
    if (isScanning_ && !isPaused_) {
        isPaused_ = true;
        scanTimer_->stop();
        dwellTimer_->stop();
    }
}

void Scanner::resumeScan() {
    if (isScanning_ && isPaused_) {
        isPaused_ = false;
        signalDetectCount_ = 0;
        scanTimer_->start();
    }
}

void Scanner::skipChannel() {
    if (isScanning_ && dwellTimer_->isActive()) {
        dwellTimer_->stop();
        resumeScan();
    }
}

void Scanner::onScanTimer() {
    if (isPaused_) {
        return;
    }
    
    switch (currentMode_) {
        case ScanMode::FREQUENCY:
        case ScanMode::BAND:
            scanNextFrequency();
            break;
            
        case ScanMode::CHANNEL:
        case ScanMode::MEMORY:
            scanNextChannel();
            break;
            
        default:
            break;
    }
}

void Scanner::scanNextFrequency() {
    // Calculate next frequency
    if (scanDirection_ == ScanDirection::UP) {
        currentFrequency_ += params_.stepSize;
        if (currentFrequency_ > params_.endFreq) {
            currentFrequency_ = params_.startFreq;
        }
    } else {
        currentFrequency_ -= params_.stepSize;
        if (currentFrequency_ < params_.startFreq) {
            currentFrequency_ = params_.endFreq;
        }
    }
    
    // Set frequency
    if (rtlsdr_ && rtlsdr_->isOpen()) {
        rtlsdr_->setCenterFrequency(currentFrequency_);
    }
    
    emit frequencyChanged(currentFrequency_);
    
    // Calculate and emit progress
    double range = params_.endFreq - params_.startFreq;
    double position = currentFrequency_ - params_.startFreq;
    int progress = static_cast<int>((position / range) * 100);
    emit scanProgress(progress);
}

void Scanner::scanNextChannel() {
    const auto& channelList = (currentMode_ == ScanMode::CHANNEL) ? 
        channels_ : memoryChannels_;
    
    if (channelList.empty()) {
        return;
    }
    
    // Calculate next channel index
    if (scanDirection_ == ScanDirection::UP) {
        currentChannelIndex_ = (currentChannelIndex_ + 1) % channelList.size();
    } else {
        if (currentChannelIndex_ == 0) {
            currentChannelIndex_ = channelList.size() - 1;
        } else {
            currentChannelIndex_--;
        }
    }
    
    // Set frequency
    const auto& channel = channelList[currentChannelIndex_];
    currentFrequency_ = channel.frequency;
    
    if (rtlsdr_ && rtlsdr_->isOpen()) {
        rtlsdr_->setCenterFrequency(currentFrequency_);
    }
    
    emit frequencyChanged(currentFrequency_);
    emit channelFound(currentFrequency_, channel.name);
    
    // Emit progress
    int progress = static_cast<int>(
        (static_cast<double>(currentChannelIndex_) / channelList.size()) * 100);
    emit scanProgress(progress);
}

void Scanner::onSignalStrength(float strength) {
    lastSignalStrength_ = strength;
    
    // Check if signal is active
    if (isSignalActive(strength)) {
        signalDetectCount_++;
        
        if (signalDetectCount_ >= SIGNAL_DETECT_THRESHOLD) {
            // Signal detected - pause scan
            pauseScan();
            
            emit signalDetected(currentFrequency_, strength);
            
            // Start dwell timer
            dwellTimer_->start(params_.dwellTimeMs);
            
#ifdef HAS_SPDLOG
            spdlog::debug("Signal detected at {} MHz, strength: {} dB", 
                         currentFrequency_ / 1e6, strength);
#endif
        }
    } else {
        signalDetectCount_ = 0;
    }
    
    // Update noise floor estimate (simple moving average)
    if (strength < noiseFloor_ + 10) {
        noiseFloor_ = 0.9 * noiseFloor_ + 0.1 * strength;
    }
}

bool Scanner::isSignalActive(double strength) {
    // Signal is active if above threshold and significantly above noise floor
    return (strength > params_.signalThreshold) && 
           (strength > noiseFloor_ + 10);
}

void Scanner::onDwellTimer() {
    // Dwell time expired - check if signal still active
    if (isSignalActive(lastSignalStrength_)) {
        // Signal still active - continue dwelling
        dwellTimer_->start(params_.resumeTimeMs);
    } else {
        // Signal gone - resume scanning
        resumeScan();
    }
}

void Scanner::onPriorityTimer() {
    if (!isScanning_ || isPaused_ || priorityChannels_.empty()) {
        return;
    }
    
    checkPriorityChannels();
}

void Scanner::checkPriorityChannels() {
    // Save current frequency
    if (!returningFromPriority_) {
        savedFrequency_ = currentFrequency_;
    }
    
    // Check each priority channel
    for (const auto& channel : priorityChannels_) {
        if (rtlsdr_ && rtlsdr_->isOpen()) {
            rtlsdr_->setCenterFrequency(channel.frequency);
            
            // Wait a bit for signal to settle
            QTimer::singleShot(50, [this, channel]() {
                if (isSignalActive(lastSignalStrength_)) {
                    // Priority channel active
                    pauseScan();
                    currentFrequency_ = channel.frequency;
                    emit frequencyChanged(currentFrequency_);
                    emit channelFound(currentFrequency_, channel.name);
                    emit signalDetected(currentFrequency_, lastSignalStrength_);
                    
                    returningFromPriority_ = true;
                    dwellTimer_->start(params_.dwellTimeMs);
                }
            });
        }
    }
    
    // Return to saved frequency if no priority activity
    if (returningFromPriority_ && !dwellTimer_->isActive()) {
        currentFrequency_ = savedFrequency_;
        if (rtlsdr_ && rtlsdr_->isOpen()) {
            rtlsdr_->setCenterFrequency(currentFrequency_);
        }
        emit frequencyChanged(currentFrequency_);
        returningFromPriority_ = false;
    }
}

void Scanner::addPriorityChannel(double frequency, int level) {
    Channel ch;
    ch.frequency = frequency;
    ch.name = QString("Priority %1").arg(frequency / 1e6, 0, 'f', 3);
    ch.priority = true;
    ch.priorityLevel = level;
    
    priorityChannels_.push_back(ch);
    
    // Sort by priority level
    std::sort(priorityChannels_.begin(), priorityChannels_.end(),
              [](const Channel& a, const Channel& b) {
                  return a.priorityLevel > b.priorityLevel;
              });
}

void Scanner::removePriorityChannel(double frequency) {
    priorityChannels_.erase(
        std::remove_if(priorityChannels_.begin(), priorityChannels_.end(),
                      [frequency](const Channel& ch) {
                          return std::abs(ch.frequency - frequency) < 1000;
                      }),
        priorityChannels_.end()
    );
}

QString Scanner::getCurrentChannel() const {
    const auto& channelList = (currentMode_ == ScanMode::CHANNEL) ? 
        channels_ : memoryChannels_;
    
    if (currentMode_ == ScanMode::CHANNEL || currentMode_ == ScanMode::MEMORY) {
        if (currentChannelIndex_ < channelList.size()) {
            return channelList[currentChannelIndex_].name;
        }
    }
    
    return QString("%1 MHz").arg(currentFrequency_ / 1e6, 0, 'f', 3);
}
