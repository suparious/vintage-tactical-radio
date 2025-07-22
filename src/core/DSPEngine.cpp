#include "DSPEngine.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

DSPEngine::DSPEngine(uint32_t sampleRate)
    : sampleRate_(sampleRate)
    , mode_(FM_WIDE)
    , bandwidth_(200000) // 200 kHz for FM
    , running_(false)
    , iqBuffer_(sampleRate * 2) // 2 seconds of buffer
    , fftSize_(2048)
    , agcEnabled_(true)
    , squelchLevel_(-20.0f)
    , noiseReductionEnabled_(false)
    , notchEnabled_(false)
    , notchFreq_(0.0f)
    , notchQ_(10.0f)
    , signalStrength_(-100.0f)
    , squelched_(true)
    , audioDecimation_(sampleRate / 48000) // Decimate to 48kHz audio
    , audioSampleRate_(48000)
    , decimationCounter_(0) {
    
    // Allocate buffers
    iqWorkBuffer_.resize(16384);
    audioBuffer_.resize(16384);
    spectrumBuffer_.resize(fftSize_);
    
    // Initialize FFT
    fftIn_ = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize_);
    fftOut_ = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize_);
    fftPlan_ = fftwf_plan_dft_1d(fftSize_, fftIn_, fftOut_, FFTW_FORWARD, FFTW_MEASURE);
    
    // Initialize DSP components
    amDemod_ = std::make_unique<AMDemodulator>(sampleRate);
    fmDemod_ = std::make_unique<FMDemodulator>(sampleRate, bandwidth_);
    agc_ = std::make_unique<AGC>(0.01f, 0.1f);
    squelch_ = std::make_unique<Squelch>(squelchLevel_);
    noiseReduction_ = std::make_unique<NoiseReduction>(sampleRate);
}

DSPEngine::~DSPEngine() {
    stop();
    
    fftwf_destroy_plan(fftPlan_);
    fftwf_free(fftIn_);
    fftwf_free(fftOut_);
}

void DSPEngine::setSampleRate(uint32_t rate) {
    stop();
    
    sampleRate_ = rate;
    audioDecimation_ = rate / audioSampleRate_;
    
    // Reinitialize components with new sample rate
    amDemod_ = std::make_unique<AMDemodulator>(rate);
    fmDemod_ = std::make_unique<FMDemodulator>(rate, bandwidth_);
    noiseReduction_ = std::make_unique<NoiseReduction>(rate);
    
#ifdef HAS_SPDLOG
    spdlog::info("DSP engine sample rate set to {} Hz", rate);
#endif
}

void DSPEngine::setMode(Mode mode) {
    mode_ = mode;
    
    // Set appropriate bandwidth for mode
    switch (mode) {
        case AM:
            bandwidth_ = 10000; // 10 kHz
            break;
        case FM_NARROW:
            bandwidth_ = 25000; // 25 kHz
            break;
        case FM_WIDE:
            bandwidth_ = 200000; // 200 kHz
            break;
        case USB:
        case LSB:
            bandwidth_ = 2800; // 2.8 kHz
            break;
        case CW:
            bandwidth_ = 200; // 200 Hz
            break;
    }
    
    if (fmDemod_) {
        fmDemod_->setBandwidth(bandwidth_);
    }
    
#ifdef HAS_SPDLOG
    spdlog::info("DSP mode set to {} with bandwidth {} Hz", static_cast<int>(mode), bandwidth_);
#endif
}

void DSPEngine::setBandwidth(uint32_t bandwidth) {
    bandwidth_ = bandwidth;
    if (fmDemod_) {
        fmDemod_->setBandwidth(bandwidth);
    }
}

void DSPEngine::setAGC(bool enable, float attack, float decay) {
    agcEnabled_ = enable;
    if (agc_) {
        agc_->setParameters(attack, decay);
    }
}

void DSPEngine::setSquelch(float level) {
    squelchLevel_ = level;
    if (squelch_) {
        squelch_->setThreshold(level);
    }
}

void DSPEngine::setNoiseReduction(bool enable, float level) {
    noiseReductionEnabled_ = enable;
    if (noiseReduction_) {
        noiseReduction_->setLevel(level);
    }
}

void DSPEngine::setNotchFilter(bool enable, float frequency, float q) {
    notchEnabled_ = enable;
    notchFreq_ = frequency;
    notchQ_ = q;
    // TODO: Implement notch filter
}

void DSPEngine::processIQ(const uint8_t* data, size_t length) {
    // Convert 8-bit IQ to complex float
    std::vector<std::complex<float>> iqData(length / 2);
    convertIQData(data, length, iqData.data());
    
    // Write to ring buffer
    if (!iqBuffer_.write(iqData.data(), iqData.size())) {
#ifdef HAS_SPDLOG
        spdlog::warn("IQ buffer overflow");
#endif
    }
}

void DSPEngine::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    processingThread_ = std::thread(&DSPEngine::processingWorker, this);
    
