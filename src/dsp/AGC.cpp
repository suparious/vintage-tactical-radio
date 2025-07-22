#include "AGC.h"
#include <cmath>
#include <algorithm>

AGC::AGC(float attack, float decay)
    : attack_(attack)
    , decay_(decay)
    , targetLevel_(0.5f)
    , maxGain_(100.0f)
    , currentGain_(1.0f)
    , envelope_(0.0f) {
}

void AGC::process(const float* input, float* output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        // Update envelope
        float absSample = fabsf(input[i]);
        float alpha = (absSample > envelope_) ? attack_ : decay_;
        envelope_ = alpha * absSample + (1.0f - alpha) * envelope_;
        
        // Calculate desired gain
        float desiredGain = 1.0f;
        if (envelope_ > 0.001f) {
            desiredGain = targetLevel_ / envelope_;
            desiredGain = std::min(desiredGain, maxGain_);
            desiredGain = std::max(desiredGain, 1.0f / maxGain_);
        }
        
        // Smooth gain changes
        float gainAlpha = 0.01f;
        currentGain_ = gainAlpha * desiredGain + (1.0f - gainAlpha) * currentGain_;
        
        // Apply gain
        output[i] = input[i] * currentGain_;
        
        // Prevent clipping
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
    }
}

void AGC::setParameters(float attack, float decay) {
    attack_ = std::max(0.0001f, std::min(1.0f, attack));
    decay_ = std::max(0.0001f, std::min(1.0f, decay));
}
