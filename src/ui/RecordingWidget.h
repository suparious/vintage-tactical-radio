#ifndef RECORDING_WIDGET_H
#define RECORDING_WIDGET_H

#include <QWidget>
#include "../audio/RecordingManager.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE

class RecordingWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit RecordingWidget(QWidget* parent = nullptr);
    
    void setRecordingManager(RecordingManager* manager);
    void setFrequency(double frequency) { currentFrequency_ = frequency; }
    void setMode(const QString& mode) { currentMode_ = mode; }
    
signals:
    void recordingStartRequested();
    void recordingStopRequested();
    
private slots:
    void onRecordButtonClicked();
    void onTimeShiftToggled(bool checked);
    void onSaveTimeShiftClicked();
    void onRecordingStarted(const QString& fileName);
    void onRecordingStopped(const QString& fileName, qint64 bytes);
    void onRecordingProgress(qint64 bytes, const QString& time);
    void onRecordingError(const QString& error);
    
private:
    void updateRecordButton();
    QString generateFileName() const;
    
    RecordingManager* recordingManager_;
    
    // UI elements
    QPushButton* recordButton_;
    QLabel* statusLabel_;
    QLabel* timeLabel_;
    QComboBox* formatCombo_;
    QCheckBox* timeShiftCheck_;
    QPushButton* saveTimeShiftButton_;
    
    // State
    bool isRecording_;
    double currentFrequency_;
    QString currentMode_;
};

#endif // RECORDING_WIDGET_H
