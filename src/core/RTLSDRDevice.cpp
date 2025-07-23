#include "RTLSDRDevice.h"
#include <sstream>
#include <cstring>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

RTLSDRDevice::RTLSDRDevice() 
    : device_(nullptr)
    , streaming_(false)
    , centerFreq_(96900000) // Default to 96.9 MHz FM
    , sampleRate_(2400000)  // Default 2.4 MHz
    , currentGain_(250)     // 25.0 dB
    , ppmCorrection_(0)
    , manualGain_(true)
    , biasTEnabled_(false)
    , directSamplingMode_(DIRECT_SAMPLING_OFF) {
}

RTLSDRDevice::~RTLSDRDevice() {
    if (streaming_) {
        stopStreaming();
    }
    if (device_) {
        close();
    }
}

int RTLSDRDevice::getDeviceCount() const {
    return rtlsdr_get_device_count();
}

std::string RTLSDRDevice::getDeviceName(int index) const {
    const char* name = rtlsdr_get_device_name(index);
    return name ? name : "";
}

std::vector<std::string> RTLSDRDevice::getDeviceList() const {
    std::vector<std::string> devices;
    int count = getDeviceCount();
    
    for (int i = 0; i < count; i++) {
        char manufacturer[256], product[256], serial[256];
        if (rtlsdr_get_device_usb_strings(i, manufacturer, product, serial) == 0) {
            std::stringstream ss;
            ss << i << ": " << manufacturer << " " << product << " SN: " << serial;
            devices.push_back(ss.str());
        } else {
            devices.push_back(std::to_string(i) + ": " + getDeviceName(i));
        }
    }
    
    return devices;
}

bool RTLSDRDevice::open(int deviceIndex) {
    if (device_) {
        close();
    }
    
    int result = rtlsdr_open(&device_, deviceIndex);
    if (!checkError(result, "opening device")) {
        return false;
    }
    
#ifdef HAS_SPDLOG
    spdlog::info("Opened RTL-SDR device: {}", getDeviceName(deviceIndex));
#endif
    
    // Reset the device
    rtlsdr_reset_buffer(device_);
    
    // Apply default settings
    setSampleRate(sampleRate_);
    setCenterFrequency(centerFreq_);
    setGainMode(manualGain_);
    if (manualGain_) {
        setGain(currentGain_);
    }
    setFrequencyCorrection(ppmCorrection_);
    
    return true;
}

void RTLSDRDevice::close() {
    if (streaming_) {
        stopStreaming();
    }
    
    if (device_) {
        rtlsdr_close(device_);
        device_ = nullptr;
#ifdef HAS_SPDLOG
        spdlog::info("Closed RTL-SDR device");
#endif
    }
}

bool RTLSDRDevice::setCenterFrequency(uint32_t freq) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    // Automatically enable direct sampling for HF frequencies
    DirectSamplingMode newMode = DIRECT_SAMPLING_OFF;
    if (freq < 24000000) { // Below 24 MHz, use direct sampling
        newMode = DIRECT_SAMPLING_Q; // Q-branch is preferred for AM broadcast
    }
    
    // Apply direct sampling mode if it changed
    if (newMode != directSamplingMode_) {
        if (!setDirectSampling(newMode)) {
            return false;
        }
    }
    
    int result = rtlsdr_set_center_freq(device_, freq);
    if (!checkError(result, "setting center frequency")) {
        return false;
    }
    
    centerFreq_ = rtlsdr_get_center_freq(device_);
#ifdef HAS_SPDLOG
    spdlog::debug("Center frequency set to {} Hz", centerFreq_);
#endif
    
    return true;
}

bool RTLSDRDevice::setFrequencyCorrection(int ppm) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    int result = rtlsdr_set_freq_correction(device_, ppm);
    if (!checkError(result, "setting frequency correction")) {
        return false;
    }
    
    ppmCorrection_ = rtlsdr_get_freq_correction(device_);
    return true;
}

bool RTLSDRDevice::setSampleRate(uint32_t rate) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    int result = rtlsdr_set_sample_rate(device_, rate);
    if (!checkError(result, "setting sample rate")) {
        return false;
    }
    
    sampleRate_ = rtlsdr_get_sample_rate(device_);
#ifdef HAS_SPDLOG
    spdlog::debug("Sample rate set to {} Hz", sampleRate_);
#endif
    
    return true;
}

bool RTLSDRDevice::setDirectSampling(int mode) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    int result = rtlsdr_set_direct_sampling(device_, mode);
    if (!checkError(result, "setting direct sampling mode")) {
        return false;
    }
    
    directSamplingMode_ = static_cast<DirectSamplingMode>(mode);
    
#ifdef HAS_SPDLOG
    const char* modeStr = (mode == 0) ? "disabled" : (mode == 1) ? "I-branch" : "Q-branch";
    spdlog::debug("Direct sampling mode: {}", modeStr);
