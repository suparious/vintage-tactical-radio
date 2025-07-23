#ifndef DSPENGINE_H
#define DSPENGINE_H

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <complex>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint32_t
#include <vector>
#include <fftw3.h>

#include "RingBuffer.h"
#include "../dsp/AMDemodulator.h"
#include "../dsp/FMDemodulator.h"
#include "../dsp/SSBDemodulator.h"
#include "../dsp/AGC.h"
#include "../dsp/Squelch.h"
#include "../dsp/NoiseReduction.h"

class DSPEngine {
public:
    enum Mode {
        AM,
        FM_NARROW,
        FM_WIDE,
        USB,
        LSB,
        CW
    };
    
    DSPEngine(uint32_t sampleRate = 2400000);
    ~DSPEngine();
    
    // Configuration
    void setSampleRate(uint32_t rate);
    uint32_t getSampleRate() const { return sampleRate_; }
    
    void setMode(Mode mode);
    Mode getMode() const { return mode_; }
    
    void setBandwidth(uint32_t bandwidth);
    uint32_t getBandwidth() const { return bandwidth_; }
    
    // Dynamic bandwidth for FM based on signal strength
    void setDynamicBandwidth(bool enable) { dynamicBandwidth_ = enable; }
    bool getDynamicBandwidth() const { return dynamicBandwidth_; }
    
    // DSP Controls
    void setAGC(bool enable, float attack = 0.01f, float decay = 0.1f);
    void setSquelch(float level); // -100 to 0 dB
    void setNoiseReduction(bool enable, float level = 0.5f);
    void setNotchFilter(bool enable, float frequency, float q = 10.0f);
    
    // Audio callback
    using AudioCallback = std::function<void(const float*, size_t)>;
    void setAudioCallback(AudioCallback callback) { audioCallback_ = callback; }
    
    // Signal strength callback
    using SignalCallback = std::function<void(float)>; // S-meter value in dB
    void setSignalCallback(SignalCallback callback) { signalCallback_ = callback; }
    
    // Spectrum callback
    using SpectrumCallback = std::function<void(const float*, size_t)>;
    void setSpectrumCallback(SpectrumCallback callback) { spectrumCallback_ = callback; }
    
    // Input IQ data
    void processIQ(const uint8_t* data, size_t length);
    
    // Control
    void start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Signal measurements
    float getSignalStrength() const { return signalStrength_; }
    float getSquelchLevel() const { return squelchLevel_; }
    bool isSquelched() const { return squelched_; }
    
private:
    // Sample rate and mode
    uint32_t sampleRate_;
    Mode mode_;
    uint32_t bandwidth_;
    
    // Processing thread
    std::thread processingThread_;
    std::atomic<bool> running_;
    
    // Buffers
    IQBuffer iqBuffer_;
    std::vector<std::complex<float>> iqWorkBuffer_;
    std::vector<float> audioBuffer_;
    std::vector<float> spectrumBuffer_;
    
    // FFT for spectrum
    fftwf_plan fftPlan_;
    fftwf_complex* fftIn_;
    fftwf_complex* fftOut_;
    size_t fftSize_;
    
    // DSP components
    std::unique_ptr<AMDemodulator> amDemod_;
    std::unique_ptr<FMDemodulator> fmDemod_;
    std::unique_ptr<SSBDemodulator> ssbDemod_;
    std::unique_ptr<AGC> agc_;
    std::unique_ptr<Squelch> squelch_;
    std::unique_ptr<NoiseReduction> noiseReduction_;
    
    // DSP settings
    bool agcEnabled_;
    float squelchLevel_;
    bool noiseReductionEnabled_;
    bool notchEnabled_;
    float notchFreq_;
    float notchQ_;
    
    // Signal measurements
    std::atomic<float> signalStrength_;
    std::atomic<bool> squelched_;
    bool dynamicBandwidth_;
    
    // Callbacks
    AudioCallback audioCallback_;
    SignalCallback signalCallback_;
    SpectrumCallback spectrumCallback_;
    
    // Processing methods
    void processingWorker();
    void convertIQData(const uint8_t* data, size_t length, std::complex<float>* output);
    void processSpectrum(const std::complex<float>* data, size_t length);
    void calculateSignalStrength(const std::complex<float>* data, size_t length);
    void demodulate(const std::complex<float>* input, size_t length, float* output);
    
    // Audio decimation
    uint32_t audioDecimation_;
    uint32_t audioSampleRate_;
    size_t decimationCounter_;
    
    // DC removal filter
    float dcI_;
    float dcQ_;
    float dcAlpha_;
};

#endif // DSPENGINE_H
