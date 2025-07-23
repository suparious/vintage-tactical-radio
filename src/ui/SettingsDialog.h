#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <memory>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QPushButton;
class QSpinBox;
class QLabel;
QT_END_NAMESPACE

class Settings;
class AudioOutput;
class RTLSDRDevice;

class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(std::shared_ptr<Settings> settings, 
                          AudioOutput* audioOutput,
                          RTLSDRDevice* rtlsdr,
                          QWidget* parent = nullptr);
    ~SettingsDialog();
    
    // Get current settings
    int getSampleRate() const;
    int getSampleFormat() const;
    bool getDynamicBandwidth() const;
    bool getBiasT() const;
    int getPpm() const;
    int getRtlSampleRate() const;
    
signals:
    // Settings changed signals
    void audioDeviceChanged(int index);
    void sampleRateChanged(int index);
    void sampleFormatChanged(int index);
    void dynamicBandwidthChanged(bool checked);
    void biasTChanged(bool checked);
    void ppmChanged(int value);
    void rtlSampleRateChanged(int index);
    void resetAllClicked();
    
public slots:
    void updateBandwidthDisplay(const QString& text);
    void loadSettings();
    void saveSettings();
    
private slots:
    void onResetAllClicked();
    void onApplyClicked();
    void onAcceptClicked();
    
private:
    void setupUI();
    void createAudioSettings();
    void createRtlSdrSettings();
    void createGeneralSettings();
    void connectSignals();
    void populateAudioDevices();
    
    // Core components
    std::shared_ptr<Settings> settings_;
    AudioOutput* audioOutput_;
    RTLSDRDevice* rtlsdr_;
    
    // Audio settings
    QComboBox* audioDeviceCombo_;
    QComboBox* sampleRateCombo_;
    QComboBox* sampleFormatCombo_;
    
    // RTL-SDR settings
    QCheckBox* biasTCheck_;
    QSpinBox* ppmSpin_;
    QComboBox* rtlSampleRateCombo_;
    
    // General settings
    QCheckBox* dynamicBandwidthCheck_;
    QLabel* bandwidthLabel_;
    
    // Buttons
    QPushButton* resetAllButton_;
    QPushButton* applyButton_;
    QPushButton* okButton_;
    QPushButton* cancelButton_;
    
    // Store initial values for cancel
    int initialSampleRate_;
    int initialSampleFormat_;
    bool initialDynamicBandwidth_;
    bool initialBiasT_;
    int initialPpm_;
    int initialRtlSampleRate_;
};

#endif // SETTINGSDIALOG_H
