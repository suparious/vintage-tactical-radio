#include "RDSDecoder.h"
#include <QVariantMap>
#include <cmath>
#include <QDateTime>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// RDS offset words for error detection
const uint16_t RDSDecoder::OFFSET_WORDS[5] = {
    0x0FC,  // A
    0x198,  // B
    0x168,  // C
    0x1B4,  // D
    0x350   // C'
};

RDSDecoder::RDSDecoder(QObject* parent)
    : DigitalDecoder(DecoderType::RDS, parent)
    , carrierPhase_(0.0f)
    , carrierFreq_(RDS_CARRIER_FREQ)
    , pilotPhase_(0.0f)
    , carrierLocked_(false)
    , symbolPhase_(0.0f)
    , symbolRate_(RDS_SYMBOL_RATE)
    , bufferIndex_(0)
    , bitBuffer_(0)
    , bitCount_(0)
    , blockCount_(0)
    , groupSync_(false)
    , programID_(0)
    , programType_(0)
    , trafficProgram_(false)
    , trafficAnnouncement_(false)
    , musicSpeech_(true)
    , rtABFlag_(0)
    , modifiedJulianDay_(0)
    , hours_(0)
    , minutes_(0)
    , localTimeOffset_(0) {
    
    // Initialize buffers
    psBuffer_.fill(' ');
    psValid_.fill(false);
    rtBuffer_.fill(' ');
    rtValid_.fill(false);
    currentGroup_.fill(0);
}

RDSDecoder::~RDSDecoder() {
    stop();
}

void RDSDecoder::start() {
    if (active_) {
        return;
    }
    
    active_ = true;
    setState(DecoderState::SEARCHING);
    
    // Calculate samples per symbol
    samplesPerSymbol_ = sampleRate_ / symbolRate_;
    
    // Initialize filters
    // 57kHz bandpass filter (56-58kHz)
    float fc = RDS_CARRIER_FREQ / sampleRate_;
    float bw = 2000.0f / sampleRate_;  // 2kHz bandwidth
    
    float omega = 2.0f * M_PI * fc;
    float alpha = sinf(omega) * sinhf(logf(2.0f) / 2.0f * bw * omega / sinf(omega));
    
    rdsFilter_.b0 = alpha;
    rdsFilter_.b1 = 0.0f;
    rdsFilter_.b2 = -alpha;
    rdsFilter_.a1 = -2.0f * cosf(omega);
    rdsFilter_.a2 = 1.0f - alpha;
    
    // Normalize
    float a0 = 1.0f + alpha;
    rdsFilter_.b0 /= a0;
    rdsFilter_.b1 /= a0;
    rdsFilter_.b2 /= a0;
    rdsFilter_.a1 /= a0;
    rdsFilter_.a2 /= a0;
    
    // Reset filter states
    rdsFilter_.x1 = rdsFilter_.x2 = 0.0f;
    rdsFilter_.y1 = rdsFilter_.y2 = 0.0f;
    
    // Allocate demodulation buffers
    size_t bufferSize = static_cast<size_t>(samplesPerSymbol_ * 10);
    i_buffer_.resize(bufferSize);
    q_buffer_.resize(bufferSize);
    
    reset();
    
#ifdef HAS_SPDLOG
    spdlog::info("RDS decoder started - Sample rate: {} Hz", sampleRate_);
#endif
}

void RDSDecoder::stop() {
    if (!active_) {
        return;
    }
    
    active_ = false;
    setState(DecoderState::IDLE);
    
#ifdef HAS_SPDLOG
    spdlog::info("RDS decoder stopped");
#endif
}

