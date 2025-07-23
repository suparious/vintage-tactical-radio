#ifndef ADSBDECODER_H
#define ADSBDECODER_H

#include "DigitalDecoder.h"
#include <unordered_map>
#include <chrono>

class ADSBDecoder : public DigitalDecoder {
    Q_OBJECT
    
public:
    explicit ADSBDecoder(QObject* parent = nullptr);
    ~ADSBDecoder() override;
    
    // Control methods
    void start() override;
    void stop() override;
    void reset() override;
    
    // Process IQ samples at 1090 MHz
    void processRaw(const uint8_t* data, size_t length) override;
    
    // Aircraft information
    struct Aircraft {
        uint32_t icao;          // ICAO 24-bit address
        QString callsign;       // Flight number
        double latitude;        // Degrees
        double longitude;       // Degrees
        float altitude;         // Feet
        float groundSpeed;      // Knots
        float track;            // Degrees
        float verticalRate;     // Feet/min
        bool onGround;
        std::chrono::steady_clock::time_point lastSeen;
        
        Aircraft() : icao(0), latitude(0), longitude(0), altitude(0),
                    groundSpeed(0), track(0), verticalRate(0), onGround(false) {}
    };
    
    // Get current aircraft list
    std::vector<Aircraft> getAircraft() const;
    
    // Configuration
    void setGainReduction(int db) { gainReduction_ = db; }
    int getGainReduction() const { return gainReduction_; }
    
signals:
    void aircraftDetected(uint32_t icao);
    void aircraftUpdated(uint32_t icao, const Aircraft& aircraft);
    void aircraftLost(uint32_t icao);
    void messageDecoded(int df, uint32_t icao, const QByteArray& data);
    
private:
    // Mode S message types
    enum DownlinkFormat {
        DF0 = 0,   // Short air-air surveillance
        DF4 = 4,   // Surveillance altitude reply
        DF5 = 5,   // Surveillance identity reply
        DF11 = 11, // All-call reply
        DF16 = 16, // Long air-air surveillance
        DF17 = 17, // Extended squitter (ADS-B)
        DF18 = 18, // Extended squitter non-transponder
        DF19 = 19, // Military extended squitter
        DF20 = 20, // Comm-B altitude reply
        DF21 = 21, // Comm-B identity reply
        DF24 = 24  // Comm-D
    };
    
    // ADS-B message types (Type Code)
    enum ADSBType {
        TC_AIRCRAFT_ID = 1,      // Aircraft identification
        TC_SURFACE_POS = 2,      // Surface position
        TC_AIRBORNE_POS_0 = 3,   // Airborne position (Baro Alt)
        TC_AIRBORNE_POS_1 = 4,   // Airborne position (Baro Alt)
        TC_SURFACE_VEL = 5,      // Surface velocity
        TC_AIRBORNE_VEL = 6,     // Airborne velocity
        TC_EVENT = 7,            // Event report
        TC_PRIORITY = 8          // Priority status
    };
    
    // Constants
    static constexpr int ADSB_PREAMBLE_LENGTH = 16;    // bits
    static constexpr int ADSB_SHORT_MSG_LENGTH = 56;   // bits
    static constexpr int ADSB_LONG_MSG_LENGTH = 112;   // bits
    static constexpr int ADSB_SAMPLE_RATE = 2000000;   // 2 MHz
    static constexpr float ADSB_FREQ = 1090e6;         // 1090 MHz
    
    // Signal processing
    void detectPreamble(const uint16_t* mag, size_t length);
    bool validatePreamble(const uint16_t* mag, size_t offset);
    int demodulateMessage(const uint16_t* mag, size_t offset, uint8_t* msg);
    
    // Message decoding
    bool decodeMessage(const uint8_t* msg, int length);
    bool checkCRC(const uint8_t* msg, int bits);
    uint32_t calculateCRC(const uint8_t* msg, int bits);
    
    // ADS-B decoding
    void decodeExtendedSquitter(const uint8_t* msg);
    void decodeAircraftID(uint32_t icao, const uint8_t* me);
    void decodeAirbornePosition(uint32_t icao, const uint8_t* me);
    void decodeAirborneVelocity(uint32_t icao, const uint8_t* me);
    void decodeSurfacePosition(uint32_t icao, const uint8_t* me);
    
    // Position decoding (CPR)
    bool decodeCPRPosition(uint32_t icao, int fflag, int encoded_lat, int encoded_lon,
                          double& lat, double& lon);
    
    // Magnitude calculation from IQ
    void calculateMagnitude(const uint8_t* iq, uint16_t* mag, size_t length);
    
    // Aircraft tracking
    std::unordered_map<uint32_t, Aircraft> aircraft_;
    void updateAircraft(uint32_t icao, const Aircraft& update);
    void removeStaleAircraft();
    
    // Buffers
    std::vector<uint16_t> magnitudeBuffer_;
    std::vector<uint8_t> messageBuffer_;
    size_t bufferIndex_;
    
    // Configuration
    int gainReduction_;
    
    // Statistics
    uint64_t messagesReceived_;
    uint64_t messagesValid_;
    uint64_t crcErrors_;
    
    // CPR decoding state
    struct CPRState {
        double evenLat, evenLon;
        double oddLat, oddLon;
        std::chrono::steady_clock::time_point evenTime;
        std::chrono::steady_clock::time_point oddTime;
        bool hasEven, hasOdd;
    };
    std::unordered_map<uint32_t, CPRState> cprStates_;
    
    // Magnitude lookup table for faster processing
    std::array<uint16_t, 256 * 256> magLUT_;
    void initMagnitudeLUT();
};

#endif // ADSBDECODER_H
