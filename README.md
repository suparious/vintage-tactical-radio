# Vintage Tactical Radio

A native Linux application that emulates a vintage military/tactical radio interface while leveraging modern RTL-SDR v3 hardware for software-defined radio reception.

![Vintage Tactical Radio](assets/images/vintage-logo.png)

## Features

- **Authentic Military Radio Interface**: Inspired by Cold War-era tactical radios like the AN/PRC-77
- **Wide Frequency Coverage**: 500 kHz to 1.7 GHz with RTL-SDR v3
- **Multiple Demodulation Modes**: AM, FM (narrow/wide), SSB, CW
- **7-Band Parametric Equalizer**: Modern and Nostalgic modes with presets
- **Real-time Spectrum Display**: Phosphor-style waterfall and spectrum analyzer
- **Memory Channels**: 1000 channels in 10 banks with quick access
- **Multiple Themes**: Military Olive, Navy Grey, Night Mode, Desert Tan, Black Ops
- **Professional Audio**: Support for ALSA, PulseAudio, JACK, USB Audio, and HDMI

## Requirements

### Hardware
- RTL-SDR v3 or compatible dongle (RTL2832U + R820T2/R860)
- Appropriate antenna for desired frequency range
- USB 2.0 or 3.0 port

### Software Dependencies
- Qt6 (Core, Widgets, Multimedia, OpenGL)
- librtlsdr
- FFTW3
- ALSA/PulseAudio
- CMake 3.16+
- C++17 compiler

## Building from Source

### Debian/Ubuntu

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake git
sudo apt install qt6-base-dev qt6-multimedia-dev libqt6opengl6-dev
sudo apt install librtlsdr-dev libfftw3-dev
sudo apt install libasound2-dev libpulse-dev

# Optional: Install spdlog for enhanced logging
sudo apt install libspdlog-dev

# Clone and build
git clone https://github.com/yourusername/vintage-tactical-radio.git
cd vintage-tactical-radio
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Fedora

```bash
# Install dependencies
sudo dnf install cmake gcc-c++ git
sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel
sudo dnf install rtl-sdr-devel fftw-devel
sudo dnf install alsa-lib-devel pulseaudio-libs-devel

# Build (same as above)
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S base-devel cmake git
sudo pacman -S qt6-base qt6-multimedia
sudo pacman -S rtl-sdr fftw
sudo pacman -S alsa-lib libpulse

# Build (same as above)
```

## Installation

### From Package

```bash
# Debian/Ubuntu
sudo dpkg -i vintage-tactical-radio_1.0.0_amd64.deb

# Or using apt
sudo apt install ./vintage-tactical-radio_1.0.0_amd64.deb
```

### AppImage

```bash
chmod +x vintage-tactical-radio-1.0.0-x86_64.AppImage
./vintage-tactical-radio-1.0.0-x86_64.AppImage
```

### Flatpak

```bash
flatpak install vintage-tactical-radio.flatpak
flatpak run io.github.vintage-tactical-radio
```

## Usage

### First Run

1. Connect your RTL-SDR device
2. Launch Vintage Tactical Radio
3. Select your RTL-SDR device from the dropdown
4. Click START to begin receiving

### Basic Operation

- **Frequency Tuning**: 
  - Drag the frequency dial up/down
  - Use mouse wheel for fine tuning
  - Click on spectrum display to tune
  
- **Mode Selection**:
  - AM: Amplitude modulation (MW/SW broadcast)
  - FM: Frequency modulation (VHF/UHF)
  - SSB: Single sideband (amateur radio)
  - CW: Continuous wave (Morse code)

- **Controls**:
  - **Volume**: Audio output level
  - **Squelch**: Mute threshold (-100 to 0 dB)
  - **RF Gain**: RTL-SDR gain (0-49 dB)
  - **Fine Tune**: ±100 kHz adjustment

### Optimal Antenna Setup

Based on frequency range (from research documents):

- **AM Broadcast (530-1700 kHz)**: Long wire with 9:1 unun
- **FM Broadcast (88-108 MHz)**: Dipole or discone antenna
- **VHF Marine (156-162 MHz)**: Marine band whip or yagi
- **General Coverage**: Discone antenna for wideband

### Equalizer Presets

- **Flat**: 0 dB all bands (default)
- **Full Bass and Treble**: Enhanced low and high frequencies
- **Bass Boosted**: Emphasized low frequencies
- **Radio**: Communications audio optimization
- **Voice**: Midrange emphasis for clarity
- **DX**: Weak signal enhancement

## Configuration

Settings are stored in `~/.config/vintage-tactical-radio/settings.json`

### Default Hotkeys

- `Space`: Start/Stop radio
- `Up/Down`: Frequency step up/down
- `Page Up/Down`: Large frequency steps
- `F1-F5`: Quick theme switching
- `M`: Mute/Unmute
- `S`: Toggle squelch

## Troubleshooting

### No RTL-SDR Device Found

1. Check USB connection
2. Verify device with `lsusb` (should show Realtek RTL2838)
3. Add udev rules:
```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0bda", ATTRS{idProduct}=="2838", MODE="0666"' | \
sudo tee /etc/udev/rules.d/20-rtlsdr.rules
sudo udevadm control --reload-rules
```

### Audio Issues

- Ensure audio device is properly selected in settings
- Check PulseAudio with `pavucontrol`
- Try different sample rates (44.1/48/192 kHz)

### Poor Reception

- Verify antenna connection and type
- Adjust RF gain (lower for strong signals)
- Enable AGC for varying signal strengths
- Check for USB noise (use extension cable)

## Development

### Project Structure

```
vintage-tactical-radio/
├── src/
│   ├── core/        # RTL-SDR interface, DSP engine
│   ├── audio/       # Audio output, equalizer
│   ├── ui/          # Qt widgets, themes
│   ├── dsp/         # Demodulators, filters
│   └── config/      # Settings, memory channels
├── assets/          # Images, sounds, themes
├── packaging/       # Distribution files
└── CMakeLists.txt
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the GPL-3.0 License - see LICENSE file for details.

## Acknowledgments

- RTL-SDR community for librtlsdr
- Qt Project for the excellent framework
- Inspired by classic military radios: AN/PRC-77, R-390A, Collins KWM-2A

## Support

- Report issues: https://github.com/yourusername/vintage-tactical-radio/issues
- Documentation: https://github.com/yourusername/vintage-tactical-radio/wiki
- Community: https://discord.gg/vintage-radio

---

**Note**: This software is for educational and amateur radio use. Please respect local regulations regarding radio reception and ensure you have appropriate licenses for transmitting on amateur radio frequencies.
