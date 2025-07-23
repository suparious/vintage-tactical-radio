#ifndef RTLSDRDEVICE_H
#define RTLSDRDEVICE_H

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint32_t, uint8_t
#include <rtl-sdr.h>

class RTLSDRDevice {
public:
    RTLSDRDevice();
    ~RTLSDRDevice();
    
    // Device enumeration
    int getDeviceCount() const;
    std::string getDeviceName(int index) const;
    std::vector<std::string> getDeviceList() const;
    
    // Device control
    bool open(int deviceIndex = 0);
    void close();
    bool isOpen() const { return device_ != nullptr; }
    
    // Frequency control
    bool setCenterFrequency(uint32_t freq);
    uint32_t getCenterFrequency() const { return centerFreq_; }
    bool setFrequencyCorrection(int ppm);
    int getFrequencyCorrection() const { return ppmCorrection_; }
    
    // Sampling control
    bool setSampleRate(uint32_t rate);
    uint32_t getSampleRate() const { return sampleRate_; }
    bool setDirectSampling(int mode); // 0=disabled, 1=I, 2=Q
    
    // Gain control
    bool setGainMode(bool manual);
    bool setGain(int gain); // in tenths of dB (e.g., 250 = 25.0 dB)
    int getGain() const { return currentGain_; }
    std::vector<int> getGains() const;
    
    // Bias-T control
    bool setBiasT(bool enable);
    bool getBiasT() const { return biasTEnabled_; }
    
    // Data streaming
    using DataCallback = std::function<void(const uint8_t*, size_t)>;
    void setDataCallback(DataCallback callback) { dataCallback_ = callback; }
    bool startStreaming();
    void stopStreaming();
    bool isStreaming() const { return streaming_; }
    
    // Error handling
    std::string getLastError() const { return lastError_; }
    
    // Optimal settings from research
    struct OptimalSettings {
        uint32_t sampleRate;
        int gain;
        int directSampling;
        std::string description;
    };
    
    static OptimalSettings getOptimalSettings(uint32_t frequency);
    
    // Direct sampling modes
    enum DirectSamplingMode {
        DIRECT_SAMPLING_OFF = 0,
        DIRECT_SAMPLING_I = 1,
        DIRECT_SAMPLING_Q = 2
    };
    
    DirectSamplingMode getDirectSamplingMode() const { return directSamplingMode_; }
    
private:
    rtlsdr_dev_t* device_;
    std::thread streamingThread_;
    std::atomic<bool> streaming_;
    DataCallback dataCallback_;
    
    // Current settings
    uint32_t centerFreq_;
    uint32_t sampleRate_;
    int currentGain_;
    int ppmCorrection_;
    bool manualGain_;
    bool biasTEnabled_;
    DirectSamplingMode directSamplingMode_;
    
    std::string lastError_;
    
    // Streaming worker
    void streamingWorker();
    static void rtlsdrCallback(unsigned char* buf, uint32_t len, void* ctx);
    
    // Error handling
    void setError(const std::string& error);
    bool checkError(int result, const std::string& operation);
};

#endif // RTLSDRDEVICE_H
