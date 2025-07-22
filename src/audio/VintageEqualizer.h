#ifndef VINTAGEEQUALIZER_H
#define VINTAGEEQUALIZER_H

#include <vector>
#include <string>
#include <map>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint32_t

class VintageEqualizer {
public:
    enum Mode {
        MODERN,
        NOSTALGIC
    };
    
    struct Band {
        float frequency;
        float gain;      // in dB
        float q;         // Q factor
    };
    
    struct Preset {
        std::string name;
        std::vector<float> gains; // 7 values in dB
    };
    
    VintageEqualizer(uint32_t sampleRate, Mode mode = MODERN);
    ~VintageEqualizer() = default;
    
    // Process audio
    void process(const float* input, float* output, size_t length);
    
    // Mode control
    void setMode(Mode mode);
    Mode getMode() const { return mode_; }
    
    // Band control
    void setBandGain(int band, float gain);
    float getBandGain(int band) const;
    void setBandQ(int band, float q);
    float getBandQ(int band) const;
    
    // Preamp
    void setPreampGain(float gain) { preampGain_ = gain; }
    float getPreampGain() const { return preampGain_; }
    
    // Gain range
    void setMaxGain(float maxGain) { maxGain_ = maxGain; }
    float getMaxGain() const { return maxGain_; }
    
    // Presets
    void loadPreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
    static std::vector<Preset> getDefaultPresets();
    
    // Reset
    void reset();
    
private:
    uint32_t sampleRate_;
    Mode mode_;
    float preampGain_;
    float maxGain_;
    
    // Band parameters
    std::vector<Band> bands_;
    static const std::vector<float> modernFrequencies_;
    static const std::vector<float> nostalgicFrequencies_;
    
    // Filter coefficients and state
    struct BiquadFilter {
        float b0, b1, b2;  // Feedforward coefficients
        float a1, a2;      // Feedback coefficients
        float x1, x2;      // Input history
        float y1, y2;      // Output history
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }
        
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = output;
            return output;
        }
    };
    
    std::vector<BiquadFilter> filters_;
    
    // Preset storage
    static std::map<std::string, Preset> presets_;
    
    // Update filter coefficients
    void updateFilter(int band);
    void calculatePeakingCoefficients(float frequency, float gain, float q,
                                     float& b0, float& b1, float& b2,
                                     float& a1, float& a2);
};

#endif // VINTAGEEQUALIZER_H
