# Technical Implementation Guide
## Vintage Tactical Radio - Developer Reference

### RTL-SDR Integration Code Patterns

```python
# Example: Direct Sampling Mode Configuration for AM Broadcast
def configure_am_broadcast(device):
    """Configure RTL-SDR for AM broadcast reception (530-1700 kHz)"""
    device.set_sample_rate(2048000)  # 2.048 MHz
    device.set_center_freq(1000000)   # 1 MHz center
    device.set_freq_correction(0)     # PPM correction
    device.set_direct_sampling(2)     # Q-branch
    device.set_gain_mode(0)           # Manual gain
    device.set_gain(450)              # 45.0 dB (45.0 * 10)
    return device

# Example: FM Broadcast Configuration
def configure_fm_broadcast(device):
    """Configure RTL-SDR for FM broadcast reception (88-108 MHz)"""
    device.set_sample_rate(2400000)   # 2.4 MHz
    device.set_center_freq(96900000)  # 96.9 MHz
    device.set_direct_sampling(0)     # Disabled
    device.set_gain_mode(0)           # Manual gain
    device.set_gain(250)              # 25.0 dB
    return device
```

### Optimal Settings Matrix (From Research)

| Band | Direct Sampling | Sample Rate | Gain Range | Bandwidth | Mode |
|------|----------------|-------------|------------|-----------|------|
| AM Broadcast | Q-Branch | 2.048 MHz | 40-48 dB | 10 kHz | AM |
| FM Broadcast | Disabled | 2.4 MHz | 20-30 dB | 200 kHz | WFM |
| VHF Marine | Disabled | 2.048 MHz | 25-35 dB | 25 kHz | NFM |
| Aviation | Disabled | 2.048 MHz | 12-25 dB | 8 kHz | AM |

### DSP Pipeline Architecture

```
[RTL-SDR] → [IQ Buffer] → [Decimation] → [Channel Filter] → [Demodulator] → [Audio Filter] → [EQ] → [Output]
                ↓                                                   ↓
          [FFT Display]                                    [S-Meter/AGC]
```

### Key Performance Optimizations

1. **Buffer Management**
   - Use circular buffers for IQ data
   - Implement double buffering for audio output
   - Typical buffer size: 16384 samples

2. **Thread Architecture**
   ```
   Main Thread: UI and user interaction
   RTL Thread: Device reading and IQ buffering
   DSP Thread: Signal processing and demodulation
   Audio Thread: Playback and output management
   Display Thread: Spectrum/waterfall rendering
   ```

3. **SIMD Optimizations**
   - Use VOLK library for vectorized operations
   - SSE/AVX for x86_64 platforms
   - NEON for ARM platforms

### Antenna Impedance Matching Reference

Based on our research:
- **Long Wire (AM)**: Requires 9:1 unun (450Ω → 50Ω)
- **Dipole (FM)**: Direct connection for 75Ω, 4:1 balun for folded
- **Discone**: Direct 50Ω connection
- **Active Antennas**: Enable bias-T (4.5V @ 180mA)

### Frequency Correction Calibration

```python
# Automatic calibration using known reference
def calibrate_ppm(device, reference_freq=162400000):  # NOAA Weather
    """Calibrate frequency offset using known reference"""
    # Implementation would measure actual vs expected frequency
    # and calculate PPM offset
    pass
```

### Audio Processing Pipeline

```python
# 7-Band EQ Implementation Structure
class VintageEqualizer:
    def __init__(self, mode='modern'):
        if mode == 'modern':
            self.frequencies = [50, 125, 315, 750, 2200, 6000, 16000]
        else:  # nostalgic
            self.frequencies = [60, 150, 400, 1000, 2400, 6000, 15000]
        
        self.gains = [0.0] * 7  # dB
        self.q_factors = [0.7] * 7
        self.preamp = 0.0  # dB
```

### UI Framework Considerations

**Qt6 Implementation Benefits:**
- Hardware-accelerated OpenGL for spectrum display
- QDial widgets perfect for vintage knobs
- QPainter for custom meter rendering
- Built-in audio subsystem support

**GTK4 Alternative:**
- Native Wayland support
- CSS theming for vintage aesthetics
- Cairo for custom graphics
- GStreamer integration

### Memory Channel Storage Format

```json
{
  "memories": [
    {
      "index": 1,
      "frequency": 96900000,
      "mode": "WFM",
      "bandwidth": 200000,
      "name": "CJAX Jack FM",
      "gain": 25,
      "squelch": -20,
      "antenna": "FM Dipole"
    }
  ]
}
```

### Error Handling Patterns

```python
class RTLSDRError(Exception):
    """Base exception for RTL-SDR operations"""
    pass

class DeviceNotFoundError(RTLSDRError):
    """Raised when RTL-SDR device cannot be found"""
    pass

class USBError(RTLSDRError):
    """Raised on USB communication errors"""
    pass
```

### Testing Considerations

1. **Mock RTL-SDR for Development**
   - Create IQ file playback mode
   - Simulate various signal conditions
   - Test without hardware

2. **Performance Benchmarks**
   - Processing latency < 10ms
   - FFT update rate > 30 FPS
   - Audio dropout rate < 0.1%

### Linux-Specific Optimizations

1. **Real-time Priority**
   ```bash
   # /etc/security/limits.d/audio.conf
   @audio - rtprio 95
   @audio - memlock unlimited
   ```

2. **USB Buffer Tuning**
   ```bash
   # Increase USB buffer size
   echo 16 > /sys/module/usbcore/parameters/usbfs_memory_mb
   ```

3. **CPU Governor**
   ```bash
   # Set performance mode
   cpupower frequency-set -g performance
   ```

### Packaging Scripts

**AppImage Build:**
```bash
#!/bin/bash
# bundle-appimage.sh
mkdir -p AppDir/usr/bin
cp vintage-tactical-radio AppDir/usr/bin/
cp -r assets AppDir/usr/share/
linuxdeploy --appdir AppDir --plugin qt --output appimage
```

**Flatpak Manifest:**
```yaml
app-id: io.github.vintage-tactical-radio
runtime: org.freedesktop.Platform
runtime-version: '23.08'
sdk: org.freedesktop.Sdk
command: vintage-tactical-radio
finish-args:
  - --device=all  # USB access
  - --socket=pulseaudio
  - --share=ipc
  - --socket=x11
modules:
  - name: rtl-sdr
    buildsystem: cmake
    sources:
      - type: git
        url: https://github.com/osmocom/rtl-sdr.git
```

### Debug Logging

```python
import logging

# Configure rotating log files
handler = RotatingFileHandler(
    '~/.config/vintage-tactical-radio/debug.log',
    maxBytes=10485760,  # 10MB
    backupCount=5
)

logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[handler]
)
```

This implementation guide provides concrete code examples and technical details that complement the PRD, giving the AI code assistant specific patterns to follow during development.