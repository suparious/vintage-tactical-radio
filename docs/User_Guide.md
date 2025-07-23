# Vintage Tactical Radio - User Guide

## Overview
Vintage Tactical Radio is a feature-rich SDR application that combines the aesthetic appeal of Cold War-era military radio equipment with modern software-defined radio capabilities. This guide covers all implemented features and how to use them.

## Main Features

### 1. Antenna Recommendations
- **Location**: Status bar (permanent widget)
- **Function**: Automatically recommends the best antenna for your current frequency
- **Usage**: 
  - The antenna type and gain are displayed in the status bar
  - Click the "?" button for detailed recommendations including impedance matching requirements
  - Updates automatically when you change frequency

### 2. Recording System
- **Location**: RECORDING panel
- **Features**:
  - **Audio Recording**: Records demodulated audio to WAV files (16 or 24-bit)
  - **IQ Recording**: Records raw IQ data for post-processing
  - **Time-shift Buffer**: 30-minute rolling buffer for capturing past transmissions
  - **Scheduled Recording**: Set future recording times (via API, not exposed in UI yet)
  
- **Usage**:
  1. Select format (WAV, FLAC*, MP3*, IQ) from dropdown
  2. Click REC button to start recording
  3. Recording time displays in HH:MM:SS format
  4. Files saved to ~/VintageRadio/Recordings/
  5. Enable "Time Shift" checkbox for continuous buffering
  6. Click "Save Buffer" to save the last N seconds from buffer

*Note: FLAC and MP3 formats are placeholders for future implementation

### 3. Scanner System
- **Location**: SCANNER panel
- **Scan Modes**:
  - **Frequency Scan**: Scans a frequency range with configurable step size
  - **Channel Scan**: Scans predefined channels
  - **Memory Scan**: Scans through saved memory channels
  - **Band Scan**: Scans within the current band limits
  
- **Controls**:
  - **SCAN/STOP**: Start or stop scanning
  - **Mode**: Select scan type
  - **Step**: Frequency step size (5 kHz to 200 kHz)
  - **Speed**: Scan rate (1-50 channels/second)
  - **Threshold**: Signal detection threshold (-100 to -20 dB)
  - **SKIP**: Skip current active channel
  
- **Features**:
  - Automatic pause on signal detection
  - Configurable dwell time on active channels
  - Progress bar shows scan position
  - Priority channel support (via API)

### 4. Memory Channels
- **Location**: MEMORY CHANNELS panel
- **Capacity**: 10 banks × 10 channels = 100 total memories
- **Features**:
  - Quick Access dropdown for favorite channels
  - Store current frequency, mode, and settings
  - Recall complete radio state
  - Clear individual memories
  
- **Usage**:
  1. Select bank (0-9) and channel (0-9)
  2. Click STORE to save current settings
  3. Click RECALL to load saved settings
  4. Click CLEAR to delete a memory

### 5. Equalizer
- **Location**: 7-BAND EQUALIZER panel
- **Modes**: 
  - Modern (50Hz, 125Hz, 315Hz, 750Hz, 2.2kHz, 6kHz, 16kHz)
  - Nostalgic (60Hz, 150Hz, 400Hz, 1kHz, 2.4kHz, 6kHz, 15kHz)
- **Presets**: Flat, Full Bass and Treble, Bass Boosted, Treble Cut, Radio, Voice, Music, DX
- **Gain Range**: Selectable ±12dB, ±18dB, ±24dB, or ±30dB

### 6. Core Radio Functions
- **Frequency Control**:
  - Large analog-style dial with digital display
  - Direct frequency entry with MHz/kHz selection
  - Fine tuning knob (±100 kHz)
  - Band quick-select (MW, SW, FM, VHF, UHF)
  
- **Demodulation Modes**: AM, FM-Narrow, FM-Wide, USB, LSB, CW
- **Audio Controls**: Volume, Squelch, RF Gain
- **Signal Meters**: S-Meter and spectrum/waterfall display

### 7. Settings (File → Settings)
- **Audio Configuration**:
  - Device selection (ALSA, PulseAudio, USB, HDMI)
  - Sample rate (44.1, 48, 96, 192 kHz)
  - Bit depth (16 or 24-bit)
  
- **RTL-SDR Configuration**:
  - Sample rate (2.048, 2.4, 2.56, 3.2 MHz)
  - PPM correction (-100 to +100)
  - Bias-T power for active antennas
  - Dynamic bandwidth adjustment

## Keyboard Shortcuts
- **Ctrl+,** : Open Settings
- **F1-F5** : Change themes (Military Olive, Navy Grey, Night Mode, Desert Tan, Black Ops)
- **Ctrl+Q** : Exit application

## File Locations
- **Configuration**: ~/.config/vintage-tactical-radio/
- **Memory Channels**: ~/.config/vintage-tactical-radio/memory_channels.json
- **Recordings**: ~/VintageRadio/Recordings/
- **Settings**: ~/.config/vintage-tactical-radio/settings.json

## Tips for Best Performance

### Antenna Selection
- Use the antenna recommendation system to choose the right antenna
- For AM broadcast: Long wire with 9:1 unun transformer
- For FM broadcast: Simple dipole or discone antenna
- For VHF/UHF: Discone for scanning, Yagi for directional work

### Recording
- Use WAV format for best quality and compatibility
- Enable time-shift buffer to never miss interesting transmissions
- IQ recording allows re-demodulation with different settings later

### Scanning
- Adjust threshold based on local noise floor
- Use slower scan speeds for weak signal detection
- Memory scan is great for monitoring known active frequencies
- Add frequently active channels to memory for quick access

### Optimal Gain Settings
The application automatically suggests optimal gain when changing frequencies:
- AM Broadcast: 40-48 dB
- FM Broadcast: 20-30 dB
- VHF Marine: 25-35 dB
- UHF: 30-40 dB

## Troubleshooting

### No RTL-SDR Device Found
- Ensure device is plugged in
- Check USB permissions (may need to add user to 'plugdev' group)
- Try different USB ports
- Install udev rules for RTL-SDR

### Audio Issues
- Check Settings → Audio Device selection
- Verify sample rate compatibility
- Try different audio systems (ALSA vs PulseAudio)

### Poor Reception
- Check antenna recommendations for your frequency
- Adjust RF gain (too high can cause overload)
- Enable dynamic bandwidth for optimal filtering
- Check for local interference sources

### Recording Problems
- Ensure sufficient disk space
- Check write permissions on recording directory
- For time-shift, ensure adequate RAM (uses ~350MB for 30-minute buffer)

## Future Features (Planned)
- FLAC and MP3 recording support
- RDS decoding for FM stations
- CTCSS/DCS tone decoding
- Network streaming and remote control
- ADS-B aircraft tracking
- Additional digital mode decoders

## Credits
Vintage Tactical Radio is an open-source project inspired by military and tactical radio equipment. It leverages the RTL-SDR hardware and various open-source libraries to provide a professional-grade SDR experience with a nostalgic interface.

For bug reports and feature requests, please visit the project repository.
