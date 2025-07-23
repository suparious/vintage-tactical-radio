#include "CTCSSDecoder.h"
#include <QVariantMap>
#include <cmath>
#include <algorithm>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CTCSSDecoder::CTCSSDecoder(QObject* parent)
    : DigitalDecoder(DecoderType::CTCSS, parent)
    , detectionThreshold_(0.1f)  // 10% of full scale
    , detectionTimeMs_(250)      // 250ms continuous detection
    , samplesForDetection_(0)
    , currentTone_(0.0f)
    , currentLevel_(0.0f)
    , toneDetected_(false)
    , blockSize_(0)
    , bufferIndex_(0)
    , noiseLevel_(0.0f) {
    
    // We'll process in blocks for efficiency
    // Block size will be set based on sample rate
}

CTCSSDecoder::~CTCSSDecoder() {
    stop();
}

void CTCSSDecoder::start() {
    if (active_) {
        return;
    }
    
    active_ = true;
    setState(DecoderState::SEARCHING);
    
    // Initialize filters with current sample rate
    initializeFilters();
    
    // Calculate samples needed for detection time
    samplesForDetection_ = (sampleRate_ * detectionTimeMs_) / 1000;
    
    // Reset detection state
    currentTone_ = 0.0f;
    currentLevel_ = 0.0f;
    toneDetected_ = false;
    detectionStart_ = std::chrono::steady_clock::now();
    
#ifdef HAS_SPDLOG
    spdlog::info("CTCSS decoder started - Sample rate: {} Hz, Detection time: {} ms", 
                 sampleRate_, detectionTimeMs_);
#endif
}

void CTCSSDecoder::stop() {
    if (!active_) {
        return;
    }
    
    active_ = false;
    setState(DecoderState::IDLE);
    
    if (toneDetected_) {
        emit toneLost();
        toneDetected_ = false;
    }
    
#ifdef HAS_SPDLOG
    spdlog::info("CTCSS decoder stopped");
#endif
}

void CTCSSDecoder::reset() {
    bool wasActive = active_;
    
    stop();
    
    // Clear all filter states
    for (auto& filter : filters_) {
        filter.reset();
    }
    
    audioBuffer_.clear();
    bufferIndex_ = 0;
    noiseLevel_ = 0.0f;
    
    if (wasActive) {
        start();
    }
}

void CTCSSDecoder::initializeFilters() {
    // Clear existing filters
    filters_.clear();
    
    // For CTCSS tones, we want good frequency resolution
    // Use a block size that gives us ~50ms of data
    blockSize_ = sampleRate_ / 20;  // 50ms blocks
    
    // Resize audio buffer
    audioBuffer_.resize(blockSize_);
    bufferIndex_ = 0;
    
    // Create Goertzel filters for each CTCSS tone
    for (float tone : CTCSS_TONES) {
        GoertzelState state;
        
        // Calculate Goertzel coefficient
        float k = (blockSize_ * tone) / sampleRate_;
        state.coeff = 2.0f * cosf(2.0f * M_PI * k / blockSize_);
        state.s1 = 0.0f;
        state.s2 = 0.0f;
        state.sampleCount = 0;
        state.targetSamples = blockSize_;
        
        filters_.push_back(state);
    }
    
    // Also add filters for extended tones if needed
    for (float tone : EXTENDED_TONES) {
        GoertzelState state;
        
        float k = (blockSize_ * tone) / sampleRate_;
        state.coeff = 2.0f * cosf(2.0f * M_PI * k / blockSize_);
        state.s1 = 0.0f;
        state.s2 = 0.0f;
        state.sampleCount = 0;
        state.targetSamples = blockSize_;
        
        filters_.push_back(state);
    }
    
#ifdef HAS_SPDLOG
    spdlog::debug("Initialized {} CTCSS Goertzel filters, block size: {}", 
                  filters_.size(), blockSize_);
#endif
}

void CTCSSDecoder::processAudio(const float* samples, size_t length) {
    if (!active_) {
        return;
    }
    
    // Process each sample
    for (size_t i = 0; i < length; i++) {
        audioBuffer_[bufferIndex_++] = samples[i];
        
        if (bufferIndex_ >= blockSize_) {
            // Process the complete block
            for (size_t j = 0; j < blockSize_; j++) {
                processSample(audioBuffer_[j]);
            }
            
            // Analyze results
            analyzeResults();
            
            // Reset buffer
            bufferIndex_ = 0;
            
            // Reset filter states for next block
            for (auto& filter : filters_) {
                filter.reset();
            }
        }
    }
}

