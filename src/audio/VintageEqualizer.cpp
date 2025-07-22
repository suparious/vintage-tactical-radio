#include "VintageEqualizer.h"
#include <cmath>
#include <algorithm>

// Static frequency definitions
const std::vector<float> VintageEqualizer::modernFrequencies_ = {
    50.0f, 125.0f, 315.0f, 750.0f, 2200.0f, 6000.0f, 16000.0f
};

const std::vector<float> VintageEqualizer::nostalgicFrequencies_ = {
    60.0f, 150.0f, 400.0f, 1000.0f, 2400.0f, 6000.0f, 15000.0f
};

// Static preset definitions
std::map<std::string, VintageEqualizer::Preset> VintageEqualizer::presets_ = {
    {"Flat", {"Flat", {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}}},
    {"Full Bass and Treble", {"Full Bass and Treble", {6.0f, 4.0f, 0.0f, 0.0f, 0.0f, 4.0f, 6.0f}}},
    {"Bass Boosted", {"Bass Boosted", {9.0f, 6.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f}}},
    {"Treble Cut", {"Treble Cut", {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -6.0f, -9.0f}}},
    {"Radio", {"Radio", {-3.0f, 0.0f, 3.0f, 6.0f, 3.0f, 0.0f, -3.0f}}},
    {"Voice", {"Voice", {-6.0f, -3.0f, 0.0f, 3.0f, 6.0f, 3.0f, 0.0f}}},
    {"Music", {"Music", {3.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 3.0f}}},
    {"DX", {"DX", {0.0f, 3.0f, 6.0f, 3.0f, 0.0f, -3.0f, -6.0f}}}
};

VintageEqualizer::VintageEqualizer(uint32_t sampleRate, Mode mode)
    : sampleRate_(sampleRate)
    , mode_(mode)
    , preampGain_(0.0f)
    , maxGain_(12.0f) {
    
    // Initialize bands
    bands_.resize(7);
    filters_.resize(7);
    
    // Set frequencies based on mode
    const auto& frequencies = (mode == MODERN) ? modernFrequencies_ : nostalgicFrequencies_;
    for (size_t i = 0; i < 7; i++) {
        bands_[i].frequency = frequencies[i];
        bands_[i].gain = 0.0f;
        bands_[i].q = 0.7f;
        updateFilter(i);
    }
}

void VintageEqualizer::process(const float* input, float* output, size_t length) {
    // Apply preamp gain
    float preampLinear = powf(10.0f, preampGain_ / 20.0f);
    
    for (size_t i = 0; i < length; i++) {
        float sample = input[i] * preampLinear;
        
        // Process through each band filter
        for (size_t band = 0; band < 7; band++) {
            if (bands_[band].gain != 0.0f) {
                sample = filters_[band].process(sample);
            }
        }
        
        // Soft clipping to prevent harsh distortion
        if (fabsf(sample) > 0.95f) {
            sample = (sample > 0.0f ? 1.0f : -1.0f) * 
                    (1.0f - expf(-3.0f * fabsf(sample)));
        }
        
        output[i] = sample;
    }
}

void VintageEqualizer::setMode(Mode mode) {
    if (mode_ == mode) return;
    
    mode_ = mode;
    
    // Update frequencies
    const auto& frequencies = (mode == MODERN) ? modernFrequencies_ : nostalgicFrequencies_;
    for (size_t i = 0; i < 7; i++) {
        bands_[i].frequency = frequencies[i];
        updateFilter(i);
    }
}

void VintageEqualizer::setBandGain(int band, float gain) {
    if (band < 0 || band >= 7) return;
    
    bands_[band].gain = std::max(-maxGain_, std::min(maxGain_, gain));
    updateFilter(band);
}

float VintageEqualizer::getBandGain(int band) const {
    if (band < 0 || band >= 7) return 0.0f;
    return bands_[band].gain;
}

void VintageEqualizer::setBandQ(int band, float q) {
    if (band < 0 || band >= 7) return;
    
    bands_[band].q = std::max(0.1f, std::min(10.0f, q));
    updateFilter(band);
}

float VintageEqualizer::getBandQ(int band) const {
    if (band < 0 || band >= 7) return 0.7f;
    return bands_[band].q;
}

void VintageEqualizer::loadPreset(const std::string& name) {
    auto it = presets_.find(name);
    if (it != presets_.end()) {
        const auto& preset = it->second;
        for (size_t i = 0; i < 7 && i < preset.gains.size(); i++) {
            setBandGain(i, preset.gains[i]);
        }
    }
}

std::vector<std::string> VintageEqualizer::getPresetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : presets_) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<VintageEqualizer::Preset> VintageEqualizer::getDefaultPresets() {
    std::vector<Preset> result;
    for (const auto& pair : presets_) {
        result.push_back(pair.second);
    }
    return result;
}

void VintageEqualizer::reset() {
    preampGain_ = 0.0f;
    
    for (size_t i = 0; i < 7; i++) {
        bands_[i].gain = 0.0f;
        bands_[i].q = 0.7f;
        updateFilter(i);
        filters_[i].reset();
    }
}

void VintageEqualizer::updateFilter(int band) {
    if (band < 0 || band >= 7) return;
    
    float b0, b1, b2, a1, a2;
    calculatePeakingCoefficients(bands_[band].frequency, 
                                bands_[band].gain,
                                bands_[band].q,
                                b0, b1, b2, a1, a2);
    
    filters_[band].b0 = b0;
    filters_[band].b1 = b1;
    filters_[band].b2 = b2;
    filters_[band].a1 = a1;
    filters_[band].a2 = a2;
}

void VintageEqualizer::calculatePeakingCoefficients(float frequency, float gain, float q,
                                                   float& b0, float& b1, float& b2,
                                                   float& a1, float& a2) {
    // Peaking EQ filter design
    float A = powf(10.0f, gain / 40.0f);
    float omega = 2.0f * M_PI * frequency / sampleRate_;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * q);
    
    float a0 = 1.0f + alpha / A;
    
    // Normalize coefficients
    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cos_omega) / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (-2.0f * cos_omega) / a0;
    a2 = (1.0f - alpha / A) / a0;
}
