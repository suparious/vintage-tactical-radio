#include "AMDemodulator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AMDemodulator::AMDemodulator(uint32_t sampleRate)
    : sampleRate_(sampleRate)
    , carrierTracking_(false)
    , dcAlpha_(0.995f) // DC removal filter coefficient
    , lastDC_(0.0f)
    , pllPhase_(0.0f)
    , pllFreq_(0.0f)
    , pllAlpha_(0.01f)
    , pllBeta_(0.001f) {
}

void AMDemodulator::demodulate(const std::complex<float>* input, float* output, size_t length) {
    if (carrierTracking_) {
        // Synchronous AM detection with carrier tracking
        for (size_t i = 0; i < length; i++) {
            // Generate local carrier
            std::complex<float> carrier(cosf(pllPhase_), sinf(pllPhase_));
            
            // Mix with input signal
            std::complex<float> mixed = input[i] * std::conj(carrier);
            
            // Extract I component (in-phase with carrier)
            float demod = mixed.real();
            
            // Phase detector for PLL
            float phaseError = atan2f(mixed.imag(), mixed.real());
            
            // Loop filter
            pllFreq_ += pllBeta_ * phaseError;
            pllPhase_ += pllFreq_ + pllAlpha_ * phaseError;
            
            // Wrap phase
            while (pllPhase_ > M_PI) pllPhase_ -= 2.0f * M_PI;
            while (pllPhase_ < -M_PI) pllPhase_ += 2.0f * M_PI;
            
            // DC removal
            lastDC_ = dcAlpha_ * lastDC_ + (1.0f - dcAlpha_) * demod;
            output[i] = demod - lastDC_;
        }
    } else {
        // Simple envelope detection
        for (size_t i = 0; i < length; i++) {
            // Calculate magnitude (envelope)
            float envelope = std::abs(input[i]);
            
            // DC removal
            lastDC_ = dcAlpha_ * lastDC_ + (1.0f - dcAlpha_) * envelope;
            output[i] = envelope - lastDC_;
        }
    }
}
