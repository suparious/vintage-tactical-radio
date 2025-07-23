#ifndef RECORDING_MANAGER_H
#define RECORDING_MANAGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QDateTime>
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>

class QTimer;

class RecordingManager : public QObject {
    Q_OBJECT
    
public:
    enum class Format {
        WAV,
        FLAC,
        MP3,
        IQ_WAV
    };
    
    enum class RecordingType {
        AUDIO,
        IQ
    };
    
    struct RecordingInfo {
        QString fileName;
        Format format;
        RecordingType type;
        QDateTime startTime;
        qint64 bytesWritten;
        double frequency;
        QString mode;
        int sampleRate;
        int bitDepth;
    };
    
    explicit RecordingManager(QObject* parent = nullptr);
    ~RecordingManager();
    
    // Recording control
    bool startRecording(const QString& fileName, Format format, RecordingType type,
                       double frequency, const QString& mode, int sampleRate = 48000, int bitDepth = 16);
    void stopRecording();
    bool isRecording() const { return isRecording_; }
    
    // Write data
    void writeAudioData(const float* data, size_t samples);
    void writeIQData(const uint8_t* data, size_t bytes);
    
    // Time-shift buffer
    void enableTimeShift(bool enable);
    bool saveTimeShiftBuffer(const QString& fileName, int seconds);
    int getTimeShiftBufferSeconds() const;
    
    // Scheduled recording
    void scheduleRecording(const QDateTime& startTime, int durationSeconds,
                          const QString& fileName, Format format,
                          double frequency, const QString& mode);
    void cancelScheduledRecording();
    
    // File management
    QString getRecordingDirectory() const;
    void setRecordingDirectory(const QString& dir);
    QStringList getRecordings() const;
    
    // Status
    RecordingInfo getCurrentRecording() const { return currentRecording_; }
    QString getRecordingTime() const;
    
signals:
    void recordingStarted(const QString& fileName);
    void recordingStopped(const QString& fileName, qint64 bytes);
    void recordingError(const QString& error);
    void recordingProgress(qint64 bytes, const QString& time);
    void scheduledRecordingStarted();
    
private slots:
    void onUpdateTimer();
    void onScheduledTimer();
    
private:
    // WAV file handling
    bool createWavFile(const QString& fileName, int sampleRate, int channels, int bitDepth);
    void writeWavHeader();
    void finalizeWavFile();
    
    // Format conversion (placeholder for future implementation)
    bool convertToFlac(const QString& wavFile, const QString& flacFile);
    bool convertToMp3(const QString& wavFile, const QString& mp3File);
    
    // Time-shift buffer management
    void updateTimeShiftBuffer(const float* data, size_t samples);
    
    // Member variables
    std::atomic<bool> isRecording_;
    RecordingInfo currentRecording_;
    std::unique_ptr<QFile> recordingFile_;
    QString recordingDirectory_;
    
    // Time-shift buffer (30 minutes at 48kHz stereo)
    static constexpr size_t TIME_SHIFT_BUFFER_SIZE = 30 * 60 * 48000 * 2;
    std::vector<float> timeShiftBuffer_;
    size_t timeShiftWritePos_;
    bool timeShiftEnabled_;
    std::mutex timeShiftMutex_;
    
    // Scheduled recording
    QTimer* scheduledTimer_;
    QDateTime scheduledStartTime_;
    int scheduledDuration_;
    QString scheduledFileName_;
    Format scheduledFormat_;
    double scheduledFrequency_;
    QString scheduledMode_;
    
    // Progress tracking
    QTimer* updateTimer_;
    QDateTime recordingStartTime_;
    
    // WAV file specifics
    qint64 dataChunkSizePos_;
    qint64 fileSizePos_;
    int wavSampleRate_;
    int wavChannels_;
    int wavBitDepth_;
};

#endif // RECORDING_MANAGER_H
