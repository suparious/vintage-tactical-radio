# Changelog

All notable changes to Vintage Tactical Radio will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Development build system improvements
- Additional packaging scripts for Debian and Flatpak
- AppStream metadata for better Linux desktop integration

### Changed
- Optimized DSP engine for better performance
- Improved UI responsiveness

### Fixed
- Memory leak in spectrum display
- Audio crackling on certain sample rates

## [1.0.0] - 2025-01-22

### Added
- Initial release of Vintage Tactical Radio
- RTL-SDR v3 support with automatic device detection
- Wide frequency coverage from 500 kHz to 1.7 GHz
- Multiple demodulation modes: AM, FM (narrow/wide), SSB, CW
- 7-band parametric equalizer with two modes:
  - Modern: 50Hz, 125Hz, 315Hz, 750Hz, 2.2kHz, 6kHz, 16kHz
  - Nostalgic: 60Hz, 150Hz, 400Hz, 1kHz, 2.4kHz, 6kHz, 15kHz
- Equalizer presets: Flat, Full Bass and Treble, Bass Boosted, Treble Cut, Radio, Voice, Music, DX
- Real-time spectrum display with phosphor-style waterfall
- Authentic military-style interface inspired by AN/PRC-77 and similar radios
- Five themes: Military Olive, Navy Grey, Night Mode, Desert Tan, Black Ops
- Memory channels: 1000 channels in 10 banks
- Professional audio support:
  - ALSA direct hardware access
  - PulseAudio desktop integration
  - JACK professional audio routing
  - USB Audio external DACs
  - HDMI audio output
- Configurable audio settings:
  - Sample rates: 44.1kHz, 48kHz, 192kHz
  - Bit depths: 16-bit, 24-bit
- AGC (Automatic Gain Control) with adjustable attack/decay
- Squelch control from 0 to -100 dB
- RF gain control from 0 to 49 dB
- Fine tuning control ±100 kHz
- Noise reduction and noise blanker
- Direct sampling mode for HF reception
- Bias-T support for active antennas
- Comprehensive keyboard shortcuts
- Settings persistence between sessions

### Technical Details
- Built with Qt6 for native performance
- Multi-threaded DSP engine
- Lock-free ring buffers for audio streaming
- SIMD optimizations for signal processing
- Hardware-accelerated OpenGL spectrum display
- Modular architecture for easy expansion

### Known Issues
- SSB/CW modes are placeholder implementations (coming in 1.1.0)
- Noise reduction requires manual noise profile learning
- Some themes may have minor visual glitches on certain desktop environments

[Unreleased]: https://github.com/vintage-tactical-radio/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/vintage-tactical-radio/releases/tag/v1.0.0
