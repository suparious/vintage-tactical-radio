#include "SSBDemodulator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SSBDemodulator::SSBDemodulator(uint32_t sampleRate, Mode mode)
    : sampleRate_(sampleRate)
    , mode_(mode)
    , bandwidth_(2800)  // Default SSB bandwidth
    , cwPitch_(700.0f)  // Default CW pitch
    , cwBandwidth_(200.0f)  // Default CW filter bandwidth
    , cwPhase_(0.0f)
    , agcLevel_(1.0f)
    , agcAttack_(0.01f)
    , agcDecay_(0.1f) {
    
    hilbert_ = std::make_unique<HilbertTransform>();
    generateLPFCoeffs();
    lpfDelay_.resize(lpfCoeffs_.size(), std::complex<float>(0, 0));
}

SSBDemodulator::~SSBDemodulator() = default;

void SSBDemodulator::demodulate(const std::complex<float>* input, float* output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        std::complex<float> sample = input[i];
        
        // Apply low-pass filter for bandwidth limiting
        sample = applyLPF(sample);
        
        float demodulated = 0.0f;
        
        switch (mode_) {
            case USB: {
                // Upper sideband: use real part of analytic signal
                // Shift down by BFO frequency (implicit in the complex multiplication)
                demodulated = sample.real();
                break;
            }
            
            case LSB: {
                // Lower sideband: use real part but with conjugate
                // This effectively flips the spectrum
                demodulated = std::conj(sample).real();
                break;
            }
            
            case CW: {
                // CW: Similar to SSB but with narrow filter and BFO
                // First demodulate as USB or LSB
                demodulated = sample.real();
                
                // Generate CW tone at specified pitch
                float bfo = cosf(2.0f * M_PI * cwPitch_ * i / sampleRate_ + cwPhase_);
                demodulated *= bfo;
                
                // Apply envelope detection for CW
                demodulated = fabsf(demodulated);
                
                // Update phase accumulator
                cwPhase_ += 2.0f * M_PI * cwPitch_ / sampleRate_;
                if (cwPhase_ > 2.0f * M_PI) {
                    cwPhase_ -= 2.0f * M_PI;
                }
                break;
            }
        }
        
        // Apply simple AGC
        float magnitude = fabsf(demodulated);
        if (magnitude > agcLevel_) {
            agcLevel_ = agcLevel_ * (1.0f - agcAttack_) + magnitude * agcAttack_;
        } else {
            agcLevel_ = agcLevel_ * (1.0f - agcDecay_) + magnitude * agcDecay_;
        }
        
        // Normalize with AGC
        if (agcLevel_ > 0.001f) {
            demodulated /= agcLevel_;
        }
        
        // Soft clipping to prevent harsh distortion
        if (fabsf(demodulated) > 0.95f) {
            demodulated = (demodulated > 0.0f ? 1.0f : -1.0f) * 
                         (1.0f - expf(-3.0f * fabsf(demodulated)));
        }
        
        output[i] = demodulated * 0.5f;  // Scale output
    }
}

void SSBDemodulator::setBandwidth(uint32_t bandwidth) {
    bandwidth_ = bandwidth;
    generateLPFCoeffs();
}

void SSBDemodulator::generateLPFCoeffs() {
    // Simple FIR low-pass filter design using windowed sinc
    const size_t numTaps = 65;
    lpfCoeffs_.resize(numTaps);
    
    float cutoffFreq = (mode_ == CW) ? cwBandwidth_ : bandwidth_;
    float normalizedCutoff = cutoffFreq / sampleRate_;
    
    // Generate sinc function
    for (size_t i = 0; i < numTaps; i++) {
        float n = i - (numTaps - 1) / 2.0f;
        if (n == 0) {
            lpfCoeffs_[i] = 2.0f * normalizedCutoff;
        } else {
            lpfCoeffs_[i] = sinf(2.0f * M_PI * normalizedCutoff * n) / (M_PI * n);
        }
        
        // Apply Hamming window
        float window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (numTaps - 1));
        lpfCoeffs_[i] *= window;
    }
    
    // Normalize coefficients
    float sum = 0.0f;
    for (float coeff : lpfCoeffs_) {
        sum += coeff;
    }
    for (float& coeff : lpfCoeffs_) {
        coeff /= sum;
    }
}

std::complex<float> SSBDemodulator::applyLPF(std::complex<float> sample) {
    // Shift delay line
    for (size_t i = lpfDelay_.size() - 1; i > 0; i--) {
        lpfDelay_[i] = lpfDelay_[i - 1];
    }
    lpfDelay_[0] = sample;
    
    // Apply FIR filter
    std::complex<float> output(0, 0);
    for (size_t i = 0; i < lpfCoeffs_.size(); i++) {
        output += lpfDelay_[i] * lpfCoeffs_[i];
    }
    
    return output;
}

// Hilbert Transform implementation
SSBDemodulator::HilbertTransform::HilbertTransform(size_t length)
    : delayIndex_(0) {
    generateCoefficients(length);
    delayLine_.resize(coeffs_.size(), 0.0f);
}

std::complex<float> SSBDemodulator::HilbertTransform::process(float sample) {
    // Add sample to delay line
    delayLine_[delayIndex_] = sample;
    
    // Calculate Hilbert transform (imaginary part)
    float hilbertOut = 0.0f;
    size_t center = coeffs_.size() / 2;
    
    for (size_t i = 0; i < coeffs_.size(); i++) {
        size_t idx = (delayIndex_ + i) % coeffs_.size();
        hilbertOut += delayLine_[idx] * coeffs_[i];
    }
    
    // Get delayed real part (to match Hilbert filter delay)
    size_t realIdx = (delayIndex_ + center) % coeffs_.size();
    float realOut = delayLine_[realIdx];
    
    // Update delay index
    delayIndex_ = (delayIndex_ + 1) % coeffs_.size();
    
    return std::complex<float>(realOut, hilbertOut);
}

void SSBDemodulator::HilbertTransform::generateCoefficients(size_t length) {
    coeffs_.resize(length);
    
    // Generate ideal Hilbert transformer coefficients
    for (size_t i = 0; i < length; i++) {
        float n = i - (length - 1) / 2.0f;
        
        if ((int)n % 2 == 0) {
            // Even indices are zero (except center)
            coeffs_[i] = 0.0f;
        } else {
            // Odd indices: 2/(π*n)
            coeffs_[i] = 2.0f / (M_PI * n);
        }
        
        // Apply Hamming window
        float window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (length - 1));
        coeffs_[i] *= window;
    }
}
