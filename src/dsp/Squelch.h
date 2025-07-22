#ifndef SQUELCH_H
#define SQUELCH_H

#include <cstdint>

class Squelch {
public:
    explicit Squelch(float threshold = -20.0f);
    ~Squelch() = default;
    
    // Process audio and return true if squelched (muted)
    bool process(float* audio, size_t length, float signalLevel);
    
    void setThreshold(float threshold) { threshold_ = threshold; }
    float getThreshold() const { return threshold_; }
    
    void setAttackTime(float ms);
    void setDecayTime(float ms);
    
    bool isOpen() const { return !squelched_; }
    
private:
    float threshold_;     // Threshold in dB
    bool squelched_;      // Current squelch state
    float attackTime_;    // Time to open squelch (ms)
    float decayTime_;     // Time to close squelch (ms)
    float fadeLevel_;     // Current fade level (0-1)
    
    // Smooth transitions
    void applyFade(float* audio, size_t length, bool opening);
};

#endif // SQUELCH_H
