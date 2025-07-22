#include "NoiseReduction.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

NoiseReduction::NoiseReduction(uint32_t sampleRate)
    : sampleRate_(sampleRate)
    , reductionLevel_(0.5f)
    , fftSize_(512)
    , profileLearned_(false)
    , bufferPos_(0) {
    
    // Initialize FFT
    fftIn_ = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize_);
    fftOut_ = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize_);
    fftPlan_ = fftwf_plan_dft_r2c_1d(fftSize_, (float*)fftIn_, fftOut_, FFTW_MEASURE);
    ifftPlan_ = fftwf_plan_dft_c2r_1d(fftSize_, fftOut_, (float*)fftIn_, FFTW_MEASURE);
    
    // Initialize buffers
    inputBuffer_.resize(fftSize_, 0.0f);
    outputBuffer_.resize(fftSize_, 0.0f);
    noiseProfile_.resize(fftSize_ / 2 + 1, 0.0f);
    
    // Create Hann window
    window_.resize(fftSize_);
    for (size_t i = 0; i < fftSize_; i++) {
        window_[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fftSize_ - 1)));
    }
}

NoiseReduction::~NoiseReduction() {
    fftwf_destroy_plan(fftPlan_);
    fftwf_destroy_plan(ifftPlan_);
    fftwf_free(fftIn_);
    fftwf_free(fftOut_);
}

void NoiseReduction::process(const float* input, float* output, size_t length) {
    // Simple passthrough if noise profile not learned
    if (!profileLearned_ || reductionLevel_ == 0.0f) {
        std::copy(input, input + length, output);
        return;
    }
    
    // Process in blocks with 50% overlap
    size_t hopSize = fftSize_ / 2;
    
    for (size_t i = 0; i < length; i++) {
        // Add input to buffer
        inputBuffer_[bufferPos_ + hopSize] = input[i];
        
        // Output from buffer
        output[i] = outputBuffer_[bufferPos_];
        
        bufferPos_++;
        
        // Process block when we have enough data
        if (bufferPos_ >= hopSize) {
            processBlock();
            
            // Shift buffers
            std::copy(inputBuffer_.begin() + hopSize, inputBuffer_.end(), inputBuffer_.begin());
            std::copy(outputBuffer_.begin() + hopSize, outputBuffer_.end(), outputBuffer_.begin());
            std::fill(outputBuffer_.begin() + hopSize, outputBuffer_.end(), 0.0f);
            
            bufferPos_ = 0;
        }
    }
}

void NoiseReduction::processBlock() {
    // Apply window and copy to FFT input
    for (size_t i = 0; i < fftSize_; i++) {
        ((float*)fftIn_)[i] = inputBuffer_[i] * window_[i];
    }
    
    // Forward FFT
    fftwf_execute(fftPlan_);
    
    // Spectral subtraction
    for (size_t i = 0; i < fftSize_ / 2 + 1; i++) {
        float real = fftOut_[i][0];
        float imag = fftOut_[i][1];
        float magnitude = sqrtf(real * real + imag * imag);
        float phase = atan2f(imag, real);
        
        // Subtract noise profile
        float cleanMagnitude = magnitude - reductionLevel_ * noiseProfile_[i];
        cleanMagnitude = std::max(0.0f, cleanMagnitude);
        
        // Reconstruct complex number
        fftOut_[i][0] = cleanMagnitude * cosf(phase);
        fftOut_[i][1] = cleanMagnitude * sinf(phase);
    }
    
    // Inverse FFT
    fftwf_execute(ifftPlan_);
    
    // Add windowed result to output buffer (overlap-add)
    for (size_t i = 0; i < fftSize_; i++) {
        outputBuffer_[i] += ((float*)fftIn_)[i] * window_[i] / fftSize_;
    }
}

void NoiseReduction::learnNoiseProfile() {
    // In a real implementation, this would analyze a noise-only segment
    // For now, just set a basic profile
    for (size_t i = 0; i < noiseProfile_.size(); i++) {
        noiseProfile_[i] = 0.01f; // Low-level noise floor
    }
    profileLearned_ = true;
}

void NoiseReduction::resetNoiseProfile() {
    std::fill(noiseProfile_.begin(), noiseProfile_.end(), 0.0f);
    profileLearned_ = false;
}
