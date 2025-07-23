#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QTimer>
#include <vector>
#include <atomic>

class RTLSDRDevice;
class DSPEngine;

class Scanner : public QObject {
    Q_OBJECT
    
public:
    enum class ScanMode {
        OFF,
        FREQUENCY,   // Scan frequency range
        CHANNEL,     // Scan predefined channels
        MEMORY,      // Scan memory channels
        BAND         // Scan within band
    };
    
    enum class ScanDirection {
        UP,
        DOWN
    };
    
    struct ScanParameters {
        double startFreq;
        double endFreq;
        double stepSize;
        int dwellTimeMs;        // Time to stay on active frequency
        int resumeTimeMs;       // Time before resuming scan
        double signalThreshold; // dB threshold to stop
        int scanSpeedHz;        // Frequencies per second
    };
    
    struct Channel {
        double frequency;
        QString name;
        QString mode;
        bool priority;
        int priorityLevel;
    };
    
    explicit Scanner(QObject* parent = nullptr);
    ~Scanner();
    
    // Configuration
    void setRTLSDR(RTLSDRDevice* rtlsdr) { rtlsdr_ = rtlsdr; }
    void setDSPEngine(DSPEngine* dsp) { dspEngine_ = dsp; }
    void setScanParameters(const ScanParameters& params);
    void setChannels(const std::vector<Channel>& channels);
    void setMemoryChannels(const std::vector<Channel>& channels);
    
    // Control
    void startScan(ScanMode mode, ScanDirection direction = ScanDirection::UP);
    void stopScan();
    void pauseScan();
    void resumeScan();
    void skipChannel();
    
    // Status
    bool isScanning() const { return isScanning_; }
    bool isPaused() const { return isPaused_; }
    ScanMode getScanMode() const { return currentMode_; }
    double getCurrentFrequency() const { return currentFrequency_; }
    QString getCurrentChannel() const;
    
    // Priority channels
    void addPriorityChannel(double frequency, int level = 1);
    void removePriorityChannel(double frequency);
    void setPriorityCheckInterval(int ms) { priorityCheckInterval_ = ms; }
    
signals:
    void scanStarted(ScanMode mode);
    void scanStopped();
    void frequencyChanged(double frequency);
    void channelFound(double frequency, const QString& name);
    void signalDetected(double frequency, double strength);
    void scanProgress(int percent);
    
private slots:
    void onScanTimer();
    void onDwellTimer();
    void onPriorityTimer();
    void onSignalStrength(float strength);
    
private:
    void scanNextFrequency();
    void scanNextChannel();
    void checkSignalActivity();
    void checkPriorityChannels();
    bool isSignalActive(double strength);
    
    // Hardware interfaces
    RTLSDRDevice* rtlsdr_;
    DSPEngine* dspEngine_;
    
    // Scan state
    std::atomic<bool> isScanning_;
    std::atomic<bool> isPaused_;
    ScanMode currentMode_;
    ScanDirection scanDirection_;
    
    // Current position
    double currentFrequency_;
    size_t currentChannelIndex_;
    
    // Scan parameters
    ScanParameters params_;
    std::vector<Channel> channels_;
    std::vector<Channel> memoryChannels_;
    std::vector<Channel> priorityChannels_;
    
    // Timers
    QTimer* scanTimer_;
    QTimer* dwellTimer_;
    QTimer* priorityTimer_;
    
    // Signal detection
    double lastSignalStrength_;
    double noiseFloor_;
    int signalDetectCount_;
    static constexpr int SIGNAL_DETECT_THRESHOLD = 3;
    
    // Priority scanning
    int priorityCheckInterval_;
    bool returningFromPriority_;
    double savedFrequency_;
};

#endif // SCANNER_H
