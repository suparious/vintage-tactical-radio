#ifndef AGC_H
#define AGC_H

#include <cstddef>  // for size_t

class AGC {
public:
    AGC(float attack = 0.01f, float decay = 0.1f);
    ~AGC() = default;
    
    void process(const float* input, float* output, size_t length);
    
    void setParameters(float attack, float decay);
    void setTargetLevel(float level) { targetLevel_ = level; }
    void setMaxGain(float gain) { maxGain_ = gain; }
    
    float getCurrentGain() const { return currentGain_; }
    
private:
    float attack_;      // Attack time constant
    float decay_;       // Decay time constant
    float targetLevel_; // Target output level
    float maxGain_;     // Maximum gain allowed
    float currentGain_; // Current gain value
    float envelope_;    // Signal envelope
};

#endif // AGC_H