#endif
    
    return true;
}

bool RTLSDRDevice::setGainMode(bool manual) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    int result = rtlsdr_set_tuner_gain_mode(device_, manual ? 1 : 0);
    if (!checkError(result, "setting gain mode")) {
        return false;
    }
    
    manualGain_ = manual;
    return true;
}

bool RTLSDRDevice::setGain(int gain) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    if (!manualGain_) {
        setError("Manual gain mode not enabled");
        return false;
    }
    
    int result = rtlsdr_set_tuner_gain(device_, gain);
    if (!checkError(result, "setting gain")) {
        return false;
    }
    
    currentGain_ = rtlsdr_get_tuner_gain(device_);
#ifdef HAS_SPDLOG
    spdlog::debug("Gain set to {} dB", currentGain_ / 10.0);
#endif
    
    return true;
}

std::vector<int> RTLSDRDevice::getGains() const {
    std::vector<int> gains;
    if (!device_) {
        return gains;
    }
    
    int count = rtlsdr_get_tuner_gains(device_, nullptr);
    if (count > 0) {
        gains.resize(count);
        rtlsdr_get_tuner_gains(device_, gains.data());
    }
    
    return gains;
}

bool RTLSDRDevice::setBiasT(bool enable) {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    int result = rtlsdr_set_bias_tee(device_, enable ? 1 : 0);
    if (!checkError(result, "setting bias-T")) {
        return false;
    }
    
    biasTEnabled_ = enable;
#ifdef HAS_SPDLOG
    spdlog::info("Bias-T {}", enable ? "enabled" : "disabled");
#endif
    
    return true;
}

bool RTLSDRDevice::startStreaming() {
    if (!device_) {
        setError("Device not open");
        return false;
    }
    
    if (streaming_) {
        return true;
    }
    
    streaming_ = true;
    streamingThread_ = std::thread(&RTLSDRDevice::streamingWorker, this);
    
#ifdef HAS_SPDLOG
    spdlog::info("Started streaming");
#endif
    
    return true;
}

void RTLSDRDevice::stopStreaming() {
    if (!streaming_) {
        return;
    }
    
    streaming_ = false;
    
    if (device_) {
        rtlsdr_cancel_async(device_);
    }
    
    if (streamingThread_.joinable()) {
        streamingThread_.join();
    }
    
#ifdef HAS_SPDLOG
    spdlog::info("Stopped streaming");
#endif
}

void RTLSDRDevice::streamingWorker() {
    const int bufferSize = 16384 * 2; // 16K IQ samples
    rtlsdr_read_async(device_, rtlsdrCallback, this, 0, bufferSize);
}

void RTLSDRDevice::rtlsdrCallback(unsigned char* buf, uint32_t len, void* ctx) {
    RTLSDRDevice* device = static_cast<RTLSDRDevice*>(ctx);
    
    if (device->streaming_ && device->dataCallback_) {
        device->dataCallback_(buf, len);
    }
}

RTLSDRDevice::OptimalSettings RTLSDRDevice::getOptimalSettings(uint32_t frequency) {
    OptimalSettings settings;
    
    // Based on our research document
    if (frequency >= 530000 && frequency <= 1700000) {
        // AM Broadcast
        settings.sampleRate = 2048000;
        settings.gain = 450; // 45.0 dB
        settings.directSampling = 2; // Q-branch
        settings.description = "AM Broadcast";
    } else if (frequency >= 88000000 && frequency <= 108000000) {
        // FM Broadcast
        settings.sampleRate = 2400000;
        settings.gain = 250; // 25.0 dB
        settings.directSampling = 0;
        settings.description = "FM Broadcast";
    } else if (frequency >= 156000000 && frequency <= 162000000) {
        // VHF Marine
        settings.sampleRate = 2048000;
        settings.gain = 300; // 30.0 dB
        settings.directSampling = 0;
        settings.description = "VHF Marine";
    } else if (frequency >= 108000000 && frequency <= 137000000) {
        // Aviation
        settings.sampleRate = 2048000;
        settings.gain = 200; // 20.0 dB
        settings.directSampling = 0;
        settings.description = "Aviation Band";
    } else {
        // Generic VHF/UHF
        settings.sampleRate = 2048000;
        settings.gain = 300; // 30.0 dB
        settings.directSampling = frequency < 24000000 ? 2 : 0;
        settings.description = "Generic";
    }
    
    return settings;
}

void RTLSDRDevice::setError(const std::string& error) {
    lastError_ = error;
#ifdef HAS_SPDLOG
    spdlog::error("RTL-SDR Error: {}", error);
#endif
}

bool RTLSDRDevice::checkError(int result, const std::string& operation) {
    if (result < 0) {
        std::stringstream ss;
        ss << "Error " << operation << ": " << result;
        setError(ss.str());
        return false;
    }
    return true;
}
