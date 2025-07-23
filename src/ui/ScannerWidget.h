#ifndef SCANNER_WIDGET_H
#define SCANNER_WIDGET_H

#include <QWidget>
#include "../dsp/Scanner.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class QSlider;
class QProgressBar;
QT_END_NAMESPACE

class ScannerWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ScannerWidget(QWidget* parent = nullptr);
    
    void setScanner(Scanner* scanner);
    void setMemoryChannels(const std::vector<Scanner::Channel>& channels);
    
signals:
    void scanModeChanged(Scanner::ScanMode mode);
    void scanParametersChanged();
    
private slots:
    void onScanButtonClicked();
    void onModeChanged(int index);
    void onStepSizeChanged(int index);
    void onSpeedChanged(int value);
    void onThresholdChanged(int value);
    void onSkipClicked();
    
    // Scanner signals
    void onScanStarted(Scanner::ScanMode mode);
    void onScanStopped();
    void onFrequencyChanged(double frequency);
    void onChannelFound(double frequency, const QString& name);
    void onSignalDetected(double frequency, double strength);
    void onScanProgress(int percent);
    
private:
    void updateScanButton();
    void updateControls();
    
    Scanner* scanner_;
    
    // UI elements
    QPushButton* scanButton_;
    QPushButton* skipButton_;
    QComboBox* modeCombo_;
    QComboBox* stepCombo_;
    QSlider* speedSlider_;
    QSlider* thresholdSlider_;
    QLabel* statusLabel_;
    QLabel* frequencyLabel_;
    QLabel* speedLabel_;
    QLabel* thresholdLabel_;
    QProgressBar* progressBar_;
    
    // State
    bool isScanning_;
    Scanner::ScanMode currentMode_;
};

#endif // SCANNER_WIDGET_H