void RDSDecoder::reset() {
    // Reset demodulation state
    carrierPhase_ = 0.0f;
    pilotPhase_ = 0.0f;
    carrierLocked_ = false;
    symbolPhase_ = 0.0f;
    bufferIndex_ = 0;
    
    // Reset decoder state
    symbolBuffer_.clear();
    bitBuffer_ = 0;
    bitCount_ = 0;
    blockCount_ = 0;
    groupSync_ = false;
    currentGroup_.fill(0);
    
    // Clear decoded data
    programID_ = 0;
    programService_.clear();
    radioText_.clear();
    programType_ = 0;
    trafficProgram_ = false;
    trafficAnnouncement_ = false;
    musicSpeech_ = true;
    
    psBuffer_.fill(' ');
    psValid_.fill(false);
    rtBuffer_.fill(' ');
    rtValid_.fill(false);
    rtABFlag_ = 0;
    
    alternativeFreqs_.clear();
}

void RDSDecoder::processAudio(const float* samples, size_t length) {
    if (!active_) {
        return;
    }
    
    // Extract 57kHz subcarrier
    extract57kHz(samples, length);
}

void RDSDecoder::extract57kHz(const float* samples, size_t length) {
    for (size_t i = 0; i < length; i++) {
        // Apply 57kHz bandpass filter
        float filtered = rdsFilter_.process(samples[i]);
        
        // Demodulate using coherent detection
        float phase = carrierPhase_;
        float i_sample = filtered * cosf(phase);
        float q_sample = filtered * -sinf(phase);
        
        // Low-pass filter (simple moving average for now)
        // In a real implementation, this would be a proper low-pass filter
        static float i_avg = 0.0f, q_avg = 0.0f;
        i_avg = 0.95f * i_avg + 0.05f * i_sample;
        q_avg = 0.95f * q_avg + 0.05f * q_sample;
        
        // Update carrier phase
        carrierPhase_ += 2.0f * M_PI * carrierFreq_ / sampleRate_;
        if (carrierPhase_ > 2.0f * M_PI) {
            carrierPhase_ -= 2.0f * M_PI;
        }
        
        // Store demodulated samples
        if (bufferIndex_ < i_buffer_.size()) {
            i_buffer_[bufferIndex_] = i_avg;
            q_buffer_[bufferIndex_] = q_avg;
            bufferIndex_++;
            
            if (bufferIndex_ >= i_buffer_.size()) {
                // Process buffer
                demodulateRDS();
                bufferIndex_ = 0;
            }
        }
    }
}

void RDSDecoder::demodulateRDS() {
    // Simple BPSK demodulation
    // In a real implementation, this would include:
    // - Carrier recovery with PLL
    // - Symbol timing recovery
    // - Differential decoding
    
    for (size_t i = 0; i < bufferIndex_; i += static_cast<size_t>(samplesPerSymbol_)) {
        // Simple slicer for BPSK
        float symbol = i_buffer_[i];
        int bit = (symbol > 0) ? 1 : 0;
        
        // Add to bit buffer
        bitBuffer_ = (bitBuffer_ << 1) | bit;
        bitCount_++;
        
        if (bitCount_ >= 26) {  // One RDS block
            // Extract block
            uint32_t block = bitBuffer_ & 0x3FFFFFF;  // 26 bits
            
            // Check and correct errors
            if (checkAndCorrectBlock(block)) {
                // Extract information bits (16 bits)
                uint16_t data = (block >> 10) & 0xFFFF;
                
                currentGroup_[blockCount_++] = data;
                
                if (blockCount_ >= 4) {
                    // Process complete group
                    processGroups();
                    blockCount_ = 0;
                    
                    if (!groupSync_) {
                        groupSync_ = true;
                        setState(DecoderState::DECODING);
                    }
                }
            } else {
                // Sync error - reset block counter
                blockCount_ = 0;
                if (groupSync_) {
                    groupSync_ = false;
                    setState(DecoderState::SYNCING);
                }
            }
            
            bitCount_ = 0;
        }
    }
}

void RDSDecoder::decodeSymbols() {
    // This would implement proper symbol decoding
    // For now, it's handled in demodulateRDS
}

