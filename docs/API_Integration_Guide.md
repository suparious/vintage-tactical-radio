# API and Integration Guide
## Vintage Tactical Radio - Component Integration Specification

### System Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     Qt6 Application Layer                    │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │   GUI/QML   │  │ Audio Engine │  │  State Manager  │   │
│  └──────┬──────┘  └──────┬───────┘  └────────┬────────┘   │
├─────────┼─────────────────┼──────────────────┼─────────────┤
│         │    C++ Bridge Layer (Signals/Slots) │             │
├─────────┼─────────────────┼──────────────────┼─────────────┤
│  ┌──────▼──────┐  ┌──────▼───────┐  ┌───────▼────────┐   │
│  │ RTL-SDR API │  │  DSP Engine  │  │ Audio Backend  │   │
│  │  (librtlsdr)│  │ (liquid-dsp) │  │ (PortAudio)    │   │
│  └─────────────┘  └──────────────┘  └────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Core API Interfaces

#### 1. RTL-SDR Hardware Interface

```cpp
class RTLSDRInterface : public QObject {
    Q_OBJECT
public:
    // Device enumeration and selection
    QStringList getAvailableDevices();
    bool openDevice(int deviceIndex);
    void closeDevice();
    
    // Frequency control
    bool setFrequency(uint32_t freq_hz);
    uint32_t getFrequency();
    
    // Gain control
    bool setGain(int gain_db); // 0-49 dB for RTL-SDR v3
    int getGain();
    void setAutoGain(bool enable);
    
    // Sampling control
    bool setSampleRate(uint32_t rate);
    uint32_t getSampleRate();
    
    // Direct sampling mode for HF
    bool setDirectSampling(int mode); // 0=disabled, 1=I, 2=Q
    
    // Bias tee control (RTL-SDR v3)
    bool setBiasTee(bool enable);
    
signals:
    void samplesReady(const QVector<std::complex<float>>& samples);
    void deviceError(const QString& error);
    void frequencyChanged(uint32_t freq);
    
private:
    rtlsdr_dev_t* device;
    QThread* workerThread;
    std::atomic<bool> running;
};
```

#### 2. DSP Processing Pipeline

```cpp
class DSPEngine : public QObject {
    Q_OBJECT
public:
    enum DemodMode {
        AM,
        FM_NARROW,
        FM_WIDE,
        USB,
        LSB,
        CW
    };
    
    // Demodulation control
    void setDemodMode(DemodMode mode);
    void setBandwidth(float bandwidth_hz);
    
    // Filtering
    void setLowPass(float cutoff_hz);
    void setHighPass(float cutoff_hz);
    void setNotchFilter(float freq_hz, float width_hz);
    
    // AGC control
    void setAGC(bool enable);
    void setAGCSpeed(float attack_ms, float decay_ms);
    
    // Noise reduction
    void setNoiseBlanker(bool enable, float threshold);
    void setNoiseReduction(bool enable, float level);
    
    // Squelch
    void setSquelch(float threshold_db);
    
public slots:
    void processSamples(const QVector<std::complex<float>>& input);
    
signals:
    void audioReady(const QVector<float>& audio);
    void signalStrength(float dbfs);
    void spectrumData(const QVector<float>& magnitudes);
};
```

#### 3. Audio Output Interface

```cpp
class AudioOutput : public QObject {
    Q_OBJECT
public:
    // Device enumeration
    QStringList getOutputDevices();
    bool selectDevice(const QString& deviceName);
    
    // Format configuration
    bool setSampleRate(int rate); // 44100, 48000, 192000
    bool setBitDepth(int bits);   // 16 or 24
    
    // Volume control
    void setVolume(float level); // 0.0 to 1.0
    
    // Equalizer
    void setEQMode(EQMode mode); // Modern or Nostalgic
    void setEQBand(int band, float gain_db);
    void loadEQPreset(const QString& presetName);
    
public slots:
    void playAudio(const QVector<float>& samples);
    
signals:
    void deviceChanged(const QString& device);
    void bufferUnderrun();
};
```

#### 4. Frequency Database Interface

```cpp
class FrequencyDatabase : public QObject {
    Q_OBJECT
public:
    struct Station {
        QString name;
        double frequency;
        QString mode;
        QString description;
        float signalStrength;
        QDateTime lastHeard;
    };
    
    // Station management
    QList<Station> getStationsInRange(double startFreq, double endFreq);
    void addStation(const Station& station);
    void updateSignalStrength(double freq, float strength);
    
    // Favorites
    void addFavorite(double freq);
    void removeFavorite(double freq);
    QList<Station> getFavorites();
    
    // Memory channels
    void saveMemory(int channel, const Station& station);
    Station loadMemory(int channel);
    
signals:
    void stationUpdated(const Station& station);
    void databaseChanged();
};
```

### Plugin System Architecture

```cpp
class RadioPlugin : public QObject {
    Q_OBJECT
public:
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QWidget* createUI() = 0;
    
    // Processing hooks
    virtual void processIQ(std::complex<float>* data, size_t count) {}
    virtual void processAudio(float* data, size_t count) {}
    virtual void processSpectrum(float* magnitudes, size_t count) {}
    
    // Metadata
    virtual QJsonObject getCapabilities() = 0;
};

// Example: ADS-B decoder plugin
class ADSBPlugin : public RadioPlugin {
public:
    QString name() const override { return "ADS-B Decoder"; }
    QString version() const override { return "1.0"; }
    QWidget* createUI() override;
    void processIQ(std::complex<float>* data, size_t count) override;
    
signals:
    void aircraftDetected(const QString& icao, float lat, float lon, int altitude);
};
```

