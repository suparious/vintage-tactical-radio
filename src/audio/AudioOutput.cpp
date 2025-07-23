#include "AudioOutput.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <algorithm>
#include <cstring>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

AudioOutput::AudioOutput(QObject* parent)
    : QIODevice(parent)
    , volume_(1.0f) {
    
    audioDevice_ = nullptr;
    
    initializeFormat();
    open(QIODevice::ReadOnly);
}

AudioOutput::~AudioOutput() {
    stop();
}

void AudioOutput::initializeFormat() {
    format_.setSampleRate(48000);
    format_.setChannelConfig(QAudioFormat::ChannelConfigMono);
    format_.setSampleFormat(QAudioFormat::Int16);
}

std::vector<AudioOutput::AudioDevice> AudioOutput::getDevices() const {
    std::vector<AudioDevice> devices;
    
    auto audioDevices = QMediaDevices::audioOutputs();
    auto defaultDevice = QMediaDevices::defaultAudioOutput();
    
    for (const auto& device : audioDevices) {
        AudioDevice info;
        QString description = device.description();
        
        // Enhance device names for clarity
        if (description.contains("USB", Qt::CaseInsensitive)) {
            info.name = QString("[USB] %1").arg(description);
        } else if (description.contains("HDMI", Qt::CaseInsensitive) || 
                   description.contains("DisplayPort", Qt::CaseInsensitive)) {
            info.name = QString("[HDMI/DP] %1").arg(description);
        } else if (description.contains("PulseAudio", Qt::CaseInsensitive)) {
            info.name = QString("[PulseAudio] %1").arg(description);
        } else if (description.contains("ALSA", Qt::CaseInsensitive) || 
                   device.id().contains("alsa")) {
            info.name = QString("[ALSA] %1").arg(description);
        } else {
            info.name = description;
        }
        
        info.id = device.id();
        info.isDefault = (device.id() == defaultDevice.id());
        devices.push_back(info);
    }
    
    return devices;
}

bool AudioOutput::setDevice(const QString& deviceId) {
    auto audioDevices = QMediaDevices::audioOutputs();
    
    for (const auto& device : audioDevices) {
        if (device.id() == deviceId.toUtf8()) {
            currentDevice_ = device;
            
            // Recreate audio sink with new device
            bool wasPlaying = isPlaying();
            if (wasPlaying) {
                stop();
            }
            
            createAudioSink();
            
            if (wasPlaying) {
                start();
            }
            
#ifdef HAS_SPDLOG
            spdlog::info("Audio device set to: {}", device.description().toStdString());
#endif
            return true;
        }
    }
    
    return false;
}

QString AudioOutput::getCurrentDevice() const {
    return QString::fromUtf8(currentDevice_.id());
}

void AudioOutput::setSampleRate(int rate) {
    if (format_.sampleRate() != rate) {
        format_.setSampleRate(rate);
        
        // Recreate audio sink with new format
        bool wasPlaying = isPlaying();
        if (wasPlaying) {
            stop();
        }
        
        createAudioSink();
        
        if (wasPlaying) {
            start();
        }
    }
}

void AudioOutput::setSampleFormat(QAudioFormat::SampleFormat format) {
    if (format_.sampleFormat() != format) {
        format_.setSampleFormat(format);
        
        // Note: Qt6 natively supports Int16, Int32, and Float formats
        // For 24-bit, we'll use Int32 internally and convert
        
        // Recreate audio sink
        bool wasPlaying = isPlaying();
        if (wasPlaying) {
            stop();
        }
        
        createAudioSink();
        
        if (wasPlaying) {
            start();
        }
    }
}

void AudioOutput::createAudioSink() {
    if (!currentDevice_.isNull()) {
        audioSink_ = std::make_unique<QAudioSink>(currentDevice_, format_);
    } else {
        audioSink_ = std::make_unique<QAudioSink>(format_);
    }
    
    connect(audioSink_.get(), &QAudioSink::stateChanged,
            this, &AudioOutput::handleStateChanged);
    
    // Set buffer size for low latency
    audioSink_->setBufferSize(format_.sampleRate() * 
                              format_.bytesPerSample() * 
                              format_.channelCount() / 10); // 100ms buffer
}

