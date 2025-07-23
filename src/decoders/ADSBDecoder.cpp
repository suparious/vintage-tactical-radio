#include "ADSBDecoder.h"
#include <QVariantMap>
#include <cmath>
#include <cstring>
#include <algorithm>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

ADSBDecoder::ADSBDecoder(QObject* parent)
    : DigitalDecoder(DecoderType::ADSB, parent)
    , bufferIndex_(0)
    , gainReduction_(0)
    , messagesReceived_(0)
    , messagesValid_(0)
    , crcErrors_(0) {
    
    // Pre-allocate buffers
    magnitudeBuffer_.resize(ADSB_SAMPLE_RATE);  // 1 second buffer
    messageBuffer_.resize(ADSB_LONG_MSG_LENGTH / 8);
    
    // Initialize magnitude lookup table
    initMagnitudeLUT();
}

ADSBDecoder::~ADSBDecoder() {
    stop();
}

void ADSBDecoder::start() {
    if (active_) {
        return;
    }
    
    active_ = true;
    setState(DecoderState::SEARCHING);
    
    reset();
    
#ifdef HAS_SPDLOG
    spdlog::info("ADS-B decoder started");
#endif
}

void ADSBDecoder::stop() {
    if (!active_) {
        return;
    }
    
    active_ = false;
    setState(DecoderState::IDLE);
    
#ifdef HAS_SPDLOG
    spdlog::info("ADS-B decoder stopped - Messages: {} valid, {} total, {} CRC errors",
                 messagesValid_, messagesReceived_, crcErrors_);
#endif
}

void ADSBDecoder::reset() {
    aircraft_.clear();
    cprStates_.clear();
    bufferIndex_ = 0;
    messagesReceived_ = 0;
    messagesValid_ = 0;
    crcErrors_ = 0;
}

void ADSBDecoder::initMagnitudeLUT() {
    // Pre-calculate magnitude values for all possible I/Q combinations
    for (int i = 0; i < 256; i++) {
        for (int q = 0; q < 256; q++) {
            float fi = (i - 127.5f) / 128.0f;
            float fq = (q - 127.5f) / 128.0f;
            float mag = sqrtf(fi * fi + fq * fq);
            magLUT_[i * 256 + q] = static_cast<uint16_t>(mag * 65535.0f);
        }
    }
}

void ADSBDecoder::processRaw(const uint8_t* data, size_t length) {
    if (!active_) {
        return;
    }
    
    // Calculate magnitude from IQ data
    size_t samples = length / 2;  // 2 bytes per IQ sample
    
    for (size_t i = 0; i < samples && bufferIndex_ < magnitudeBuffer_.size(); i++) {
        uint8_t i_val = data[i * 2];
        uint8_t q_val = data[i * 2 + 1];
        
        // Use lookup table for fast magnitude calculation
        magnitudeBuffer_[bufferIndex_++] = magLUT_[i_val * 256 + q_val];
    }
    
    // Process buffer when full
    if (bufferIndex_ >= magnitudeBuffer_.size()) {
        detectPreamble(magnitudeBuffer_.data(), bufferIndex_);
        bufferIndex_ = 0;
        
        // Remove aircraft not seen for >60 seconds
        removeStaleAircraft();
    }
}

void ADSBDecoder::detectPreamble(const uint16_t* mag, size_t length) {
    // Look for preamble pattern
    for (size_t i = 0; i < length - ADSB_LONG_MSG_LENGTH * 2; i++) {
        if (validatePreamble(mag, i)) {
            // Try to decode message
            uint8_t msg[14];  // Max message size
            int msgLen = demodulateMessage(mag, i + ADSB_PREAMBLE_LENGTH, msg);
            
            if (msgLen > 0) {
                messagesReceived_++;
                if (decodeMessage(msg, msgLen)) {
                    messagesValid_++;
                    setState(DecoderState::DECODING);
                }
            }
        }
    }
}

void ADSBDecoder::calculateMagnitude(const uint8_t* iq, uint16_t* mag, size_t length) {
    for (size_t i = 0; i < length / 2; i++) {
        mag[i] = magLUT_[iq[i * 2] * 256 + iq[i * 2 + 1]];
    }
}

