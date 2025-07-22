#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <QAudioSink>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QIODevice>

class AudioOutput : public QIODevice {
    Q_OBJECT
    
public:
    struct AudioDevice {
        QString name;
        QString id;
        bool isDefault;
    };
    
    explicit AudioOutput(QObject* parent = nullptr);
    ~AudioOutput();
    
    // Device enumeration
    std::vector<AudioDevice> getDevices() const;
    bool setDevice(const QString& deviceId);
    QString getCurrentDevice() const;
    
    // Audio format configuration
    void setSampleRate(int rate);
    int getSampleRate() const { return format_.sampleRate(); }
    
    void setSampleFormat(QAudioFormat::SampleFormat format);
    QAudioFormat::SampleFormat getSampleFormat() const { return format_.sampleFormat(); }
    
    // Playback control
    bool start();
    void stop();
    bool isPlaying() const;
    
    // Volume control (0.0 - 1.0)
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    // Write audio data
    void writeAudio(const float* data, size_t samples);
    
    // Buffer status
    int getBufferSize() const;
    int getBufferFree() const;
    
protected:
    // QIODevice interface
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;
    
private slots:
    void handleStateChanged(QAudio::State state);
    
private:
    QAudioFormat format_;
    std::unique_ptr<QAudioSink> audioSink_;
    QIODevice* audioDevice_;
    QAudioDevice currentDevice_;
    float volume_;
    
    // Internal buffer for format conversion
    std::vector<char> conversionBuffer_;
    
    void initializeFormat();
    void createAudioSink();
    void convertFloatToFormat(const float* input, char* output, size_t samples);
};

#endif // AUDIOOUTPUT_H
