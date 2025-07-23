#include "RecordingWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDateTime>
#include <QInputDialog>

RecordingWidget::RecordingWidget(QWidget* parent)
    : QWidget(parent)
    , recordingManager_(nullptr)
    , isRecording_(false)
    , currentFrequency_(96.9e6)
    , currentMode_("FM") {
    
    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(10);
    
    // Record button
    recordButton_ = new QPushButton(tr("REC"), this);
    recordButton_->setObjectName("recordButton");
    recordButton_->setCheckable(true);
    recordButton_->setFixedWidth(60);
    recordButton_->setToolTip(tr("Start/Stop recording"));
    connect(recordButton_, &QPushButton::clicked, this, &RecordingWidget::onRecordButtonClicked);
    layout->addWidget(recordButton_);
    
    // Format selector
    formatCombo_ = new QComboBox(this);
    formatCombo_->addItems({"WAV", "FLAC", "MP3", "IQ"});
    formatCombo_->setToolTip(tr("Recording format"));
    formatCombo_->setFixedWidth(80);
    layout->addWidget(formatCombo_);
    
    // Time display
    timeLabel_ = new QLabel("00:00:00", this);
    timeLabel_->setObjectName("recordingTime");
    timeLabel_->setAlignment(Qt::AlignCenter);
    timeLabel_->setFixedWidth(80);
    layout->addWidget(timeLabel_);
    
    // Status label
    statusLabel_ = new QLabel(tr("Ready"), this);
    statusLabel_->setObjectName("recordingStatus");
    layout->addWidget(statusLabel_, 1);
    
    // Time-shift controls
    layout->addSpacing(20);
    timeShiftCheck_ = new QCheckBox(tr("Time Shift"), this);
    timeShiftCheck_->setToolTip(tr("Enable 30-minute time-shift buffer"));
    connect(timeShiftCheck_, &QCheckBox::toggled, this, &RecordingWidget::onTimeShiftToggled);
    layout->addWidget(timeShiftCheck_);
    
    saveTimeShiftButton_ = new QPushButton(tr("Save Buffer"), this);
    saveTimeShiftButton_->setEnabled(false);
    saveTimeShiftButton_->setToolTip(tr("Save time-shift buffer to file"));
    connect(saveTimeShiftButton_, &QPushButton::clicked, this, &RecordingWidget::onSaveTimeShiftClicked);
    layout->addWidget(saveTimeShiftButton_);
    
    // Set initial state
    updateRecordButton();
    
    // Apply styling
    setStyleSheet(R"(
        QPushButton#recordButton {
            font-weight: bold;
            background-color: #4a4a3a;
            border: 2px solid #8a8a7a;
        }
        QPushButton#recordButton:checked {
            background-color: #cc0000;
            color: white;
            border-color: #ff0000;
        }
        QLabel#recordingTime {
            font-family: monospace;
            font-size: 14px;
            font-weight: bold;
            color: #ffcc00;
            background-color: #2a2a1a;
            border: 1px solid #4a4a3a;
            padding: 2px;
        }
        QLabel#recordingStatus {
            color: #aaaaaa;
        }
    )");
}

void RecordingWidget::setRecordingManager(RecordingManager* manager) {
    if (recordingManager_) {
        // Disconnect old signals
        disconnect(recordingManager_, nullptr, this, nullptr);
    }
    
    recordingManager_ = manager;
    
    if (recordingManager_) {
        // Connect signals
        connect(recordingManager_, &RecordingManager::recordingStarted,
                this, &RecordingWidget::onRecordingStarted);
        connect(recordingManager_, &RecordingManager::recordingStopped,
                this, &RecordingWidget::onRecordingStopped);
        connect(recordingManager_, &RecordingManager::recordingProgress,
                this, &RecordingWidget::onRecordingProgress);
        connect(recordingManager_, &RecordingManager::recordingError,
                this, &RecordingWidget::onRecordingError);
    }
}