bool AudioOutput::start() {
    if (!audioSink_) {
        createAudioSink();
    }
    
    if (audioSink_) {
        audioDevice_ = audioSink_->start();
        return audioDevice_ != nullptr;
    }
    
    return false;
}

void AudioOutput::stop() {
    if (audioSink_) {
        audioSink_->stop();
        audioDevice_ = nullptr;
    }
}

bool AudioOutput::isPlaying() const {
    return audioSink_ && audioSink_->state() == QAudio::ActiveState;
}

void AudioOutput::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    if (audioSink_) {
        audioSink_->setVolume(volume_);
    }
}

void AudioOutput::writeAudio(const float* data, size_t samples) {
    if (!audioSink_ || !audioDevice_) {
        return;
    }
    
    // Convert float samples to the output format
    size_t bytesPerSample = format_.bytesPerSample();
    size_t totalBytes = samples * bytesPerSample;
    
    if (conversionBuffer_.size() < totalBytes) {
        conversionBuffer_.resize(totalBytes);
    }
    
    convertFloatToFormat(data, conversionBuffer_.data(), samples);
    
    // Write to the audio device
    qint64 written = audioDevice_->write(conversionBuffer_.data(), totalBytes);
    
    if (written != static_cast<qint64>(totalBytes)) {
#ifdef HAS_SPDLOG
        spdlog::warn("Audio underrun: {} bytes written of {} requested", 
                    written, totalBytes);
#endif
    }
}

qint64 AudioOutput::readData(char* data, qint64 maxlen) {
    // This is called by QAudioSink to pull data
    // We're using push mode, so this shouldn't be called
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 AudioOutput::writeData(const char* data, qint64 len) {
    // This is for QIODevice interface, not used in our push mode
    Q_UNUSED(data);
    Q_UNUSED(len);
    return len;
}

void AudioOutput::handleStateChanged(QAudio::State state) {
#ifdef HAS_SPDLOG
    switch (state) {
        case QAudio::ActiveState:
            spdlog::info("Audio output started");
            break;
        case QAudio::SuspendedState:
            spdlog::info("Audio output suspended");
            break;
        case QAudio::StoppedState:
            spdlog::info("Audio output stopped");
            break;
        case QAudio::IdleState:
            spdlog::debug("Audio output idle");
            break;
    }
#else
    Q_UNUSED(state);
#endif
}

void AudioOutput::convertFloatToFormat(const float* input, char* output, size_t samples) {
    switch (format_.sampleFormat()) {
        case QAudioFormat::Int16: {
            int16_t* out16 = reinterpret_cast<int16_t*>(output);
            for (size_t i = 0; i < samples; i++) {
                float sample = input[i] * volume_;
                sample = std::max(-1.0f, std::min(1.0f, sample));
                out16[i] = static_cast<int16_t>(sample * 32767.0f);
            }
            break;
        }
        
        case QAudioFormat::Int32: {
            int32_t* out32 = reinterpret_cast<int32_t*>(output);
            for (size_t i = 0; i < samples; i++) {
                float sample = input[i] * volume_;
                sample = std::max(-1.0f, std::min(1.0f, sample));
                // For 24-bit audio, use 24-bit range in 32-bit container
                // This provides s24le compatibility
                out32[i] = static_cast<int32_t>(sample * 8388607.0f) << 8;
            }
            break;
        }
        
        case QAudioFormat::Float: {
            float* outFloat = reinterpret_cast<float*>(output);
            for (size_t i = 0; i < samples; i++) {
                outFloat[i] = input[i] * volume_;
            }
            break;
        }
        
        default:
            // Unsupported format, fill with silence
            std::memset(output, 0, samples * format_.bytesPerSample());
            break;
    }
}

int AudioOutput::getBufferSize() const {
    return audioSink_ ? audioSink_->bufferSize() : 0;
}

int AudioOutput::getBufferFree() const {
    return audioSink_ ? audioSink_->bytesFree() : 0;
}
