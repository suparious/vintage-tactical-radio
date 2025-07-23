#include "RecordingManager.h"

#include <QTimer>
#include <QDir>
#include <QDataStream>
#include <cstring>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

RecordingManager::RecordingManager(QObject* parent)
    : QObject(parent)
    , isRecording_(false)
    , timeShiftWritePos_(0)
    , timeShiftEnabled_(false)
    , dataChunkSizePos_(0)
    , fileSizePos_(0) {
    
    // Set default recording directory
    recordingDirectory_ = QDir::homePath() + "/VintageRadio/Recordings";
    QDir().mkpath(recordingDirectory_);
    
    // Initialize timers
    updateTimer_ = new QTimer(this);
    updateTimer_->setInterval(1000); // Update every second
    connect(updateTimer_, &QTimer::timeout, this, &RecordingManager::onUpdateTimer);
    
    scheduledTimer_ = new QTimer(this);
    scheduledTimer_->setSingleShot(true);
    connect(scheduledTimer_, &QTimer::timeout, this, &RecordingManager::onScheduledTimer);
    
    // Pre-allocate time-shift buffer
    if (timeShiftEnabled_) {
        timeShiftBuffer_.resize(TIME_SHIFT_BUFFER_SIZE);
    }
}

RecordingManager::~RecordingManager() {
    if (isRecording_) {
        stopRecording();
    }
}

bool RecordingManager::startRecording(const QString& fileName, Format format, RecordingType type,
                                    double frequency, const QString& mode, int sampleRate, int bitDepth) {
    if (isRecording_) {
        emit recordingError("Already recording");
        return false;
    }
    
    // Ensure directory exists
    QDir().mkpath(recordingDirectory_);
    
    // Build full file path
    QString fullPath = recordingDirectory_ + "/" + fileName;
    
    // Add extension if not present
    if (format == Format::WAV && !fullPath.endsWith(".wav")) {
        fullPath += ".wav";
    } else if (format == Format::IQ_WAV && !fullPath.endsWith(".wav")) {
        fullPath += "_iq.wav";
    }
    
    // Initialize recording info
    currentRecording_.fileName = fullPath;
    currentRecording_.format = format;
    currentRecording_.type = type;
    currentRecording_.startTime = QDateTime::currentDateTime();
    currentRecording_.bytesWritten = 0;
    currentRecording_.frequency = frequency;
    currentRecording_.mode = mode;
    currentRecording_.sampleRate = sampleRate;
    currentRecording_.bitDepth = bitDepth;
    
    // For now, only implement WAV recording
    if (format == Format::WAV || format == Format::IQ_WAV) {
        recordingFile_ = std::make_unique<QFile>(fullPath);
        if (!recordingFile_->open(QIODevice::WriteOnly)) {
            emit recordingError("Failed to create file: " + recordingFile_->errorString());
            return false;
        }
        
        // Create WAV file with appropriate settings
        int channels = (type == RecordingType::IQ) ? 2 : 2; // Stereo for both
        if (!createWavFile(fullPath, sampleRate, channels, bitDepth)) {
            recordingFile_->close();
            recordingFile_.reset();
            emit recordingError("Failed to create WAV header");
            return false;
        }
        
        isRecording_ = true;
        recordingStartTime_ = QDateTime::currentDateTime();
        updateTimer_->start();
        
        emit recordingStarted(fullPath);
        
#ifdef HAS_SPDLOG
        spdlog::info("Started recording: {} at {} Hz, {} bits", 
                    fullPath.toStdString(), sampleRate, bitDepth);
#endif
        
        return true;
    } else {
        emit recordingError("Format not yet implemented");
        return false;
    }
}

void RecordingManager::stopRecording() {
    if (!isRecording_) {
        return;
    }
    
    isRecording_ = false;
    updateTimer_->stop();
    
    if (recordingFile_ && recordingFile_->isOpen()) {
        // Finalize WAV file
        finalizeWavFile();
        recordingFile_->close();
        
        emit recordingStopped(currentRecording_.fileName, currentRecording_.bytesWritten);
        
#ifdef HAS_SPDLOG
        spdlog::info("Stopped recording: {}, {} bytes written", 
                    currentRecording_.fileName.toStdString(), 
                    currentRecording_.bytesWritten);
#endif
    }
    
    recordingFile_.reset();
}

