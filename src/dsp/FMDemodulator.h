#ifndef FMDEMODULATOR_H
#define FMDEMODULATOR_H

#include <complex>
#include <vector>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint32_t

class FMDemodulator {
public:
    FMDemodulator(uint32_t sampleRate, uint32_t bandwidth);
    ~FMDemodulator() = default;
    
    void demodulate(const std::complex<float>* input, float* output, size_t length);
    
    void setBandwidth(uint32_t bandwidth);
    uint32_t getBandwidth() const { return bandwidth_; }
    
    // De-emphasis filter (50us for Europe, 75us for US)
    void setDeemphasis(float timeConstant);
    
private:
    uint32_t sampleRate_;
    uint32_t bandwidth_;
    
    // Phase discriminator state
    std::complex<float> lastSample_;
    
    // De-emphasis filter
    float deemphasisAlpha_;
    float lastDeemphasis_;
    
    // Audio filter coefficients
    std::vector<float> audioFilter_;
    std::vector<float> audioFilterState_;
    
    void updateAudioFilter();
};

#endif // FMDEMODULATOR_H