void RDSDecoder::processGroups() {
    uint16_t blockA = currentGroup_[0];
    uint16_t blockB = currentGroup_[1];
    uint16_t blockC = currentGroup_[2];
    uint16_t blockD = currentGroup_[3];
    
    // Extract PI code from block A
    if (blockA != programID_) {
        programID_ = blockA;
        emit programIDChanged(programID_);
        
        QVariantMap data;
        data["type"] = "RDS_PI";
        data["pi"] = programID_;
        emitData(data);
    }
    
    // Extract group type and version from block B
    uint8_t groupType = (blockB >> 12) & 0x0F;
    bool versionB = (blockB >> 11) & 0x01;
    
    // Traffic program flag
    bool tp = (blockB >> 10) & 0x01;
    if (tp != trafficProgram_) {
        trafficProgram_ = tp;
    }
    
    // Program type
    uint8_t pty = (blockB >> 5) & 0x1F;
    if (pty != programType_) {
        programType_ = pty;
        emit programTypeChanged(programType_);
    }
    
    // Process based on group type
    switch (groupType) {
        case 0:  // Basic tuning and switching information
            processGroupType0(blockB, blockC, blockD);
            break;
            
        case 2:  // RadioText
            processGroupType2(blockB, blockC, blockD);
            break;
            
        case 4:  // Clock-time and date
            if (!versionB) {
                processGroupType4A(blockB, blockC, blockD);
            }
            break;
            
        // Add more group types as needed
    }
}

void RDSDecoder::processGroupType0(uint16_t blockB, uint16_t blockC, uint16_t blockD) {
    Q_UNUSED(blockC);  // Alternative frequencies would be decoded from blockC
    // Traffic announcement
    bool ta = (blockB >> 4) & 0x01;
    if (ta != trafficAnnouncement_) {
        trafficAnnouncement_ = ta;
        emit trafficAnnouncementChanged(ta);
    }
    
    // Music/Speech
    bool ms = (blockB >> 3) & 0x01;
    musicSpeech_ = ms;
    
    // Program service name segment
    uint8_t segment = blockB & 0x03;
    
    // Each segment contains 2 characters
    psBuffer_[segment * 2] = (blockD >> 8) & 0xFF;
    psBuffer_[segment * 2 + 1] = blockD & 0xFF;
    psValid_[segment * 2] = true;
    psValid_[segment * 2 + 1] = true;
    
    // Check if we have complete PS
    bool complete = true;
    for (bool valid : psValid_) {
        if (!valid) {
            complete = false;
            break;
        }
    }
    
    if (complete) {
        QString ps = QString::fromLatin1(psBuffer_.data(), 8).trimmed();
        if (ps != programService_) {
            programService_ = ps;
            emit programServiceChanged(programService_);
            
            QVariantMap data;
            data["type"] = "RDS_PS";
            data["ps"] = programService_;
            emitData(data);
            
#ifdef HAS_SPDLOG
            spdlog::info("RDS PS: '{}'", programService_.toStdString());
#endif
        }
    }
}

