# Product Requirements Document: Vintage Tactical Radio
## RTL-SDR Based Software Defined Radio Application

### Version 1.0
### Date: January 2025

---

## Executive Summary

This PRD defines the requirements for a Linux-native desktop application that emulates a vintage military/tactical/spy radio interface while leveraging modern RTL-SDR v3 hardware. The application will provide sophisticated radio reception capabilities across multiple frequency bands with a focus on nostalgic aesthetics and professional-grade functionality.

---

## 1. Product Overview

### 1.1 Vision Statement
Create a desktop SDR application that combines the aesthetic appeal of Cold War-era military radio equipment with modern software-defined radio capabilities, providing both functional radio reception and an immersive vintage experience.

### 1.2 Target Users
- Radio enthusiasts and hobbyists
- Amateur radio operators
- Scanner/monitoring enthusiasts
- Military radio collectors and historians
- Emergency preparedness community

### 1.3 Core Value Propositions
- Authentic vintage military/tactical radio interface
- Professional-grade signal processing with RTL-SDR v3
- Comprehensive frequency coverage (530 kHz - 1.7 GHz)
- Native Linux performance without web dependencies
- Extensive customization through parametric EQ and DSP

---

## 2. Technical Requirements

### 2.1 Hardware Compatibility

#### Primary Hardware
- **RTL-SDR v3** or compatible dongles with:
  - RTL2832U chipset with 1PPM TCXO
  - R820T2/R860 tuner chip
  - Frequency range: 500 kHz - 1.766 GHz
  - Direct sampling capability for HF
  - USB 2.0 interface
  - Device identification: "Realtek Semiconductor Corp. RTL2838 DVB-T"

#### Supported Antennas (Based on Research)
- **AM Broadcast (530-1700 kHz)**: Long wire with 9:1 unun
- **FM Broadcast (88-108 MHz)**: Dipole or discone antenna
- **VHF Marine (156-162 MHz)**: Marine band whip or yagi
- **General Coverage**: Discone antenna for wideband reception

### 2.2 Software Architecture

#### Core Components
1. **RTL-SDR Interface Layer**
   - librtlsdr integration
   - Direct sampling mode switching
   - Automatic gain control
   - Bias-T control for active antennas

2. **DSP Engine**
   - Real-time signal processing
   - Multi-threaded demodulation
   - Software AGC implementation
   - Noise reduction algorithms

3. **Audio Processing**
   - 7-band parametric equalizer
   - Real-time audio effects
   - Multiple output device support

4. **User Interface**
   - Native GUI framework (Qt or GTK)
   - Hardware-accelerated rendering
   - Vintage military aesthetic

### 2.3 Supported Frequency Bands and Modes

Based on our comprehensive research, the application will support:

#### Initial Release Bands
1. **AM Broadcast (530-1700 kHz)**
   - Mode: AM with carrier AGC
   - Channel spacing: 10 kHz
   - Bandwidth: 10 kHz
   - Direct sampling: Q-branch

2. **FM Broadcast (88-108 MHz)**
   - Mode: WFM (Wide FM)
   - Channel spacing: 200 kHz (100 kHz in some regions)
   - Bandwidth: 180-200 kHz
   - Features: Stereo decoding, RDS support

3. **VHF Marine (156-162 MHz)**
   - Mode: NFM (Narrow FM)
   - Channel spacing: 25 kHz
   - Bandwidth: 16 kHz
   - International marine channels

#### Future Expansion Bands
- **Amateur Radio HF (1.8-30 MHz)**: USB/LSB/CW modes
- **CB Radio (26.965-27.405 MHz)**: AM/SSB modes
- **Aviation Band (108-137 MHz)**: AM mode
- **Amateur VHF/UHF**: Various modes

---

## 3. User Interface Requirements

### 3.1 Design Philosophy
The UI should evoke the feeling of operating a Vietnam War-era military radio or Cold War spy equipment. Think AN/PRC-77, R-390A, or similar tactical radios.

### 3.2 Visual Design Elements

#### Required UI Components
1. **Main Frequency Display**
   - Large analog-style frequency dial with vernier tuning
   - Nixie tube or vacuum fluorescent display (VFD) style digital readout
   - Mechanical counter-style frequency display option

2. **S-Meter/Signal Strength**
   - Analog VU meter with needle
   - Backlit scale with military-style markings
   - Peak hold indicator

3. **Control Panel**
   - Chunky rotary knobs with resistance feel simulation
   - Toggle switches for mode selection
   - Push buttons with LED indicators
   - Military-style labels and nomenclature

4. **Spectrum Display**
   - Phosphor-style waterfall display
   - Vintage oscilloscope aesthetic
   - Adjustable persistence and intensity

