#include "DSPEngine.h"
#include "../decoders/CTCSSDecoder.h"
#include "../decoders/RDSDecoder.h"
#include "../decoders/ADSBDecoder.h"
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
    , mode_(FM_WIDE)  // Default to FM_WIDE
    , bandwidth_(200000) // 200 kHz for FM
    , running_(false)
    , iqBuffer_(sampleRate * 2) // 2 seconds of buffer
    , fftSize_(2048)
    , agcEnabled_(false)  // Disable AGC by default for FM
    , squelchLevel_(-20.0f)
    , noiseReductionEnabled_(false)
    , notchEnabled_(false)
    , notchFreq_(0.0f)
    , notchQ_(10.0f)
    , signalStrength_(-100.0f)
    , squelched_(false)  // Start with squelch open
    , dynamicBandwidth_(false)  // Disable by default for testing
    , audioDecimation_(sampleRate / 48000) // Decimate to 48kHz audio
    , audioSampleRate_(48000)
    , decimationCounter_(0)
    , ctcssEnabled_(false)
    , rdsEnabled_(false)
    , adsbEnabled_(false)
    , currentFrequency_(0) {
    
    // DC removal filter state
    dcI_ = 0.0f;
    dcQ_ = 0.0f;
    dcAlpha_ = 0.995f;  // DC removal filter coefficient
    
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
    ssbDemod_ = std::make_unique<SSBDemodulator>(sampleRate, SSBDemodulator::USB);
    agc_ = std::make_unique<AGC>(0.01f, 0.1f);
    squelch_ = std::make_unique<Squelch>(squelchLevel_);
    noiseReduction_ = std::make_unique<NoiseReduction>(sampleRate);
    
    // Initialize digital decoders
    ctcssDecoder_ = std::make_unique<CTCSSDecoder>();
    ctcssDecoder_->setSampleRate(audioSampleRate_);
    
    rdsDecoder_ = std::make_unique<RDSDecoder>();
    rdsDecoder_->setSampleRate(sampleRate);
    
    adsbDecoder_ = std::make_unique<ADSBDecoder>();
}

void DSPEngine::enableCTCSS(bool enable) {
    ctcssEnabled_ = enable;
    if (ctcssDecoder_) {
        if (enable) {
            ctcssDecoder_->start();
        } else {
            ctcssDecoder_->stop();
        }
    }
}

void DSPEngine::enableRDS(bool enable) {
    rdsEnabled_ = enable;
    if (rdsDecoder_) {
        if (enable) {
            rdsDecoder_->start();
        } else {
            rdsDecoder_->stop();
        }
    }
}