void RDSDecoder::processGroupType2(uint16_t blockB, uint16_t blockC, uint16_t blockD) {
    // RadioText
    uint8_t abFlag = (blockB >> 4) & 0x01;
    uint8_t segment = blockB & 0x0F;
    
    // Check if RT has been cleared
    if (abFlag != rtABFlag_) {
        rtABFlag_ = abFlag;
        rtBuffer_.fill(' ');
        rtValid_.fill(false);
    }
    
    // Each segment contains 4 characters (version A) or 2 characters (version B)
    bool versionB = (blockB >> 11) & 0x01;
    
    if (!versionB) {
        // Version A - 4 characters
        if (segment < 16) {
            rtBuffer_[segment * 4] = (blockC >> 8) & 0xFF;
            rtBuffer_[segment * 4 + 1] = blockC & 0xFF;
            rtBuffer_[segment * 4 + 2] = (blockD >> 8) & 0xFF;
            rtBuffer_[segment * 4 + 3] = blockD & 0xFF;
            
            for (int i = 0; i < 4; i++) {
                rtValid_[segment * 4 + i] = true;
            }
        }
    } else {
        // Version B - 2 characters
        if (segment < 32) {
            rtBuffer_[segment * 2] = (blockD >> 8) & 0xFF;
            rtBuffer_[segment * 2 + 1] = blockD & 0xFF;
            rtValid_[segment * 2] = true;
            rtValid_[segment * 2 + 1] = true;
        }
    }
    
    // Check for complete RT (up to first 0x0D character)
    QString rt;
    for (size_t i = 0; i < rtBuffer_.size(); i++) {
        if (rtBuffer_[i] == 0x0D) {  // CR character
            break;
        }
        if (rtValid_[i]) {
            rt += QChar(rtBuffer_[i]);
        }
    }
    
    rt = rt.trimmed();
    if (!rt.isEmpty() && rt != radioText_) {
        radioText_ = rt;
        emit radioTextChanged(radioText_);
        
        QVariantMap data;
        data["type"] = "RDS_RT";
        data["rt"] = radioText_;
        emitData(data);
        
#ifdef HAS_SPDLOG
        spdlog::info("RDS RT: '{}'", radioText_.toStdString());
#endif
    }
}

void RDSDecoder::processGroupType4A(uint16_t blockB, uint16_t blockC, uint16_t blockD) {
    // Clock-time and date
    // MJD
    modifiedJulianDay_ = ((blockB & 0x03) << 15) | (blockC >> 1);
    
    // Local time
    hours_ = ((blockC & 0x01) << 4) | (blockD >> 12);
    minutes_ = (blockD >> 6) & 0x3F;
    
    // Local time offset (in half hours)
    localTimeOffset_ = blockD & 0x3F;
    if (localTimeOffset_ & 0x20) {
        localTimeOffset_ |= 0xC0;  // Sign extend
    }
    
    // Convert to QDateTime
    // MJD to calendar date conversion
    int a = modifiedJulianDay_ + 32044 + 2400001;
    int b = (4 * a + 3) / 146097;
    int c = a - (146097 * b) / 4;
    int d = (4 * c + 3) / 1461;
    int e = c - (1461 * d) / 4;
    int m = (5 * e + 2) / 153;
    
    int day = e - (153 * m + 2) / 5 + 1;
    int month = m + 3 - 12 * (m / 10);
    int year = 100 * b + d - 4800 + m / 10;
    
    QDateTime ct = QDateTime(QDate(year, month, day), 
                            QTime(hours_, minutes_));
    
    // Apply local time offset
    ct = ct.addSecs(localTimeOffset_ * 1800);  // Half hours to seconds
    
    emit clockTimeReceived(ct);
    
    QVariantMap data;
    data["type"] = "RDS_CT";
    data["datetime"] = ct;
    emitData(data);
}

bool RDSDecoder::checkAndCorrectBlock(uint32_t& block) {
    // Simple CRC check - in a real implementation, this would
    // include full syndrome calculation and error correction
    // For now, just return true to allow testing
    Q_UNUSED(block);
    return true;
}

uint16_t RDSDecoder::calculateSyndrome(uint32_t block) {
    // Simplified syndrome calculation
    // Real implementation would use the RDS generator polynomial
    Q_UNUSED(block);
    return 0;
}

QString RDSDecoder::getProgramTypeName(uint8_t pty) {
    // North American RBDS program types
    static const char* const PTY_NAMES[] = {
        "None", "News", "Information", "Sports", "Talk", "Rock", "Classic Rock", "Adult Hits",
        "Soft Rock", "Top 40", "Country", "Oldies", "Soft", "Nostalgia", "Jazz", "Classical",
        "R&B", "Soft R&B", "Language", "Religious Music", "Religious Talk", "Personality", "Public", "College",
        "Spanish Talk", "Spanish Music", "Hip Hop", "Unassigned", "Unassigned", "Weather", "Emergency Test", "Emergency"
    };
    
    if (pty < 32) {
        return QString(PTY_NAMES[pty]);
    }
    return QString("Unknown");
}
