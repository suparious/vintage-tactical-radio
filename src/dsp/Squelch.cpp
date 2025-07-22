#include "Squelch.h"
#include <cmath>
#include <algorithm>

Squelch::Squelch(float threshold)
    : threshold_(threshold)
    , squelched_(true)
    , attackTime_(5.0f)    // 5ms attack
    , decayTime_(100.0f)   // 100ms decay
    , fadeLevel_(0.0f) {
}

bool Squelch::process(float* audio, size_t length, float signalLevel) {
    bool shouldSquelch = signalLevel < threshold_;
    
    // State change detection
    if (shouldSquelch != squelched_) {
        squelched_ = shouldSquelch;
        
        // Apply smooth transition
        applyFade(audio, length, !squelched_);
    } else if (squelched_) {
        // Mute audio if squelched
        std::fill(audio, audio + length, 0.0f);
    }
    
    return squelched_;
}

void Squelch::setAttackTime(float ms) {
    attackTime_ = std::max(0.1f, std::min(100.0f, ms));
}

void Squelch::setDecayTime(float ms) {
    decayTime_ = std::max(1.0f, std::min(1000.0f, ms));
}

void Squelch::applyFade(float* audio, size_t length, bool opening) {
    float fadeTime = opening ? attackTime_ : decayTime_;
    float fadeRate = 1000.0f / (fadeTime * 48000.0f); // Assuming 48kHz audio
    
    for (size_t i = 0; i < length; i++) {
        if (opening) {
            fadeLevel_ = std::min(1.0f, fadeLevel_ + fadeRate);
        } else {
            fadeLevel_ = std::max(0.0f, fadeLevel_ - fadeRate);
        }
        
        audio[i] *= fadeLevel_;
    }
}
