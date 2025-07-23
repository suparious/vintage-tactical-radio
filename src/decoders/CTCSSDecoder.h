#ifndef CTCSSDECODER_H
#define CTCSSDECODER_H

#include "DigitalDecoder.h"
#include <array>
#include <chrono>

class CTCSSDecoder : public DigitalDecoder {
    Q_OBJECT
    
public:
    explicit CTCSSDecoder(QObject* parent = nullptr);
    ~CTCSSDecoder() override;
    
    // Control methods
    void start() override;
    void stop() override;
    void reset() override;
    
    // Process audio samples
    void processAudio(const float* samples, size_t length) override;
    
    // Configuration
    void setDetectionThreshold(float threshold) { detectionThreshold_ = threshold; }
    float getDetectionThreshold() const { return detectionThreshold_; }
    
    void setDetectionTime(int milliseconds) { detectionTimeMs_ = milliseconds; }
    int getDetectionTime() const { return detectionTimeMs_; }
    
    // Get detected tone info
    float getCurrentTone() const { return currentTone_; }
    float getCurrentLevel() const { return currentLevel_; }
    
    // Standard CTCSS tones
    static constexpr std::array<float, 50> CTCSS_TONES = {
        67.0f, 69.3f, 71.9f, 74.4f, 77.0f, 79.7f, 82.5f, 85.4f,
        88.5f, 91.5f, 94.8f, 97.4f, 100.0f, 103.5f, 107.2f, 110.9f,
        114.8f, 118.8f, 123.0f, 127.3f, 131.8f, 136.5f, 141.3f, 146.2f,
        151.4f, 156.7f, 159.8f, 162.2f, 165.5f, 167.9f, 171.3f, 173.8f,
        177.3f, 179.9f, 183.5f, 186.2f, 189.9f, 192.8f, 196.6f, 199.5f,
        203.5f, 206.5f, 210.7f, 218.1f, 225.7f, 229.1f, 233.6f, 241.8f,
        250.3f, 254.1f
    };
    
    // Extended tones for some systems
    static constexpr std::array<float, 12> EXTENDED_TONES = {
        // Some Motorola systems use these additional tones
        69.4f, 71.0f, 77.7f, 85.0f, 97.3f, 105.0f,
        111.8f, 116.8f, 120.0f, 125.0f, 135.0f, 235.0f
    };
    
signals:
    void toneDetected(float frequency, float level);
    void toneChanged(float oldFreq, float newFreq);
    void toneLost();
    
private:
    // Goertzel algorithm for efficient single-frequency detection
    struct GoertzelState {
        float coeff;
        float s1;
        float s2;
        size_t sampleCount;
        size_t targetSamples;
        
        void reset() {
            s1 = 0.0f;
            s2 = 0.0f;
            sampleCount = 0;
        }
        
        float magnitude() const;
    };
    
    // Initialize Goertzel coefficients for all tones
    void initializeFilters();
    
    // Process a single sample through all Goertzel filters
    void processSample(float sample);
    
    // Check if we have enough samples and analyze results
    void analyzeResults();
    
    // Detection parameters
    float detectionThreshold_;
    int detectionTimeMs_;
    size_t samplesForDetection_;
    
    // Current detection state
    float currentTone_;
    float currentLevel_;
    std::chrono::steady_clock::time_point detectionStart_;
    bool toneDetected_;
    
    // Goertzel filter states for each tone
    std::vector<GoertzelState> filters_;
    size_t blockSize_;
    
    // Audio buffer for processing
    std::vector<float> audioBuffer_;
    size_t bufferIndex_;
    
    // Moving average for noise estimation
    float noiseLevel_;
    static constexpr float NOISE_ALPHA = 0.95f;
};

#endif // CTCSSDECODER_H