void CTCSSDecoder::processSample(float sample) {
    // Apply pre-emphasis filter to reduce noise
    // Simple high-pass to remove DC and very low frequencies
    static float prevSample = 0.0f;
    float filtered = sample - 0.95f * prevSample;
    prevSample = sample;
    
    // Update noise level estimate
    float absSample = fabsf(filtered);
    noiseLevel_ = NOISE_ALPHA * noiseLevel_ + (1.0f - NOISE_ALPHA) * absSample;
    
    // Process through each Goertzel filter
    for (auto& filter : filters_) {
        float s0 = filter.coeff * filter.s1 - filter.s2 + filtered;
        filter.s2 = filter.s1;
        filter.s1 = s0;
        filter.sampleCount++;
    }
}

float CTCSSDecoder::GoertzelState::magnitude() const {
    // Calculate magnitude squared
    // magnitude = sqrt(s1^2 + s2^2 - s1*s2*coeff)
    // But we can work with magnitude squared for efficiency
    float real = s1 - s2 * coeff;
    float imag = s2 * sqrtf(1.0f - coeff * coeff);
    return sqrtf(real * real + imag * imag) / (targetSamples / 2);
}

void CTCSSDecoder::analyzeResults() {
    // Find the tone with maximum energy
    float maxMagnitude = 0.0f;
    int maxIndex = -1;
    
    for (size_t i = 0; i < filters_.size(); i++) {
        float mag = filters_[i].magnitude();
        if (mag > maxMagnitude) {
            maxMagnitude = mag;
            maxIndex = i;
        }
    }
    
    // Calculate SNR
    float snr = (noiseLevel_ > 0.0f) ? (maxMagnitude / noiseLevel_) : 0.0f;
    
    // Determine the detected frequency
    float detectedFreq = 0.0f;
    if (maxIndex >= 0) {
        if (static_cast<size_t>(maxIndex) < CTCSS_TONES.size()) {
            detectedFreq = CTCSS_TONES[maxIndex];
        } else {
            detectedFreq = EXTENDED_TONES[maxIndex - CTCSS_TONES.size()];
        }
    }
    
    // Check if tone meets detection criteria
    bool tonePresent = (maxMagnitude > detectionThreshold_) && (snr > 3.0f);
    
    if (tonePresent) {
        if (!toneDetected_ || fabsf(detectedFreq - currentTone_) > 0.5f) {
            // New tone detected or tone changed
            auto now = std::chrono::steady_clock::now();
            
            if (!toneDetected_) {
                // First detection
                detectionStart_ = now;
                currentTone_ = detectedFreq;
                currentLevel_ = maxMagnitude;
                setState(DecoderState::SYNCING);
            } else if (fabsf(detectedFreq - currentTone_) > 0.5f) {
                // Tone changed
                float oldTone = currentTone_;
                currentTone_ = detectedFreq;
                currentLevel_ = maxMagnitude;
                detectionStart_ = now;
                emit toneChanged(oldTone, currentTone_);
                
#ifdef HAS_SPDLOG
                spdlog::info("CTCSS tone changed: {:.1f} Hz -> {:.1f} Hz", oldTone, currentTone_);
#endif
            }
            
            // Check if we've detected long enough
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - detectionStart_).count();
            
            if (elapsed >= detectionTimeMs_) {
                if (!toneDetected_) {
                    toneDetected_ = true;
                    setState(DecoderState::DECODING);
                    emit toneDetected(currentTone_, currentLevel_);
                    
                    // Emit decoded data
                    QVariantMap data;
                    data["type"] = "CTCSS";
                    data["frequency"] = currentTone_;
                    data["level"] = currentLevel_;
                    data["snr"] = snr;
                    emitData(data);
                    
#ifdef HAS_SPDLOG
                    spdlog::info("CTCSS tone detected: {:.1f} Hz, level: {:.3f}, SNR: {:.1f} dB", 
                                currentTone_, currentLevel_, 20.0f * log10f(snr));
#endif
                }
            }
        }
        
        // Update current level
        currentLevel_ = maxMagnitude;
        
    } else if (toneDetected_) {
        // Tone lost
        toneDetected_ = false;
        setState(DecoderState::SEARCHING);
        emit toneLost();
        
#ifdef HAS_SPDLOG
        spdlog::info("CTCSS tone lost: {:.1f} Hz", currentTone_);
#endif
        
        currentTone_ = 0.0f;
        currentLevel_ = 0.0f;
    }
}
