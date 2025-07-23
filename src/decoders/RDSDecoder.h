#ifndef RDSDECODER_H
#define RDSDECODER_H

#include "DigitalDecoder.h"
#include <deque>
#include <array>
#include <string>

class RDSDecoder : public DigitalDecoder {
    Q_OBJECT
    
public:
    explicit RDSDecoder(QObject* parent = nullptr);
    ~RDSDecoder() override;
    
    // Control methods
    void start() override;
    void stop() override;
    void reset() override;
    
    // Process FM demodulated audio (containing RDS at 57kHz)
    void processAudio(const float* samples, size_t length) override;
    
    // RDS data types
    enum class RDSDataType {
        PI,         // Program Identification
        PS,         // Program Service name
        PTY,        // Program Type
        RT,         // RadioText
        CT,         // Clock Time
        AF,         // Alternative Frequencies
        TA,         // Traffic Announcement
        TP,         // Traffic Program
        MS,         // Music/Speech
        PIN,        // Program Item Number
        TMC,        // Traffic Message Channel
        EWS         // Emergency Warning System
    };
    
    // Get decoded data
    uint16_t getProgramID() const { return programID_; }
    QString getProgramService() const { return programService_; }
    QString getRadioText() const { return radioText_; }
    uint8_t getProgramType() const { return programType_; }
    bool hasTrafficProgram() const { return trafficProgram_; }
    bool hasTrafficAnnouncement() const { return trafficAnnouncement_; }
    bool isMusic() const { return musicSpeech_; }
    
    // Program type names
    static QString getProgramTypeName(uint8_t pty);
    
signals:
    void programIDChanged(uint16_t pi);
    void programServiceChanged(const QString& ps);
    void radioTextChanged(const QString& rt);
    void programTypeChanged(uint8_t pty);
    void trafficAnnouncementChanged(bool ta);
    void clockTimeReceived(const QDateTime& ct);
    void alternativeFrequenciesReceived(const QList<float>& frequencies);
    
private:
    // RDS constants
    static constexpr float RDS_CARRIER_FREQ = 57000.0f;  // 57 kHz
    static constexpr float RDS_SYMBOL_RATE = 1187.5f;    // symbols/sec
    static constexpr int RDS_BITS_PER_GROUP = 104;       // 4 blocks of 26 bits
    
    // Demodulation stages
    void extract57kHz(const float* samples, size_t length);
    void demodulateRDS();
    void decodeSymbols();
    void processGroups();
    
    // Group processing
    void processGroupType0(uint16_t blockB, uint16_t blockC, uint16_t blockD);
    void processGroupType2(uint16_t blockB, uint16_t blockC, uint16_t blockD);
    void processGroupType4A(uint16_t blockB, uint16_t blockC, uint16_t blockD);
    
    // Error correction
    bool checkAndCorrectBlock(uint32_t& block);
    uint16_t calculateSyndrome(uint32_t block);
    
    // Filters and buffers
    struct BandpassFilter {
        float b0, b1, b2;
        float a1, a2;
        float x1, x2;
        float y1, y2;
        
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = output;
            return output;
        }
    };
    
    BandpassFilter rdsFilter_;      // 57kHz bandpass
    BandpassFilter pilotFilter_;    // 19kHz pilot tone filter
    
    // Carrier recovery
    float carrierPhase_;
    float carrierFreq_;
    float pilotPhase_;
    bool carrierLocked_;
    
    // Symbol timing recovery
    float symbolPhase_;
    float symbolRate_;
    size_t samplesPerSymbol_;
    
    // Demodulation buffers
    std::vector<float> i_buffer_;
    std::vector<float> q_buffer_;
    size_t bufferIndex_;
    
    // Symbol decoder
    std::deque<int> symbolBuffer_;
    uint32_t bitBuffer_;
    size_t bitCount_;
    
    // Group decoder
    std::array<uint16_t, 4> currentGroup_;
    size_t blockCount_;
    bool groupSync_;
    
    // Decoded data storage
    uint16_t programID_;
    QString programService_;
    QString radioText_;
    uint8_t programType_;
    bool trafficProgram_;
    bool trafficAnnouncement_;
    bool musicSpeech_;
    
    // PS and RT assembly
    std::array<char, 8> psBuffer_;
    std::array<bool, 8> psValid_;
    std::array<char, 64> rtBuffer_;
    std::array<bool, 64> rtValid_;
    uint8_t rtABFlag_;
    
    // Clock time assembly
    uint32_t modifiedJulianDay_;
    uint8_t hours_;
    uint8_t minutes_;
    int8_t localTimeOffset_;
    
    // Alternative frequencies
    QList<float> alternativeFreqs_;
    
    // Syndrome lookup table for error correction
    static const uint16_t OFFSET_WORDS[5];
    static const uint16_t SYNDROME_TABLE[1024];
};

#endif // RDSDECODER_H