#ifdef HAS_SPDLOG
    spdlog::info("DSP engine started");
#endif
}

void DSPEngine::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
    
#ifdef HAS_SPDLOG
    spdlog::info("DSP engine stopped");
#endif
}

void DSPEngine::processingWorker() {
    const size_t blockSize = 4096;
    
    while (running_) {
        // Check if we have enough data
        if (iqBuffer_.getReadAvailable() < blockSize) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Read IQ data
        iqBuffer_.read(iqWorkBuffer_.data(), blockSize);
        
        // Calculate signal strength
        calculateSignalStrength(iqWorkBuffer_.data(), blockSize);
        
        // Process spectrum
        processSpectrum(iqWorkBuffer_.data(), blockSize);
        
        // Demodulate
        demodulate(iqWorkBuffer_.data(), blockSize, audioBuffer_.data());
        
        // Apply AGC
        if (agcEnabled_ && agc_) {
            agc_->process(audioBuffer_.data(), audioBuffer_.data(), blockSize);
        }
        
        // Apply squelch
        if (squelch_) {
            squelched_ = squelch_->process(audioBuffer_.data(), blockSize, signalStrength_);
        }
        
        // Apply noise reduction
        if (noiseReductionEnabled_ && noiseReduction_) {
            noiseReduction_->process(audioBuffer_.data(), audioBuffer_.data(), blockSize);
        }
        
        // Decimate audio and send to callback
        if (audioCallback_ && !squelched_) {
            size_t decimatedSamples = 0;
            std::vector<float> decimatedAudio(blockSize / audioDecimation_);
            
            for (size_t i = 0; i < blockSize; i++) {
                if (decimationCounter_ == 0) {
                    decimatedAudio[decimatedSamples++] = audioBuffer_[i];
                }
                decimationCounter_ = (decimationCounter_ + 1) % audioDecimation_;
            }
            
            audioCallback_(decimatedAudio.data(), decimatedSamples);
        }
        
        // Send signal strength
        if (signalCallback_) {
            signalCallback_(signalStrength_.load());
        }
    }
}

void DSPEngine::convertIQData(const uint8_t* data, size_t length, std::complex<float>* output) {
    // Convert 8-bit unsigned to float [-1, 1]
    for (size_t i = 0; i < length / 2; i++) {
        float i_sample = (data[i * 2] - 127.5f) / 127.5f;
        float q_sample = (data[i * 2 + 1] - 127.5f) / 127.5f;
        output[i] = std::complex<float>(i_sample, q_sample);
    }
}

void DSPEngine::processSpectrum(const std::complex<float>* data, size_t length) {
    if (!spectrumCallback_) {
        return;
    }
    
    // Copy data to FFT input (with window function)
    size_t fftInput = std::min(length, fftSize_);
    for (size_t i = 0; i < fftInput; i++) {
        // Hann window
        float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fftInput - 1)));
        fftIn_[i][0] = data[i].real() * window;
        fftIn_[i][1] = data[i].imag() * window;
    }
    
    // Zero pad if necessary
    for (size_t i = fftInput; i < fftSize_; i++) {
        fftIn_[i][0] = 0.0f;
        fftIn_[i][1] = 0.0f;
    }
    
    // Execute FFT
    fftwf_execute(fftPlan_);
    
    // Calculate magnitude spectrum in dB
    for (size_t i = 0; i < fftSize_; i++) {
        float real = fftOut_[i][0];
        float imag = fftOut_[i][1];
        float magnitude = sqrtf(real * real + imag * imag);
        spectrumBuffer_[i] = 20.0f * log10f(magnitude + 1e-10f);
    }
    
    // Rearrange spectrum (negative frequencies first)
    std::rotate(spectrumBuffer_.begin(), 
                spectrumBuffer_.begin() + fftSize_ / 2, 
                spectrumBuffer_.end());
    
    spectrumCallback_(spectrumBuffer_.data(), fftSize_);
}

void DSPEngine::calculateSignalStrength(const std::complex<float>* data, size_t length) {
    float sum = 0.0f;
    for (size_t i = 0; i < length; i++) {
        float mag = std::abs(data[i]);
        sum += mag * mag;
    }
    
    float rms = sqrtf(sum / length);
    float dB = 20.0f * log10f(rms + 1e-10f);
    
    // Apply some smoothing
    float alpha = 0.1f;
    signalStrength_ = alpha * dB + (1.0f - alpha) * signalStrength_.load();
}

void DSPEngine::demodulate(const std::complex<float>* input, size_t length, float* output) {
    switch (mode_) {
        case AM:
            if (amDemod_) {
                amDemod_->demodulate(input, output, length);
            }
            break;
            
        case FM_NARROW:
        case FM_WIDE:
            if (fmDemod_) {
                fmDemod_->demodulate(input, output, length);
            }
            break;
            
        case USB:
        case LSB:
        case CW:
            // TODO: Implement SSB/CW demodulation
            std::fill(output, output + length, 0.0f);
            break;
    }
}
