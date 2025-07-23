# Adding New Digital Decoders

This guide explains how to add new digital decoders to the Vintage Tactical Radio app.

## Example: Adding a DCS (Digital Coded Squelch) Decoder

### Step 1: Create the Decoder Class

```cpp
// src/decoders/DCSDecoder.h
#ifndef DCSDECODER_H
#define DCSDECODER_H

#include "DigitalDecoder.h"
#include <array>

class DCSDecoder : public DigitalDecoder {
    Q_OBJECT
    
public:
    explicit DCSDecoder(QObject* parent = nullptr);
    ~DCSDecoder() override;
    
    // Control methods
    void start() override;
    void stop() override;
    void reset() override;
    
    // Process audio samples
    void processAudio(const float* samples, size_t length) override;
    
    // DCS uses 23-bit Golay codes
    static constexpr int DCS_CODE_LENGTH = 23;
    static constexpr float DCS_BITRATE = 134.3f; // bits per second
    
signals:
    void codeDetected(int code, bool inverted);
    void codeLost();
    
private:
    // DCS detection logic
    void detectDCSCode(const float* samples, size_t length);
    bool validateGolay(uint32_t code);
    
    // Current state
    int currentCode_;
    bool codeInverted_;
    float bitPhase_;
    uint32_t shiftRegister_;
};

#endif // DCSDECODER_H
```

### Step 2: Implement the Decoder

```cpp
// src/decoders/DCSDecoder.cpp
#include "DCSDecoder.h"
#include <cmath>

DCSDecoder::DCSDecoder(QObject* parent)
    : DigitalDecoder(DecoderType::DCS, parent)
    , currentCode_(0)
    , codeInverted_(false)
    , bitPhase_(0.0f)
    , shiftRegister_(0) {
}

void DCSDecoder::start() {
    if (active_) return;
    
    active_ = true;
    setState(DecoderState::SEARCHING);
    reset();
}

void DCSDecoder::processAudio(const float* samples, size_t length) {
    if (!active_) return;
    
    // Extract DCS bits from audio
    detectDCSCode(samples, length);
}

// ... implement detection logic ...
```

### Step 3: Add to DSPEngine

```cpp
// In DSPEngine.h
private:
    std::unique_ptr<DCSDecoder> dcsDecoder_;
    bool dcsEnabled_;

public:
    DCSDecoder* getDCSDecoder() const { return dcsDecoder_.get(); }
    void enableDCS(bool enable);

// In DSPEngine.cpp constructor
dcsDecoder_ = std::make_unique<DCSDecoder>();
dcsDecoder_->setSampleRate(audioSampleRate_);

// Add enable method
void DSPEngine::enableDCS(bool enable) {
    dcsEnabled_ = enable;
    if (dcsDecoder_) {
        if (enable) {
            dcsDecoder_->start();
        } else {
            dcsDecoder_->stop();
        }
    }
}

// In processingWorker(), send audio to DCS decoder
if (dcsEnabled_ && dcsDecoder_) {
    dcsDecoder_->processAudio(decimatedAudio.data(), decimatedSamples);
}
```

### Step 4: Add UI Support

```cpp
// In DecoderWidget, add a new tab
void DecoderWidget::createDCSTab() {
    auto* dcsWidget = new QWidget();
    auto* layout = new QVBoxLayout(dcsWidget);
    
    // Enable checkbox
    dcsEnable_ = new QCheckBox(tr("Enable DCS Detection"));
    layout->addWidget(dcsEnable_);
    
    // Status display
    auto* statusGroup = new QGroupBox(tr("DCS Status"));
    auto* statusLayout = new QGridLayout(statusGroup);
    
    statusLayout->addWidget(new QLabel(tr("Code:")), 0, 0);
    dcsCodeLabel_ = new QLabel(tr("---"));
    statusLayout->addWidget(dcsCodeLabel_, 0, 1);
    
    layout->addWidget(statusGroup);
    layout->addStretch();
    
    tabWidget_->addTab(dcsWidget, tr("DCS"));
}
```

### Step 5: Update CMakeLists.txt

```cmake
# Add to SOURCES
src/decoders/DCSDecoder.cpp

# Add to HEADERS  
src/decoders/DCSDecoder.h
```

### Step 6: Connect Everything

```cpp
// In MainWindow::createDecoderPanel()
decoderWidget_->setDCSDecoder(dspEngine_->getDCSDecoder());

// Add signal connection
connect(decoderWidget_, &DecoderWidget::dcsEnableChanged,
        [this](bool enabled) {
            if (dspEngine_) {
                dspEngine_->enableDCS(enabled);
            }
        });
```

## Decoder Implementation Tips

### 1. Choose the Right Input Type
- **Audio**: For subaudible tones (CTCSS, DCS)
- **IQ Data**: For separate carriers (RDS at 57kHz)
- **Raw Data**: For digital signals (ADS-B, POCSAG)

### 2. Optimize for Real-time
- Use efficient algorithms (Goertzel for single frequencies)
- Process in blocks for better cache usage
- Minimize memory allocations in processing loops

### 3. Handle Edge Cases
- Weak signals with noise
- Frequency drift
- Multiple simultaneous codes
- False positives

### 4. Provide Good UI Feedback
- Signal strength indicators
- Detection confidence levels
- History logs for intermittent signals
- Clear error messages

### 5. Test Thoroughly
- Use recorded samples for consistent testing
- Test with weak and strong signals
- Verify at different sample rates
- Check CPU usage

## Common Decoder Types to Consider

1. **POCSAG** - Pager protocol
   - Frequency: Various (commonly 450-470 MHz)
   - Modulation: FSK
   - Data rate: 512/1200/2400 bps

2. **FLEX** - Advanced pager protocol
   - Frequency: 929-932 MHz
   - Modulation: 4-FSK
   - Data rate: 1600/3200/6400 bps

3. **APRS** - Amateur Packet Reporting System
   - Frequency: 144.39 MHz (US)
   - Modulation: AFSK 1200 baud
   - Protocol: AX.25

4. **SAME** - Emergency Alert System
   - Frequency: Various FM/AM stations
   - Modulation: AFSK 520.83 baud
   - Used for weather alerts

5. **DMR** - Digital Mobile Radio
   - Frequency: Various
   - Modulation: 4FSK
   - Requires more complex processing

## Conclusion

Adding new decoders follows a consistent pattern:
1. Inherit from DigitalDecoder
2. Implement the processing logic
3. Add to DSPEngine
4. Create UI components
5. Connect everything together

The modular architecture makes it straightforward to extend the decoder capabilities of the Vintage Tactical Radio app.