bool ADSBDecoder::validatePreamble(const uint16_t* mag, size_t offset) {
    // ADS-B preamble pattern: 1010000101000000
    // Check for proper pulse positions and amplitudes
    
    // Get reference level from first pulse
    uint16_t high = mag[offset];
    uint16_t low = mag[offset + 5];
    
    if (high < low * 2) {
        return false;  // Not enough signal
    }
    
    // Check preamble pattern
    if (mag[offset + 0] < high * 0.7) return false;  // 1
    if (mag[offset + 1] > low * 1.3) return false;   // 0
    if (mag[offset + 2] < high * 0.7) return false;  // 1
    if (mag[offset + 3] > low * 1.3) return false;   // 0
    if (mag[offset + 4] > low * 1.3) return false;   // 0
    if (mag[offset + 5] > low * 1.3) return false;   // 0
    if (mag[offset + 6] > low * 1.3) return false;   // 0
    if (mag[offset + 7] < high * 0.7) return false;  // 1
    if (mag[offset + 8] > low * 1.3) return false;   // 0
    if (mag[offset + 9] < high * 0.7) return false;  // 1
    
    return true;
}

int ADSBDecoder::demodulateMessage(const uint16_t* mag, size_t offset, uint8_t* msg) {
    // Determine message length from DF
    uint8_t df = 0;
    
    // Demodulate first 5 bits to get DF
    for (int i = 0; i < 5; i++) {
        uint16_t bit_high = mag[offset + i * 2];
        uint16_t bit_low = mag[offset + i * 2 + 1];
        
        if (bit_high > bit_low) {
            df |= (1 << (4 - i));
        }
    }
    
    // Determine message length
    int msgBits = (df == 0 || df == 4 || df == 5 || df == 11) ? 
                  ADSB_SHORT_MSG_LENGTH : ADSB_LONG_MSG_LENGTH;
    
    // Clear message buffer
    memset(msg, 0, msgBits / 8);
    
    // Demodulate message
    for (int i = 0; i < msgBits; i++) {
        uint16_t bit_high = mag[offset + i * 2];
        uint16_t bit_low = mag[offset + i * 2 + 1];
        
        if (bit_high > bit_low) {
            msg[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    
    return msgBits;
}

bool ADSBDecoder::decodeMessage(const uint8_t* msg, int length) {
    // Check CRC
    if (!checkCRC(msg, length)) {
        crcErrors_++;
        return false;
    }
    
    // Extract DF
    uint8_t df = (msg[0] >> 3) & 0x1F;
    
    // Process based on DF
    switch (df) {
        case DF17:  // Extended squitter (ADS-B)
        case DF18:  // Extended squitter non-transponder
            decodeExtendedSquitter(msg);
            break;
            
        case DF4:   // Altitude reply
        case DF20:  // Comm-B altitude reply
            {
                // Extract ICAO and altitude
                uint32_t icao = (msg[1] << 16) | (msg[2] << 8) | msg[3];
                // Altitude decoding would go here
                QVariantMap data;
                data["type"] = "ADSB_ALT";
                data["icao"] = icao;
                data["df"] = df;
                emitData(data);
            }
            break;
            
        default:
            // Other message types
            break;
    }
    
    return true;
}

bool ADSBDecoder::checkCRC(const uint8_t* msg, int bits) {
    // Simplified CRC check - real implementation would use Mode S polynomial
    // For now, accept all messages for testing
    Q_UNUSED(msg);
    Q_UNUSED(bits);
    return true;
}

void ADSBDecoder::decodeExtendedSquitter(const uint8_t* msg) {
    // Extract ICAO address
    uint32_t icao = (msg[1] << 16) | (msg[2] << 8) | msg[3];
    
    // Extract capability
    uint8_t ca = msg[0] & 0x07;
    
    // Extract message (ME field)
    uint8_t me[7];
    memcpy(me, &msg[4], 7);
    
    // Extract type code
    uint8_t tc = (me[0] >> 3) & 0x1F;
    
    // Decode based on type code
    if (tc >= 1 && tc <= 4) {
        // Aircraft identification
        decodeAircraftID(icao, me);
    } else if (tc >= 9 && tc <= 18) {
        // Airborne position
        decodeAirbornePosition(icao, me);
    } else if (tc == 19) {
        // Airborne velocity
        decodeAirborneVelocity(icao, me);
    } else if (tc >= 5 && tc <= 8) {
        // Surface position
        decodeSurfacePosition(icao, me);
    }
    
    // Emit raw message
    QVariantMap data;
    data["type"] = "ADSB_MSG";
    data["icao"] = icao;
    data["tc"] = tc;
    data["ca"] = ca;
    data["me"] = QByteArray(reinterpret_cast<const char*>(me), 7);
    emitData(data);
}

void ADSBDecoder::decodeAircraftID(uint32_t icao, const uint8_t* me) {
    // Extract callsign
    static const char charset[] = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";
    
    char callsign[9];
    callsign[8] = 0;
    
    uint64_t chars = 0;
    for (int i = 1; i < 7; i++) {
        chars = (chars << 8) | me[i];
    }
    
    for (int i = 0; i < 8; i++) {
        callsign[i] = charset[(chars >> (42 - i * 6)) & 0x3F];
    }
    
    // Update aircraft
    Aircraft& ac = aircraft_[icao];
    ac.icao = icao;
    ac.callsign = QString(callsign).trimmed();
    ac.lastSeen = std::chrono::steady_clock::now();
    
    emit aircraftUpdated(icao, ac);
    
#ifdef HAS_SPDLOG
    spdlog::info("ADS-B Aircraft ID: {} = {}", icao, callsign);
#endif
}

void ADSBDecoder::decodeAirbornePosition(uint32_t icao, const uint8_t* me) {
    // Extract altitude
    uint16_t altCode = ((me[1] & 0x1F) << 7) | (me[2] >> 1);
    
    float altitude;
    if (me[1] & 0x08) {
        // 25 foot increments
        altitude = altCode * 25.0f - 1000.0f;
    } else {
        // 100 foot increments
        altitude = altCode * 100.0f - 1000.0f;
    }
    
    // Extract CPR format (odd/even)
    int fflag = me[3] & 0x04 ? 1 : 0;
    
    // Extract encoded lat/lon
    int encoded_lat = ((me[3] & 0x03) << 15) | (me[4] << 7) | (me[5] >> 1);
    int encoded_lon = ((me[5] & 0x01) << 16) | (me[6] << 8) | me[7];
    
    // Update aircraft
    Aircraft& ac = aircraft_[icao];
    ac.icao = icao;
    ac.altitude = altitude;
    ac.lastSeen = std::chrono::steady_clock::now();
    
    // Decode position if we have both even and odd messages
    double lat, lon;
    if (decodeCPRPosition(icao, fflag, encoded_lat, encoded_lon, lat, lon)) {
        ac.latitude = lat;
        ac.longitude = lon;
    }
    
    emit aircraftUpdated(icao, ac);
}

void ADSBDecoder::decodeAirborneVelocity(uint32_t icao, const uint8_t* me) {
    // Subtype
    uint8_t st = me[0] & 0x07;
    
    if (st == 1 || st == 2) {
        // Ground speed
        uint16_t ew_raw = ((me[1] & 0x03) << 8) | me[2];
        uint16_t ns_raw = ((me[3] & 0x7F) << 3) | (me[4] >> 5);
        
        int ew_vel = (me[1] & 0x04) ? -(ew_raw - 1) : (ew_raw - 1);
        int ns_vel = (me[3] & 0x80) ? -(ns_raw - 1) : (ns_raw - 1);
        
        float groundSpeed = sqrtf(ew_vel * ew_vel + ns_vel * ns_vel);
        float track = atan2f(ew_vel, ns_vel) * 180.0f / M_PI;
        if (track < 0) track += 360.0f;
        
        // Vertical rate
        uint16_t vr_raw = ((me[4] & 0x1F) << 4) | (me[5] >> 4);
        int vr = (me[4] & 0x20) ? -(vr_raw - 1) : (vr_raw - 1);
        float verticalRate = vr * 64.0f;  // 64 fpm increments
        
        // Update aircraft
        Aircraft& ac = aircraft_[icao];
        ac.icao = icao;
        ac.groundSpeed = groundSpeed;
        ac.track = track;
        ac.verticalRate = verticalRate;
        ac.lastSeen = std::chrono::steady_clock::now();
        
        emit aircraftUpdated(icao, ac);
    }
}

void ADSBDecoder::decodeSurfacePosition(uint32_t icao, const uint8_t* me) {
    Q_UNUSED(me);  // Would decode surface position from ME field
    // Similar to airborne position but for surface movement
    // Mark aircraft as on ground
    Aircraft& ac = aircraft_[icao];
    ac.icao = icao;
    ac.onGround = true;
    ac.lastSeen = std::chrono::steady_clock::now();
    
    emit aircraftUpdated(icao, ac);
}

bool ADSBDecoder::decodeCPRPosition(uint32_t icao, int fflag, int encoded_lat, int encoded_lon,
                                    double& lat, double& lon) {
    // Compact Position Reporting (CPR) decoding
    // This is a simplified version - real implementation is more complex
    
    CPRState& cpr = cprStates_[icao];
    auto now = std::chrono::steady_clock::now();
    
    if (fflag) {
        // Odd message
        cpr.oddLat = encoded_lat / 131072.0;
        cpr.oddLon = encoded_lon / 131072.0;
        cpr.oddTime = now;
        cpr.hasOdd = true;
    } else {
        // Even message
        cpr.evenLat = encoded_lat / 131072.0;
        cpr.evenLon = encoded_lon / 131072.0;
        cpr.evenTime = now;
        cpr.hasEven = true;
    }
    
    // Check if we have both even and odd messages
    if (!cpr.hasEven || !cpr.hasOdd) {
        return false;
    }
    
    // Check if messages are recent (within 10 seconds)
    auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(
        cpr.evenTime > cpr.oddTime ? (cpr.evenTime - cpr.oddTime) : (cpr.oddTime - cpr.evenTime)).count();
    
    if (timeDiff > 10) {
        return false;
    }
    
    // Simplified global decoding (assumes we're not near poles or date line)
    // Real implementation would be much more complex
    
    const double dlat0 = 360.0 / 60.0;
    const double dlat1 = 360.0 / 59.0;
    
    double j = floor(cpr.evenLat * 59.0 - cpr.oddLat * 60.0 + 0.5);
    double lat_even = dlat0 * (j + cpr.evenLat);
    double lat_odd = dlat1 * (j + cpr.oddLat);
    
    // Use the most recent position
    if (cpr.evenTime > cpr.oddTime) {
        lat = lat_even;
        
        double nl = 1.0;  // Simplified - should calculate based on latitude
        double dlon = 360.0 / nl;
        double m = floor(cpr.evenLon * nl - cpr.oddLon * nl + 0.5);
        lon = dlon * (m + cpr.evenLon);
    } else {
        lat = lat_odd;
        
        double nl = 1.0;  // Simplified
        double dlon = 360.0 / nl;
        double m = floor(cpr.evenLon * nl - cpr.oddLon * nl + 0.5);
        lon = dlon * (m + cpr.oddLon);
    }
    
    // Normalize
    if (lat > 90) lat -= 180;
    if (lat < -90) lat += 180;
    if (lon > 180) lon -= 360;
    if (lon < -180) lon += 360;
    
    return true;
}

std::vector<ADSBDecoder::Aircraft> ADSBDecoder::getAircraft() const {
    std::vector<Aircraft> result;
    for (const auto& pair : aircraft_) {
        result.push_back(pair.second);
    }
    return result;
}

void ADSBDecoder::updateAircraft(uint32_t icao, const Aircraft& update) {
    aircraft_[icao] = update;
    emit aircraftUpdated(icao, update);
}

void ADSBDecoder::removeStaleAircraft() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = aircraft_.begin(); it != aircraft_.end();) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.lastSeen).count();
        
        if (age > 60) {
            emit aircraftLost(it->first);
            it = aircraft_.erase(it);
        } else {
            ++it;
        }
    }
}
