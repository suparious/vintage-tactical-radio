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
    
    // Settings
    void onAudioDeviceChanged(int index);
    void onSampleRateChanged(int index);
    void onSampleFormatChanged(int index);
    void onResetAllClicked();
    
    // DSP callbacks
    void onSignalStrengthChanged(float strength);
    void onSpectrumData(const float* data, size_t length);
    
private:
    // Core components
    std::shared_ptr<Settings> settings_;
    std::unique_ptr<RTLSDRDevice> rtlsdr_;
    std::unique_ptr<DSPEngine> dspEngine_;
    std::unique_ptr<AudioOutput> audioOutput_;
    std::unique_ptr<VintageEqualizer> equalizer_;
    
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
    
    // Settings controls
    QComboBox* audioDeviceCombo_;
    QComboBox* sampleRateCombo_;
    QComboBox* sampleFormatCombo_;
    
    // EQ controls
    QComboBox* eqModeCombo_;
    QComboBox* eqPresetCombo_;
    
    // Buttons
    QPushButton* startStopButton_;
    QPushButton* resetEQButton_;
    QPushButton* resetAllButton_;
    
    // Status
    QLabel* statusLabel_;
    std::atomic<bool> isRunning_;
    
    // Current state
    double currentFrequency_;
    int currentBand_;
    
    // Setup methods
    void setupUI();
    void createMenuBar();
    void createCentralWidget();
    void createControlPanel();
    void createEQPanel();
    void createSettingsPanel();
    void connectSignals();
    void applyTheme();
    
    // Device methods
    void initializeDevices();
    void startRadio();
    void stopRadio();
    
    // Helper methods
    void updateFrequencyForBand(int band);
    void updateStatus(const QString& message);
    void saveSettings();
    void loadSettings();
};

#endif // MAINWINDOW_H
