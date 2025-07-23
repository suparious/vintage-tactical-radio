# Digital Decoder Implementation Summary

## Overview
The Vintage Tactical Radio app now includes three digital decoders that enhance the radio experience by extracting digital information from analog signals:

1. **CTCSS (Continuous Tone-Coded Squelch System)**
2. **RDS (Radio Data System)**  
3. **ADS-B (Automatic Dependent Surveillance-Broadcast)**

## Implementation Details

### CTCSS Decoder
- **Status**: Fully functional
- **Algorithm**: Goertzel algorithm for efficient single-frequency detection
- **Features**:
  - Detects 38 standard CTCSS tones (67.0 Hz to 254.1 Hz)
  - Additional support for 12 extended Motorola tones
  - Real-time tone detection with adjustable threshold
  - Visual indication of detected tone frequency and level
  - Detection history log

### RDS Decoder
- **Status**: Basic implementation (needs refinement)
- **Features**:
  - Program Service (PS) name extraction
  - RadioText (RT) display
  - Program Type (PTY) identification
  - Traffic announcements (TA/TP)
  - Clock time extraction
- **TODO**:
  - Implement proper error correction
  - Add syndrome calculation
  - Improve carrier recovery PLL
  - Add support for alternative frequencies (AF)

### ADS-B Decoder
- **Status**: Basic implementation (needs refinement)
- **Features**:
  - Mode-S message decoding at 1090 MHz
  - Aircraft identification (ICAO address and callsign)
  - Position decoding (latitude/longitude)
  - Altitude and speed information
  - Real-time aircraft table display
- **TODO**:
  - Implement proper CRC checking
  - Improve CPR position decoding
  - Add support for extended squitter subtypes
  - Implement surface position messages

## UI Integration

### Decoder Panel
- Dedicated "DIGITAL DECODERS" panel in the main window
- Three tabs for CTCSS, RDS, and ADS-B
- Automatic enable/disable based on frequency and mode:
  - **CTCSS**: Available for AM, FM-Narrow, and FM-Wide modes
  - **RDS**: Only available for FM-Wide in 88-108 MHz band
  - **ADS-B**: Only available at 1090 MHz ±1 MHz

### Visual Design
- Vintage military theme with amber/green displays
- Monospace fonts for data display
- Status indicators and real-time updates
- History logs and data tables

## Technical Architecture

### Signal Flow
1. RTL-SDR → IQ samples → DSPEngine
2. DSPEngine demodulates to audio or processes raw IQ
3. Audio sent to CTCSS/RDS decoders
4. Raw IQ sent to ADS-B decoder at 1090 MHz
5. Decoded data emitted via Qt signals
6. UI widgets update in real-time

### Key Classes
- `DigitalDecoder`: Base class for all decoders
- `CTCSSDecoder`: Goertzel-based tone detection
- `RDSDecoder`: 57kHz subcarrier extraction and BPSK demodulation
- `ADSBDecoder`: Mode-S message parsing
- `DecoderWidget`: UI for all three decoders
- `DSPEngine`: Manages decoder lifecycle and data routing

## Usage

### CTCSS
1. Tune to a frequency with CTCSS tones (e.g., repeater input)
2. Enable CTCSS detection in the decoder panel
3. The decoder will show detected tone frequency and level
4. Use for repeater access tone identification

### RDS
1. Tune to an FM broadcast station (88-108 MHz)
2. Enable RDS decoding
3. View station name, song info, and other metadata
4. Clock time updates automatically if broadcast

### ADS-B
1. Tune to 1090 MHz
2. Enable ADS-B decoding
3. Aircraft within range appear in the table
4. View callsign, altitude, speed, and position

## Future Enhancements

### Additional Decoders
- **DCS**: Digital Coded Squelch for digital tone squelch
- **SAME**: Specific Area Message Encoding for weather alerts
- **POCSAG**: Pager protocol decoding
- **APRS**: Amateur Packet Reporting System

### Improvements
- Network streaming of decoded data
- Database logging of detections
- Map display for ADS-B aircraft
- RDS TMC (Traffic Message Channel) support
- CTCSS tone generation for transmit (if supported)

## Performance Considerations

- Decoders run in the DSP thread for real-time performance
- UI updates use Qt's thread-safe signal/slot mechanism
- Efficient algorithms (Goertzel for CTCSS, FFT for spectrum)
- Minimal CPU usage when decoders are disabled

## Conclusion

The digital decoder implementation adds significant value to the Vintage Tactical Radio app, allowing users to extract digital information from analog signals. While CTCSS is fully functional, RDS and ADS-B provide a solid foundation for future enhancement. The modular architecture makes it easy to add new decoders and improve existing ones.