### IPC and Remote Control

```cpp
class RemoteControlServer : public QObject {
    Q_OBJECT
public:
    // JSON-RPC 2.0 over TCP/WebSocket
    void startServer(quint16 port);
    void stopServer();
    
    // Exposed methods
    Q_INVOKABLE QJsonObject getStatus();
    Q_INVOKABLE bool setFrequency(double freq);
    Q_INVOKABLE bool setMode(const QString& mode);
    Q_INVOKABLE QJsonArray getScanResults();
    
signals:
    void commandReceived(const QJsonObject& cmd);
    void clientConnected(const QString& address);
};
```

### State Management and Persistence

```cpp
class ApplicationState : public QObject {
    Q_OBJECT
    Q_PROPERTY(double frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    
public:
    // State persistence
    void saveState(const QString& profileName);
    void loadState(const QString& profileName);
    
    // Undo/Redo
    void pushState();
    void undo();
    void redo();
    
    // Profile management
    QStringList getProfiles();
    void deleteProfile(const QString& name);
    
signals:
    void stateChanged();
    void profileLoaded(const QString& name);
};
```

### QML Integration Example

```qml
// Main radio interface
ApplicationWindow {
    id: mainWindow
    
    // C++ backend connections
    Connections {
        target: RadioController
        
        function onFrequencyChanged(freq) {
            frequencyDial.value = freq
            spectrumDisplay.centerFrequency = freq
        }
        
        function onSignalStrengthChanged(dbfs) {
            sMeter.value = dbfs
            signalIndicator.active = dbfs > squelchSlider.value
        }
    }
    
    // Vintage frequency dial component
    VintageDialControl {
        id: frequencyDial
        min: RadioController.minFrequency
        max: RadioController.maxFrequency
        value: RadioController.frequency
        
        onValueChanged: {
            RadioController.frequency = value
        }
        
        // Custom styling
        style: VintageDialStyle {
            backgroundColor: Theme.panelColor
            needleColor: Theme.indicatorColor
            scaleColor: Theme.textColor
        }
    }
    
    // Spectrum display with phosphor effect
    SpectrumDisplay {
        id: spectrumDisplay
        
        // Phosphor persistence settings
        persistence: persistenceSlider.value
        intensity: intensitySlider.value
        
        // Grid and markers
        showGrid: true
        gridColor: Theme.gridColor
        
        // Peak detection
        peakHold: peakHoldButton.checked
        peakDecay: 0.95
    }
}
```

### Build System Integration

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(VintageTacticalRadio VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets Multimedia)
find_package(PkgConfig REQUIRED)

# Find RTL-SDR
pkg_check_modules(RTLSDR REQUIRED librtlsdr)

# Find liquid-dsp
pkg_check_modules(LIQUID REQUIRED liquid-dsp)

# Find PortAudio
find_package(portaudio REQUIRED)

# Add executable
add_executable(vintage-tactical-radio
    src/main.cpp
    src/rtlsdr_interface.cpp
    src/dsp_engine.cpp
    src/audio_output.cpp
    src/frequency_database.cpp
    src/application_state.cpp
)

# Link libraries
target_link_libraries(vintage-tactical-radio
    Qt6::Core
    Qt6::Quick
    Qt6::Widgets
    Qt6::Multimedia
    ${RTLSDR_LIBRARIES}
    ${LIQUID_LIBRARIES}
    portaudio
)

# Install targets
install(TARGETS vintage-tactical-radio
    RUNTIME DESTINATION bin
)

# Create AppImage
if(BUILD_APPIMAGE)
    include(AppImageBuilder)
    build_appimage(vintage-tactical-radio)
endif()
```

### Performance Optimization Guidelines

1. **Real-time Audio Processing**
   - Use lock-free ring buffers for audio data
   - Process in power-of-2 block sizes (512, 1024, 2048)
   - Implement SIMD optimizations where possible
   - Avoid memory allocations in audio callback

2. **Spectrum Display**
   - Use GPU acceleration for waterfall rendering
   - Implement efficient FFT with FFTW3 or similar
   - Decimate display data based on window size
   - Cache computed values where possible

3. **Threading Model**
   ```
   Main Thread: GUI/User Interaction
   RTL-SDR Thread: Hardware I/O
   DSP Thread: Signal Processing
   Audio Thread: Output (callback-based)
   Database Thread: Station logging/scanning
   ```

4. **Memory Management**
   - Pre-allocate buffers at startup
   - Use object pools for frequently created/destroyed objects
   - Implement smart pointers for automatic cleanup
   - Monitor and limit memory usage

### Error Handling and Recovery

```cpp
class ErrorHandler : public QObject {
    Q_OBJECT
public:
    enum ErrorLevel {
        Info,
        Warning,
        Error,
        Critical
    };
    
    void handleError(ErrorLevel level, const QString& message);
    
    // Automatic recovery strategies
    void setAutoReconnect(bool enable);
    void setFallbackDevice(const QString& device);
    
signals:
    void errorOccurred(ErrorLevel level, const QString& message);
    void recoveryAttempted(bool success);
};
```

### Testing Framework

```cpp
// Unit test example
class RTLSDRTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void testDeviceEnumeration();
    void testFrequencyTuning();
    void testGainControl();
    void testDirectSampling();
    void cleanupTestCase();
};

// Integration test example
class RadioIntegrationTest : public QObject {
    Q_OBJECT
private slots:
    void testFullSignalPath();
    void testAudioOutput();
    void testStatePeristence();
};
```

This comprehensive API and integration guide provides all the technical details needed to implement the vintage tactical radio application with professional-grade architecture and performance.