void DSPEngine::enableADSB(bool enable) {
    adsbEnabled_ = enable;
    if (adsbDecoder_) {
        if (enable) {
            adsbDecoder_->start();
        } else {
            adsbDecoder_->stop();
        }
    }
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
    ssbDemod_ = std::make_unique<SSBDemodulator>(rate, SSBDemodulator::USB);
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
            bandwidth_ = 220000; // 220 kHz for stereo FM broadcast
            break;
        case USB:
        case LSB:
            bandwidth_ = 2800; // 2.8 kHz
            if (ssbDemod_) {
                ssbDemod_->setMode(mode == USB ? SSBDemodulator::USB : SSBDemodulator::LSB);
                ssbDemod_->setBandwidth(bandwidth_);
            }
            break;
        case CW:
            bandwidth_ = 200; // 200 Hz
            if (ssbDemod_) {
                ssbDemod_->setMode(SSBDemodulator::CW);
                ssbDemod_->setCWBandwidth(bandwidth_);
            }
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
        
        // Adjust bandwidth dynamically for FM if enabled
        if (dynamicBandwidth_ && mode_ == FM_WIDE) {
            float strength = signalStrength_.load();
            // Strong signal (> -40 dB): use full 220 kHz for stereo
            // Weak signal (< -60 dB): reduce to 200 kHz
            // Very weak (< -70 dB): reduce to 180 kHz for better SNR
            uint32_t newBandwidth;
            if (strength > -40.0f) {
                newBandwidth = 220000; // Full stereo bandwidth
            } else if (strength > -60.0f) {
                newBandwidth = 200000; // Standard bandwidth
            } else if (strength > -70.0f) {
                newBandwidth = 180000; // Reduced bandwidth
            } else {
                newBandwidth = 150000; // Mono-compatible bandwidth
            }
            
            if (newBandwidth != bandwidth_) {
                bandwidth_ = newBandwidth;
                if (fmDemod_) {
                    fmDemod_->setBandwidth(bandwidth_);
                }
            }
        }
        
        // Process spectrum
        processSpectrum(iqWorkBuffer_.data(), blockSize);
    
    // Send raw IQ to ADS-B decoder if enabled and frequency is correct
    if (adsbEnabled_ && adsbDecoder_ && currentFrequency_ >= 1089e6 && currentFrequency_ <= 1091e6) {
        // Convert complex float back to uint8_t for ADS-B decoder
        std::vector<uint8_t> rawData(blockSize * 2);
        for (size_t i = 0; i < blockSize; i++) {
            rawData[i * 2] = static_cast<uint8_t>((iqWorkBuffer_[i].real() * 127.5f) + 127.5f);
            rawData[i * 2 + 1] = static_cast<uint8_t>((iqWorkBuffer_[i].imag() * 127.5f) + 127.5f);
        }
        adsbDecoder_->processRaw(rawData.data(), rawData.size());
    }
        
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
        if (audioCallback_) {
            if (!squelched_) {
                // Use a better decimation filter
                size_t decimatedSamples = 0;
                std::vector<float> decimatedAudio(blockSize / audioDecimation_ + 1);
                
                // Simple low-pass filter coefficients for anti-aliasing
                // This is a 5-tap FIR filter designed for cutoff at Nyquist/2 of output rate
                static const float lpf[] = {0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f};
                static std::vector<float> filterState(5, 0.0f);
                
                for (size_t i = 0; i < blockSize; i++) {
                    // Shift filter state and add new sample
                    for (int j = 4; j > 0; j--) {
                        filterState[j] = filterState[j-1];
                    }
                    filterState[0] = audioBuffer_[i];
                    
                    // Apply filter
                    float filtered = 0.0f;
                    for (int j = 0; j < 5; j++) {
                        filtered += lpf[j] * filterState[j];
                    }
                    
                    // Decimate
                    if (decimationCounter_ == 0) {
                        decimatedAudio[decimatedSamples++] = filtered;
                    }
                    decimationCounter_ = (decimationCounter_ + 1) % audioDecimation_;
                }
                
                audioCallback_(decimatedAudio.data(), decimatedSamples);
                
                // Send audio to CTCSS decoder if enabled
                if (ctcssEnabled_ && ctcssDecoder_) {
                    ctcssDecoder_->processAudio(decimatedAudio.data(), decimatedSamples);
                }
                
                // Send audio to RDS decoder if enabled (FM mode only)
                if (rdsEnabled_ && rdsDecoder_ && (mode_ == FM_WIDE || mode_ == FM_NARROW)) {
                    // RDS needs the full rate audio before decimation
                    rdsDecoder_->processAudio(audioBuffer_.data(), blockSize);
                }
            } else {
                // Send silence when squelched
                std::vector<float> silence(blockSize / audioDecimation_, 0.0f);
                audioCallback_(silence.data(), silence.size());
            }
        }
        
        // Send signal strength
        if (signalCallback_) {
            signalCallback_(signalStrength_.load());
        }
    }
}

void DSPEngine::convertIQData(const uint8_t* data, size_t length, std::complex<float>* output) {
    // Convert 8-bit unsigned to float [-1, 1] with DC removal
    for (size_t i = 0; i < length / 2; i++) {
        // Convert to float centered at 0
        float i_sample = (data[i * 2] - 127.5f) / 127.5f;
        float q_sample = (data[i * 2 + 1] - 127.5f) / 127.5f;
        
        // Update DC estimates
        dcI_ = dcAlpha_ * dcI_ + (1.0f - dcAlpha_) * i_sample;
        dcQ_ = dcAlpha_ * dcQ_ + (1.0f - dcAlpha_) * q_sample;
        
        // Remove DC
        i_sample -= dcI_;
        q_sample -= dcQ_;
        
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
    
    // Calculate magnitude spectrum in dB with proper normalization
    const float normFactor = 1.0f / (fftSize_ * fftSize_);  // FFT normalization
    const float refLevel = 1.0f;  // Reference level for dB calculation
    
    for (size_t i = 0; i < fftSize_; i++) {
        float real = fftOut_[i][0];
        float imag = fftOut_[i][1];
        float magnitude = sqrtf((real * real + imag * imag) * normFactor);
        
        // Convert to dB with proper scaling
        // Add offset to keep values in reasonable range (-120 to 0 dB)
        float dB = 20.0f * log10f(magnitude / refLevel + 1e-10f);
        
        // Clamp to reasonable range
        spectrumBuffer_[i] = std::max(-120.0f, std::min(0.0f, dB));
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
    // Convert to dB relative to full scale (1.0)
    // Since our IQ data is normalized to [-1, 1], full scale is 1.0
    float dB = 20.0f * log10f(rms + 1e-10f);
    
    // Typical values will be around -30 to -60 dB for normal signals
    // Clamp to reasonable range
    dB = std::max(-100.0f, std::min(0.0f, dB));
    
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
            if (ssbDemod_) {
                ssbDemod_->demodulate(input, output, length);
            }
            break;
    }
}
