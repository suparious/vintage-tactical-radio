#include "FMDemodulator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    // Quadrature demodulation with improved phase discrimination
    for (size_t i = 0; i < length; i++) {
        // Avoid division by zero
        if (std::abs(input[i]) < 1e-10f) {
            output[i] = 0.0f;
            lastSample_ = input[i];
            continue;
        }
        
        // Phase discriminator using arctangent method
        std::complex<float> product = input[i] * std::conj(lastSample_);
        float phase = atan2f(product.imag(), product.real());
        
        // Scale to audio range
        // FM deviation for broadcast is ±75 kHz
        // The phase difference represents frequency deviation
        float demod = phase * sampleRate_ / (2.0f * M_PI);
        
        // Normalize by maximum deviation
        // Use dynamic deviation based on bandwidth
        float maxDeviation;
        if (bandwidth_ >= 200000) {
            maxDeviation = 75000.0f;  // ±75 kHz for FM broadcast
        } else if (bandwidth_ >= 50000) {
            maxDeviation = 25000.0f;  // ±25 kHz for wide FM
        } else {
            maxDeviation = 5000.0f;   // ±5 kHz for narrow FM
        }
        demod = demod / maxDeviation;
        
        // Clamp to prevent overmodulation artifacts
        // Allow some headroom for strong signals
        demod = std::max(-1.5f, std::min(1.5f, demod));
        
        // Apply de-emphasis filter (first-order IIR low-pass)
        // This reduces high-frequency noise and restores proper audio balance
        float deemphasized = (1.0f - deemphasisAlpha_) * demod + deemphasisAlpha_ * lastDeemphasis_;
        lastDeemphasis_ = deemphasized;
        output[i] = deemphasized;
        
        lastSample_ = input[i];
    }
    
    // Apply audio filter if needed (for bandwidth limiting)
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
    // Standard values: 75μs (US), 50μs (Europe)
    float fc = 1.0f / (2.0f * M_PI * timeConstant);
    float RC = 1.0f / (2.0f * M_PI * fc);
    float dt = 1.0f / sampleRate_;
    deemphasisAlpha_ = RC / (RC + dt);
}

void FMDemodulator::updateAudioFilter() {
    // Design a simple low-pass filter based on bandwidth
    // For FM broadcast, we want to pass audio up to 15 kHz
    // For narrow FM, we want a tighter filter
    
    int filterLength = 21; // Keep it short for low latency
    audioFilter_.resize(filterLength);
    audioFilterState_.resize(filterLength, 0.0f);
    
    // Adjust cutoff based on bandwidth
    float cutoff;
    if (bandwidth_ < 50000) {
        cutoff = 3000.0f; // 3kHz for narrow FM
    } else if (bandwidth_ >= 200000) {
        cutoff = 15000.0f; // 15kHz for broadcast FM (mono audio)
    } else {
        // Scale between narrow and wide
        cutoff = 3000.0f + (bandwidth_ - 50000.0f) / 150000.0f * 12000.0f;
    }
    float normalizedCutoff = cutoff / sampleRate_;
    
    // Simple windowed sinc filter
    for (int i = 0; i < filterLength; i++) {
        int n = i - filterLength / 2;
        if (n == 0) {
            audioFilter_[i] = 2.0f * normalizedCutoff;
        } else {
            audioFilter_[i] = sinf(2.0f * M_PI * normalizedCutoff * n) / (M_PI * n);
        }
        
        // Hamming window for reduced sidelobes
        audioFilter_[i] *= 0.54f - 0.46f * cosf(2.0f * M_PI * i / (filterLength - 1));
    }
    
    // Normalize filter coefficients
    float sum = 0.0f;
    for (float coeff : audioFilter_) {
        sum += coeff;
    }
    for (float& coeff : audioFilter_) {
        coeff /= sum;
    }
}
