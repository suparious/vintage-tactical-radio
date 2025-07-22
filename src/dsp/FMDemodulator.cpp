#include "FMDemodulator.h"
#include <cmath>

FMDemodulator::FMDemodulator(uint32_t sampleRate, uint32_t bandwidth)
    : sampleRate_(sampleRate)
    , bandwidth_(bandwidth)
    , lastSample_(0.0f, 0.0f)
    , deemphasisAlpha_(1.0f)
    , lastDeemphasis_(0.0f) {
    
    // Set default de-emphasis for US (75us)
    setDeemphasis(75e-6f);
    
    // Initialize audio filter
    updateAudioFilter();
}

void FMDemodulator::demodulate(const std::complex<float>* input, float* output, size_t length) {
    // Quadrature demodulation
    for (size_t i = 0; i < length; i++) {
        // Phase discriminator using arctangent method
        std::complex<float> product = input[i] * std::conj(lastSample_);
        float phase = atan2f(product.imag(), product.real());
        
        // Scale to audio range
        float demod = phase * sampleRate_ / (2.0f * M_PI * bandwidth_);
        
        // Apply de-emphasis filter
        lastDeemphasis_ = demod + deemphasisAlpha_ * (lastDeemphasis_ - demod);
        output[i] = lastDeemphasis_;
        
        lastSample_ = input[i];
    }
    
    // Apply audio filter if needed
    if (!audioFilter_.empty() && audioFilterState_.size() == audioFilter_.size()) {
        // Simple FIR filter
        for (size_t i = 0; i < length; i++) {
            float filtered = 0.0f;
            
            // Shift state
            for (size_t j = audioFilterState_.size() - 1; j > 0; j--) {
                audioFilterState_[j] = audioFilterState_[j - 1];
            }
            audioFilterState_[0] = output[i];
            
            // Apply filter
            for (size_t j = 0; j < audioFilter_.size(); j++) {
                filtered += audioFilter_[j] * audioFilterState_[j];
            }
            
            output[i] = filtered;
        }
    }
}

void FMDemodulator::setBandwidth(uint32_t bandwidth) {
    bandwidth_ = bandwidth;
    updateAudioFilter();
}

void FMDemodulator::setDeemphasis(float timeConstant) {
    // Calculate filter coefficient for first-order IIR de-emphasis filter
    float fc = 1.0f / (2.0f * M_PI * timeConstant);
    deemphasisAlpha_ = expf(-2.0f * M_PI * fc / sampleRate_);
}

void FMDemodulator::updateAudioFilter() {
    // Design a simple low-pass filter based on bandwidth
    // For FM broadcast, we want to pass audio up to 15 kHz
    // For narrow FM, we want a tighter filter
    
    int filterLength = 21; // Keep it short for low latency
    audioFilter_.resize(filterLength);
    audioFilterState_.resize(filterLength, 0.0f);
    
    float cutoff = (bandwidth_ < 50000) ? 3000.0f : 15000.0f; // 3kHz for narrow, 15kHz for wide
    float normalizedCutoff = cutoff / sampleRate_;
    
    // Simple windowed sinc filter
    for (int i = 0; i < filterLength; i++) {
        int n = i - filterLength / 2;
        if (n == 0) {
            audioFilter_[i] = 2.0f * normalizedCutoff;
        } else {
            audioFilter_[i] = sinf(2.0f * M_PI * normalizedCutoff * n) / (M_PI * n);
        }
        
        // Hamming window
        audioFilter_[i] *= 0.54f - 0.46f * cosf(2.0f * M_PI * i / (filterLength - 1));
    }
    
    // Normalize
    float sum = 0.0f;
    for (float coeff : audioFilter_) {
        sum += coeff;
    }
    for (float& coeff : audioFilter_) {
        coeff /= sum;
    }
}
