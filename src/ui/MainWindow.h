#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <atomic>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QPushButton;
class QGroupBox;
class QCheckBox;
QT_END_NAMESPACE

class Settings;
class RTLSDRDevice;
class DSPEngine;
class AudioOutput;
class VintageEqualizer;
class VintageKnob;
class VintageMeter;
class FrequencyDial;
class SpectrumDisplay;
class MemoryChannelManager;
class QSpinBox;
class AntennaWidget;
class RecordingWidget;
class RecordingManager;
class Scanner;
class ScannerWidget;
class DecoderWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(std::shared_ptr<Settings> settings, QWidget* parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    // Device control
    void onDeviceChanged(int index);
    void onStartStop();
    void onDynamicBandwidthChanged(bool checked);
    void onBiasTChanged(bool checked);
    
    // Frequency control
    void onFrequencyChanged(double frequency);
    void onBandChanged(int band);
    void onModeChanged(int mode);
    
    // Audio control
    void onVolumeChanged(double value);
    void onSquelchChanged(double value);
    void onGainChanged(double value);
    
    // EQ control
    void onEQModeChanged(int mode);
    void onEQPresetChanged(int preset);
    void onEQBandChanged(int band, double value);
    void onEQResetClicked();
    void onEQGainRangeChanged(int index);
    
    // Settings
    void onSettingsTriggered();
    void onAudioDeviceChanged(int index);
    void onSampleRateChanged(int index);
    void onSampleFormatChanged(int index);
    void onResetAllClicked();
    void onPpmChanged(int value);
    void onRtlSampleRateChanged(int index);
    
    // DSP callbacks
    void onSignalStrengthChanged(float strength);
    void onSpectrumData(const float* data, size_t length);
    
    // Theme control
    void onThemeChanged(int theme);
    
    // Memory channel control
    void onMemoryStore();
    void onMemoryRecall();
    void onMemoryClear();
    void onMemoryChannelChanged();
    void onQuickChannelSelected(int index);
    
    // Scanner control
    void onScannerFrequencyChanged(double frequency);
    void updateMemoryChannelsForScanner();
    
    // Decoder control
    void updateDecoderState();
    
private:
    // Core components
    std::shared_ptr<Settings> settings_;
    std::unique_ptr<RTLSDRDevice> rtlsdr_;
    std::unique_ptr<DSPEngine> dspEngine_;
    std::unique_ptr<AudioOutput> audioOutput_;
    std::unique_ptr<VintageEqualizer> equalizer_;
    std::unique_ptr<MemoryChannelManager> memoryManager_;
    std::unique_ptr<RecordingManager> recordingManager_;
    std::unique_ptr<Scanner> scanner_;
    
    // UI components
    FrequencyDial* frequencyDial_;
    VintageMeter* signalMeter_;
    SpectrumDisplay* spectrumDisplay_;
    
    // Control knobs
    VintageKnob* volumeKnob_;
    VintageKnob* squelchKnob_;
    VintageKnob* gainKnob_;
    VintageKnob* tuningKnob_;
    
    // EQ knobs
    std::vector<VintageKnob*> eqKnobs_;
    
    // Mode/band controls
    QComboBox* deviceCombo_;
    QComboBox* bandSelector_;
    QComboBox* modeSelector_;
    
    // EQ controls
    QComboBox* eqModeCombo_;
    QComboBox* eqPresetCombo_;
    QComboBox* eqGainRangeCombo_;
    
    // Buttons
    QPushButton* startStopButton_;
    QPushButton* resetEQButton_;
    
    // Memory channel controls
    QComboBox* memoryBankCombo_;
    QSpinBox* memoryChannelSpin_;
    QPushButton* memoryStoreButton_;
    QPushButton* memoryRecallButton_;
    QPushButton* memoryClearButton_;
    QComboBox* quickChannelCombo_;
    
    // Status
    QLabel* statusLabel_;
    QLabel* bandwidthLabel_;
    AntennaWidget* antennaWidget_;
    RecordingWidget* recordingWidget_;
    ScannerWidget* scannerWidget_;
    DecoderWidget* decoderWidget_;
    std::atomic<bool> isRunning_;
    
    // Current state
    double currentFrequency_;
    int currentBand_;
    int currentTheme_;
    
    // Settings controls (now in SettingsDialog)
    class SettingsDialog* settingsDialog_;
    
    // Setup methods
    void setupUI();
    void createMenuBar();
    void createCentralWidget();
    void createControlPanel();
    void createEQPanel();
    void createMemoryPanel();
    void createRecordingPanel();
    void createScannerPanel();
    void createDecoderPanel();
    void connectSignals();
    void applyTheme();
    
    // Device methods
    void initializeDevices();
    void startRadio();
    void stopRadio();
    
    // Helper methods
    void updateFrequencyForBand(int band);
    void updateStatus(const QString& message);
    void updateBandwidthDisplay();
    void applyOptimalGain(double frequency);
    void saveSettings();
    void loadSettings();
    void createSettingsDialog();
};

#endif // MAINWINDOW_H