void RecordingManager::writeAudioData(const float* data, size_t samples) {
    if (!isRecording_ || currentRecording_.type != RecordingType::AUDIO) {
        // Update time-shift buffer even when not recording
        if (timeShiftEnabled_) {
            updateTimeShiftBuffer(data, samples);
        }
        return;
    }
    
    if (!recordingFile_ || !recordingFile_->isOpen()) {
        return;
    }
    
    // Convert float samples to appropriate bit depth
    QByteArray buffer;
    
    if (currentRecording_.bitDepth == 16) {
        // Convert to 16-bit signed integers
        buffer.resize(samples * sizeof(int16_t));
        int16_t* outData = reinterpret_cast<int16_t*>(buffer.data());
        
        for (size_t i = 0; i < samples; i++) {
            float sample = data[i];
            // Clamp to [-1, 1]
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            // Convert to int16
            outData[i] = static_cast<int16_t>(sample * 32767.0f);
        }
    } else if (currentRecording_.bitDepth == 24) {
        // Convert to 24-bit signed integers
        buffer.resize(samples * 3);
        uint8_t* outData = reinterpret_cast<uint8_t*>(buffer.data());
        
        for (size_t i = 0; i < samples; i++) {
            float sample = data[i];
            // Clamp to [-1, 1]
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            // Convert to int32 then extract lower 24 bits
            int32_t sample32 = static_cast<int32_t>(sample * 8388607.0f);
            outData[i * 3] = sample32 & 0xFF;
            outData[i * 3 + 1] = (sample32 >> 8) & 0xFF;
            outData[i * 3 + 2] = (sample32 >> 16) & 0xFF;
        }
    }
    
    // Write to file
    qint64 written = recordingFile_->write(buffer);
    if (written > 0) {
        currentRecording_.bytesWritten += written;
    }
    
    // Update time-shift buffer
    if (timeShiftEnabled_) {
        updateTimeShiftBuffer(data, samples);
    }
}

void RecordingManager::writeIQData(const uint8_t* data, size_t bytes) {
    if (!isRecording_ || currentRecording_.type != RecordingType::IQ) {
        return;
    }
    
    if (!recordingFile_ || !recordingFile_->isOpen()) {
        return;
    }
    
    // For IQ data, we need to convert from unsigned 8-bit to signed values
    // and potentially interleave as I/Q pairs
    QByteArray buffer(reinterpret_cast<const char*>(data), bytes);
    
    qint64 written = recordingFile_->write(buffer);
    if (written > 0) {
        currentRecording_.bytesWritten += written;
    }
}

bool RecordingManager::createWavFile(const QString& fileName, int sampleRate, int channels, int bitDepth) {
    Q_UNUSED(fileName);
    
    if (!recordingFile_ || !recordingFile_->isOpen()) {
        return false;
    }
    
    wavSampleRate_ = sampleRate;
    wavChannels_ = channels;
    wavBitDepth_ = bitDepth;
    
    // Write WAV header
    writeWavHeader();
    
    return true;
}

void RecordingManager::writeWavHeader() {
    QDataStream stream(recordingFile_.get());
    stream.setByteOrder(QDataStream::LittleEndian);
    
    // RIFF header
    stream.writeRawData("RIFF", 4);
    fileSizePos_ = recordingFile_->pos();
    stream << quint32(0); // File size - 8 (will be updated later)
    stream.writeRawData("WAVE", 4);
    
    // Format chunk
    stream.writeRawData("fmt ", 4);
    stream << quint32(16); // Chunk size
    stream << quint16(1);  // Audio format (1 = PCM)
    stream << quint16(wavChannels_); // Number of channels
    stream << quint32(wavSampleRate_); // Sample rate
    stream << quint32(wavSampleRate_ * wavChannels_ * wavBitDepth_ / 8); // Byte rate
    stream << quint16(wavChannels_ * wavBitDepth_ / 8); // Block align
    stream << quint16(wavBitDepth_); // Bits per sample
    
    // Data chunk
    stream.writeRawData("data", 4);
    dataChunkSizePos_ = recordingFile_->pos();
    stream << quint32(0); // Data size (will be updated later)
}

void RecordingManager::finalizeWavFile() {
    if (!recordingFile_ || !recordingFile_->isOpen()) {
        return;
    }
    
    qint64 currentPos = recordingFile_->pos();
    
    // Update file size
    recordingFile_->seek(fileSizePos_);
    QDataStream stream(recordingFile_.get());
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(currentPos - 8);
    
    // Update data chunk size
    recordingFile_->seek(dataChunkSizePos_);
    stream << quint32(currentPos - dataChunkSizePos_ - 4);
    
    // Return to end of file
    recordingFile_->seek(currentPos);
}

void RecordingManager::enableTimeShift(bool enable) {
    std::lock_guard<std::mutex> lock(timeShiftMutex_);
    
    if (enable && !timeShiftEnabled_) {
        timeShiftBuffer_.resize(TIME_SHIFT_BUFFER_SIZE);
        timeShiftWritePos_ = 0;
    } else if (!enable && timeShiftEnabled_) {
        timeShiftBuffer_.clear();
        timeShiftBuffer_.shrink_to_fit();
    }
    
    timeShiftEnabled_ = enable;
}

void RecordingManager::updateTimeShiftBuffer(const float* data, size_t samples) {
    std::lock_guard<std::mutex> lock(timeShiftMutex_);
    
    if (!timeShiftEnabled_ || timeShiftBuffer_.empty()) {
        return;
    }
    
    // Copy data to circular buffer
    for (size_t i = 0; i < samples; i++) {
        timeShiftBuffer_[timeShiftWritePos_] = data[i];
        timeShiftWritePos_ = (timeShiftWritePos_ + 1) % TIME_SHIFT_BUFFER_SIZE;
    }
}