### 3.3 Color Schemes
- **Default**: Military olive drab with amber/orange displays
- **Night Mode**: Red-filtered displays for tactical operations
- **Alternative Themes**: 
  - Navy grey with green phosphor
  - Black with blue-white displays
  - Desert tan with LCD-style readouts

### 3.4 Audio/Visual Feedback
- Mechanical knob rotation sounds
- Switch click sounds
- Static/noise between stations
- Authentic squelch tail
- Optional CRT phosphor burn-in effect

---

## 4. Feature Specifications

### 4.1 Core Radio Functions

#### Tuning Controls
- **Main Tuning**: Large rotary encoder simulation
- **Fine Tuning**: Vernier dial for precise adjustment
- **Direct Entry**: Keypad for frequency input
- **Memory Channels**: 100 programmable memories with alphanumeric tags
- **Band Selection**: Quick-access band switches

#### Signal Processing
- **AGC (Automatic Gain Control)**
  - Fast/Slow/Off modes
  - Adjustable attack/decay times
- **Squelch**
  - Range: 0 to -100 dB
  - Default: -20 dB
  - Carrier-activated and noise-activated modes
- **Noise Blanker**
  - Adjustable threshold
  - Pulse width detection
- **Notch Filter**
  - Manual and automatic modes
  - Adjustable Q factor

### 4.2 Audio Processing

#### 7-Band Parametric Equalizer
**Modern Mode Frequencies**:
- 50 Hz (Sub-bass)
- 125 Hz (Bass)
- 315 Hz (Lower-mid)
- 750 Hz (Mid)
- 2.2 kHz (Upper-mid)
- 6 kHz (Presence)
- 16 kHz (Brilliance)

**Nostalgic Mode Frequencies**:
- 60 Hz (Bass)
- 150 Hz (Lower-bass)
- 400 Hz (Mid-bass)
- 1 kHz (Midrange)
- 2.4 kHz (Upper-mid)
- 6 kHz (Treble)
- 15 kHz (Air)

**Specifications**:
- Gain range: ±12 dB (default), ±30 dB (maximum)
- Q factor: 0.5 to 10 (adjustable per band)
- Global preamp: ±20 dB

#### Equalizer Presets
1. **Flat** (0 dB all bands)
2. **Full Bass and Treble** (+6dB @ 60Hz, +4dB @ 150Hz, 0dB mids, +4dB @ 6kHz, +6dB @ 15kHz)
3. **Bass Boosted** (+9dB @ 60Hz, +6dB @ 150Hz, +3dB @ 400Hz)
4. **Treble Cut** (0dB bass/mids, -6dB @ 6kHz, -9dB @ 15kHz)
5. **Radio** (Communications audio emphasis)
6. **Voice** (Midrange emphasis for clarity)
7. **Music** (Gentle V-curve)
8. **DX** (Weak signal enhancement)

### 4.3 Audio Output Configuration

#### Supported Audio Systems
- **ALSA**: Direct hardware access
- **PulseAudio**: Desktop integration
- **JACK**: Professional audio routing
- **USB Audio**: External DACs
- **HDMI**: TV/monitor audio

#### Audio Settings
- Sample rates: 44.1 kHz, 48 kHz, 96 kHz, 192 kHz
- Bit depths: 16-bit (s16le), 24-bit (s24le), 32-bit float
- Buffer sizes: 64 to 4096 samples
- Latency compensation

### 4.4 RTL-SDR Specific Settings

Based on our research findings:

#### Gain Control
- **Range**: 0 to 49.6 dB
- **Automatic**: RTL AGC on/off
- **Manual**: 0.9 dB steps
- **Presets by band**:
  - AM Broadcast: 40-48 dB
  - FM Broadcast: 20-30 dB
  - VHF Marine: 25-35 dB

#### Sampling Configuration
- **Default bandwidth**: 2.4 MHz (stability preferred over 3.2 MHz max)
- **Decimation**: 1x to 32x
- **Offset tuning**: For eliminating DC spike
- **Bias-T**: Software control for active antennas

#### Direct Sampling Modes
- **Disabled**: Normal operation (>24 MHz)
- **I-Branch**: HF reception option 1
- **Q-Branch**: HF reception option 2 (preferred for AM broadcast)

---

## 5. Performance Requirements

### 5.1 System Requirements
- **OS**: Linux (Debian 11+, Ubuntu 20.04+, Fedora 35+)
- **CPU**: Dual-core 2.0 GHz minimum (Quad-core recommended)
- **RAM**: 2 GB minimum (4 GB recommended)
- **USB**: USB 2.0 port (USB 3.0 recommended)
- **Graphics**: OpenGL 2.1 support for spectrum display

