#ifndef DIGITALDECODER_H
#define DIGITALDECODER_H

#include <QObject>
#include <QVariantMap>
#include <memory>
#include <vector>
#include <atomic>
#include <complex>

class DigitalDecoder : public QObject {
    Q_OBJECT
    
public:
    enum class DecoderType {
        CTCSS,      // Continuous Tone-Coded Squelch System
        DCS,        // Digital Coded Squelch
        RDS,        // Radio Data System
        ADSB,       // Automatic Dependent Surveillance-Broadcast
        SAME        // Specific Area Message Encoding
    };
    
    enum class DecoderState {
        IDLE,
        SEARCHING,
        SYNCING,
        DECODING,
        ERROR
    };
    
    explicit DigitalDecoder(DecoderType type, QObject* parent = nullptr);
    virtual ~DigitalDecoder() = default;
    
    // Control methods
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    
    // Process methods - different decoders need different inputs
    virtual void processAudio(const float* samples, size_t length) { Q_UNUSED(samples); Q_UNUSED(length); }
    virtual void processIQ(const std::complex<float>* samples, size_t length) { Q_UNUSED(samples); Q_UNUSED(length); }
    virtual void processRaw(const uint8_t* data, size_t length) { Q_UNUSED(data); Q_UNUSED(length); }
    
    // State and configuration
    bool isActive() const { return active_; }
    DecoderType getType() const { return type_; }
    DecoderState getState() const { return state_; }
    
    // Configuration
    virtual void setSampleRate(uint32_t sampleRate) { sampleRate_ = sampleRate; }
    uint32_t getSampleRate() const { return sampleRate_; }
    
signals:
    void stateChanged(DecoderState state);
    void dataDecoded(const QVariantMap& data);
    void errorOccurred(const QString& error);
    
protected:
    void setState(DecoderState state);
    void emitData(const QVariantMap& data);
    void emitError(const QString& error);
    
    DecoderType type_;
    std::atomic<bool> active_;
    std::atomic<DecoderState> state_;
    uint32_t sampleRate_;
};

#endif // DIGITALDECODER_H
