#ifndef SSBDEMODULATOR_H
#define SSBDEMODULATOR_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <complex>
#include <memory>

class SSBDemodulator {
public:
    enum Mode {
        USB,  // Upper sideband
        LSB,  // Lower sideband
        CW    // Continuous wave (Morse code)
    };
    
    SSBDemodulator(uint32_t sampleRate, Mode mode = USB);
    ~SSBDemodulator();
    
    void demodulate(const std::complex<float>* input, float* output, size_t length);
    
    void setMode(Mode mode) { mode_ = mode; }
    Mode getMode() const { return mode_; }
    
    void setBandwidth(uint32_t bandwidth);
    uint32_t getBandwidth() const { return bandwidth_; }
    
    // CW specific settings
    void setCWPitch(float pitch) { cwPitch_ = pitch; }
    float getCWPitch() const { return cwPitch_; }
    
    void setCWBandwidth(float bandwidth) { cwBandwidth_ = bandwidth; }
    float getCWBandwidth() const { return cwBandwidth_; }
    
private:
    uint32_t sampleRate_;
    Mode mode_;
    uint32_t bandwidth_;
    
    // CW parameters
    float cwPitch_;      // CW tone pitch in Hz (typically 600-800 Hz)
    float cwBandwidth_;  // CW filter bandwidth (typically 100-500 Hz)
    float cwPhase_;      // Phase accumulator for CW tone generation
    
    // Hilbert transform for SSB
    class HilbertTransform;
    std::unique_ptr<HilbertTransform> hilbert_;
    
    // Low-pass filter for bandwidth limiting
    std::vector<float> lpfCoeffs_;
    std::vector<std::complex<float>> lpfDelay_;
    
    // AGC for SSB
    float agcLevel_;
    float agcAttack_;
    float agcDecay_;
    
    void generateLPFCoeffs();
    std::complex<float> applyLPF(std::complex<float> sample);
};

// Hilbert transform implementation for 90-degree phase shift
class SSBDemodulator::HilbertTransform {
public:
    HilbertTransform(size_t length = 65);
    
    // Process real signal, return analytic signal (I + jQ)
    std::complex<float> process(float sample);
    
private:
    std::vector<float> coeffs_;
    std::vector<float> delayLine_;
    size_t delayIndex_;
    
    void generateCoefficients(size_t length);
};

#endif // SSBDEMODULATOR_H