### 5.2 Performance Targets
- **Audio latency**: <50ms
- **Frequency switching**: <100ms
- **UI responsiveness**: 60 FPS
- **CPU usage**: <30% on recommended hardware

---

## 6. Additional Features

### 6.1 Recording Capabilities
- **Audio recording**: WAV, FLAC, MP3
- **IQ recording**: For post-processing
- **Scheduled recording**: Timer function
- **Time-shift buffer**: 30-minute rolling buffer

### 6.2 Scanning Functions
- **Channel scan**: Automatic station detection
- **Frequency scan**: Activity monitoring
- **Memory scan**: Cycle through saved channels
- **Priority channels**: Interrupt scanning

### 6.3 Digital Modes (Future)
- **RDS decoding**: FM station information
- **CTCSS/DCS**: Tone squelch decoding
- **SAME**: Weather alert decoding
- **ADS-B**: Aircraft tracking (1090 MHz)

### 6.4 Network Features
- **Remote control**: Web interface for headless operation
- **Audio streaming**: Network audio output
- **Frequency database**: Online station lookups
- **Cluster spotting**: DX cluster integration

---

## 7. Packaging and Distribution

### 7.1 Package Formats
1. **Debian Package (.deb)**
   - Native integration with apt
   - Automatic dependency resolution
   - System-wide installation

2. **AppImage**
   - Portable, no installation required
   - Works across distributions
   - Includes all dependencies

3. **Flatpak**
   - Sandboxed environment
   - Easy updates via Flathub
   - Cross-distribution compatibility

### 7.2 Dependencies
- librtlsdr0
- libusb-1.0
- FFTW3
- Qt5/Qt6 or GTK3/GTK4
- PulseAudio/ALSA libraries
- libsamplerate

---

## 8. Configuration and Settings

### 8.1 User Preferences Storage
- **Location**: ~/.config/vintage-tactical-radio/
- **Format**: JSON or INI files
- **Backup/restore**: Import/export functionality

### 8.2 Default Settings
```json
{
  "audio": {
    "device": "default",
    "sampleRate": 48000,
    "bitDepth": 16,
    "bufferSize": 1024
  },
  "radio": {
    "mode": "FM",
    "frequency": 96900000,
    "bandwidth": 200000,
    "gain": 30,
    "squelch": -20
  },
  "equalizer": {
    "mode": "modern",
    "preset": "flat",
    "bands": [0, 0, 0, 0, 0, 0, 0]
  },
  "ui": {
    "theme": "military-olive",
    "sounds": true,
    "animations": true
  }
}
```

### 8.3 Reset Functionality
- **Soft reset**: Return to default settings
- **Hard reset**: Clear all configurations and memories
- **Selective reset**: Reset specific subsystems

---

## 9. Testing Requirements

### 9.1 Functional Testing
- All frequency bands receive correctly
- Mode switching works without audio glitches
- Memory channels save/recall properly
- Scanning functions operate correctly

### 9.2 Performance Testing
- CPU usage under various conditions
- Memory leak detection
- USB disconnect/reconnect handling
- Audio underrun prevention

### 9.3 Compatibility Testing
- Multiple RTL-SDR dongles
- Various Linux distributions
- Different audio subsystems
- USB 2.0 and 3.0 ports

---

## 10. Future Roadmap

### Phase 1 (MVP)
- Basic AM/FM reception
- Core UI implementation
- Essential audio processing

### Phase 2
- VHF Marine support
- Advanced DSP features
- Recording capabilities

### Phase 3
- Amateur radio bands
- Digital mode decoding
- Network features

### Phase 4
- Plugin architecture
- Community modifications
- Mobile companion app

---

## 11. Success Metrics

- User can tune and listen to AM/FM broadcasts
- Audio quality comparable to commercial receivers
- UI provides authentic vintage experience
- Application stability >99.9% uptime
- Community adoption and contributions

---

## Appendix A: Technical References

1. RTL-SDR Direct Sampling Implementation
2. SDR++ Optimal Configuration Matrix (from research)
3. Canadian Frequency Allocation Table
4. Antenna Impedance Matching Guidelines
5. Military Radio UI Design Patterns

---

## Appendix B: Competitive Analysis

- **SDR++**: Modern UI, excellent performance, lacks vintage aesthetic
- **GQRX**: Good functionality, generic interface
- **CubicSDR**: Clean design, missing tactical feel
- **Our Differentiator**: Authentic military radio experience with professional SDR capabilities

---

This PRD incorporates the extensive research on frequency allocations, antenna specifications, and optimal SDR configurations while maintaining focus on creating an authentic vintage military radio experience. The document is structured to provide clear, actionable requirements for an AI code assistant to implement the application.