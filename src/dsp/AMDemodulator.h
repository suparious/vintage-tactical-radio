#ifndef AMDEMODULATOR_H
#define AMDEMODULATOR_H

#include <complex>
#include <vector>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint32_t

class AMDemodulator {
public:
    explicit AMDemodulator(uint32_t sampleRate);
    ~AMDemodulator() = default;
    
    void demodulate(const std::complex<float>* input, float* output, size_t length);
    
    // Carrier tracking for synchronous detection
    void setCarrierTracking(bool enable) { carrierTracking_ = enable; }
    bool getCarrierTracking() const { return carrierTracking_; }
    
private:
    uint32_t sampleRate_;
    bool carrierTracking_;
    
    // DC removal filter
    float dcAlpha_;
    float lastDC_;
    
    // Carrier tracking PLL (for synchronous AM)
    float pllPhase_;
    float pllFreq_;
    float pllAlpha_;
    float pllBeta_;
};

#endif // AMDEMODULATOR_H