bool RecordingManager::saveTimeShiftBuffer(const QString& fileName, int seconds) {
    std::lock_guard<std::mutex> lock(timeShiftMutex_);
    
    if (!timeShiftEnabled_ || timeShiftBuffer_.empty()) {
        emit recordingError("Time-shift buffer not enabled");
        return false;
    }
    
    // Calculate samples to save
    size_t samplesToSave = seconds * wavSampleRate_ * wavChannels_;
    if (samplesToSave > TIME_SHIFT_BUFFER_SIZE) {
        samplesToSave = TIME_SHIFT_BUFFER_SIZE;
    }
    
    // Create temporary recording
    QString tempFileName = fileName;
    if (!tempFileName.endsWith(".wav")) {
        tempFileName += ".wav";
    }
    
    // Start from the oldest data
    size_t readPos = (timeShiftWritePos_ + TIME_SHIFT_BUFFER_SIZE - samplesToSave) % TIME_SHIFT_BUFFER_SIZE;
    
    // Create WAV file
    QFile file(recordingDirectory_ + "/" + tempFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        emit recordingError("Failed to create time-shift file");
        return false;
    }
    
    // Write header
    recordingFile_ = std::make_unique<QFile>(recordingDirectory_ + "/" + tempFileName);
    recordingFile_->open(QIODevice::WriteOnly);
    writeWavHeader();
    
    // Write samples
    std::vector<int16_t> buffer(samplesToSave);
    for (size_t i = 0; i < samplesToSave; i++) {
        float sample = timeShiftBuffer_[readPos];
        buffer[i] = static_cast<int16_t>(sample * 32767.0f);
        readPos = (readPos + 1) % TIME_SHIFT_BUFFER_SIZE;
    }
    
    recordingFile_->write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(int16_t));
    
    // Finalize
    finalizeWavFile();
    recordingFile_->close();
    recordingFile_.reset();
    
    emit recordingStopped(tempFileName, samplesToSave * sizeof(int16_t));
    
    return true;
}

int RecordingManager::getTimeShiftBufferSeconds() const {
    if (!timeShiftEnabled_ || wavSampleRate_ == 0 || wavChannels_ == 0) {
        return 0;
    }
    
    return TIME_SHIFT_BUFFER_SIZE / (wavSampleRate_ * wavChannels_);
}

void RecordingManager::scheduleRecording(const QDateTime& startTime, int durationSeconds,
                                       const QString& fileName, Format format,
                                       double frequency, const QString& mode) {
    // Cancel any existing scheduled recording
    cancelScheduledRecording();
    
    scheduledStartTime_ = startTime;
    scheduledDuration_ = durationSeconds;
    scheduledFileName_ = fileName;
    scheduledFormat_ = format;
    scheduledFrequency_ = frequency;
    scheduledMode_ = mode;
    
    // Calculate milliseconds until start time
    qint64 msUntilStart = QDateTime::currentDateTime().msecsTo(startTime);
    
    if (msUntilStart > 0) {
        scheduledTimer_->start(msUntilStart);
    } else {
        emit recordingError("Scheduled time is in the past");
    }
}

void RecordingManager::cancelScheduledRecording() {
    scheduledTimer_->stop();
}

QString RecordingManager::getRecordingDirectory() const {
    return recordingDirectory_;
}

void RecordingManager::setRecordingDirectory(const QString& dir) {
    recordingDirectory_ = dir;
    QDir().mkpath(recordingDirectory_);
}

QStringList RecordingManager::getRecordings() const {
    QDir dir(recordingDirectory_);
    return dir.entryList(QStringList() << "*.wav" << "*.flac" << "*.mp3", 
                        QDir::Files, QDir::Time);
}

QString RecordingManager::getRecordingTime() const {
    if (!isRecording_) {
        return "00:00:00";
    }
    
    qint64 seconds = recordingStartTime_.secsTo(QDateTime::currentDateTime());
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    return QString("%1:%2:%3")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(secs, 2, 10, QChar('0'));
}

void RecordingManager::onUpdateTimer() {
    if (isRecording_) {
        emit recordingProgress(currentRecording_.bytesWritten, getRecordingTime());
    }
}

void RecordingManager::onScheduledTimer() {
    // Start the scheduled recording
    QString fileName = scheduledFileName_;
    if (fileName.isEmpty()) {
        fileName = QString("scheduled_%1").arg(
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    }
    
    if (startRecording(fileName, scheduledFormat_, RecordingType::AUDIO,
                      scheduledFrequency_, scheduledMode_)) {
        emit scheduledRecordingStarted();
        
        // Schedule stop if duration specified
        if (scheduledDuration_ > 0) {
            QTimer::singleShot(scheduledDuration_ * 1000, this, &RecordingManager::stopRecording);
        }
    }
}