void RecordingWidget::onRecordButtonClicked() {
    if (!recordingManager_) {
        return;
    }
    
    if (!isRecording_) {
        // Start recording
        QString fileName = generateFileName();
        
        RecordingManager::Format format = RecordingManager::Format::WAV;
        RecordingManager::RecordingType type = RecordingManager::RecordingType::AUDIO;
        
        switch (formatCombo_->currentIndex()) {
            case 0: format = RecordingManager::Format::WAV; break;
            case 1: format = RecordingManager::Format::FLAC; break;
            case 2: format = RecordingManager::Format::MP3; break;
            case 3: 
                format = RecordingManager::Format::IQ_WAV;
                type = RecordingManager::RecordingType::IQ;
                break;
        }
        
        if (recordingManager_->startRecording(fileName, format, type,
                                            currentFrequency_, currentMode_)) {
            isRecording_ = true;
            emit recordingStartRequested();
        } else {
            recordButton_->setChecked(false);
        }
    } else {
        // Stop recording
        recordingManager_->stopRecording();
        isRecording_ = false;
        emit recordingStopRequested();
    }
    
    updateRecordButton();
}

void RecordingWidget::onTimeShiftToggled(bool checked) {
    if (recordingManager_) {
        recordingManager_->enableTimeShift(checked);
        saveTimeShiftButton_->setEnabled(checked);
        
        if (checked) {
            statusLabel_->setText(tr("Time-shift buffer enabled"));
        } else {
            statusLabel_->setText(tr("Time-shift buffer disabled"));
        }
    }
}

void RecordingWidget::onSaveTimeShiftClicked() {
    if (!recordingManager_) {
        return;
    }
    
    // Ask user how many seconds to save
    bool ok;
    int seconds = QInputDialog::getInt(this, tr("Save Time-Shift Buffer"),
                                      tr("Seconds to save (max 1800):"),
                                      60, 1, 1800, 1, &ok);
    if (!ok) {
        return;
    }
    
    QString fileName = generateFileName() + "_timeshift";
    if (recordingManager_->saveTimeShiftBuffer(fileName, seconds)) {
        QMessageBox::information(this, tr("Time-Shift Saved"),
                               tr("Saved %1 seconds to %2.wav").arg(seconds).arg(fileName));
    }
}

void RecordingWidget::onRecordingStarted(const QString& fileName) {
    Q_UNUSED(fileName);
    statusLabel_->setText(tr("Recording..."));
    formatCombo_->setEnabled(false);
}

void RecordingWidget::onRecordingStopped(const QString& fileName, qint64 bytes) {
    Q_UNUSED(fileName);
    
    QString sizeStr;
    if (bytes < 1024) {
        sizeStr = QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        sizeStr = QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else {
        sizeStr = QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
    
    statusLabel_->setText(tr("Recording saved (%1)").arg(sizeStr));
    timeLabel_->setText("00:00:00");
    formatCombo_->setEnabled(true);
    recordButton_->setChecked(false);
    isRecording_ = false;
    updateRecordButton();
}

void RecordingWidget::onRecordingProgress(qint64 bytes, const QString& time) {
    Q_UNUSED(bytes);
    timeLabel_->setText(time);
}

void RecordingWidget::onRecordingError(const QString& error) {
    statusLabel_->setText(tr("Error: %1").arg(error));
    recordButton_->setChecked(false);
    isRecording_ = false;
    formatCombo_->setEnabled(true);
    updateRecordButton();
}

void RecordingWidget::updateRecordButton() {
    if (isRecording_) {
        recordButton_->setText(tr("STOP"));
        recordButton_->setStyleSheet(recordButton_->styleSheet() + 
            "\n#recordButton { animation: blink 1s linear infinite; }");
    } else {
        recordButton_->setText(tr("REC"));
    }
}

QString RecordingWidget::generateFileName() const {
    return QString("VTR_%1MHz_%2_%3")
           .arg(currentFrequency_ / 1e6, 0, 'f', 3)
           .arg(currentMode_)
           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
}
