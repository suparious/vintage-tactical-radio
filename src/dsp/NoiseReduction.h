#ifndef NOISEREDUCTION_H
#define NOISEREDUCTION_H

#include <vector>
#include <complex>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint32_t
#include <fftw3.h>

class NoiseReduction {
public:
    explicit NoiseReduction(uint32_t sampleRate);
    ~NoiseReduction();
    
    void process(const float* input, float* output, size_t length);
    
    void setLevel(float level) { reductionLevel_ = level; }
    float getLevel() const { return reductionLevel_; }
    
    // Learn noise profile from current audio
    void learnNoiseProfile();
    void resetNoiseProfile();
    
private:
    uint32_t sampleRate_;
    float reductionLevel_;
    
    // FFT for spectral subtraction
    size_t fftSize_;
    fftwf_plan fftPlan_;
    fftwf_plan ifftPlan_;
    fftwf_complex* fftIn_;
    fftwf_complex* fftOut_;
    
    // Noise profile
    std::vector<float> noiseProfile_;
    bool profileLearned_;
    
    // Overlap-add buffers
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    std::vector<float> window_;
    size_t bufferPos_;
    
    void processBlock();
};

#endif // NOISEREDUCTION_H
