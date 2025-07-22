# Prompt

I have a small side project that I need your help with. It is the RTL-SDR based Vintage FM Radio app, that runs as a native linux app. Mac and Windows support would be nice to have, but I don't want that to add unnecessary complexity to our project.

## Hardware

RTL-SDR v3 or compatible dongle (like a NESDR SMArt RTL-SDR v5) with a RTL2832U 1PPM TCXO chip. This will come with a R820T2 (R860-based) tuner chip that can handle 100 kHz to 1.75 GHz and has up to 3.2 MHz of instantaneous bandwidth. In debian linux 13, when I pug the USB, it shows up as "Realtek Semiconductor Corp. RTL2838 DVB-T" in 'lsusb'. I assume that it is easy to find out how to use this hardware. It doesn't need any drivers, and seemrd to work using apps like gnuradio and SDR++ out-of-the-box. As the source device, the RTL-SDR should be configured to use 2.4MHz instantaneous bandwidth, even thought our maximum is 3.2MHz. This should add a bit more stability by default.

## Supported Frequency ranges

Our Radio will support AM / FM Radio broadcast, also Canadian Maritime Radio monitoring. This will require changing the radio mode and changing the sampling bandwidth. That's all we will support for now, but we may decide to add in other ranges, like HAM radio and CB radio, once we can get some basic FM broadcast radio working first.

### 530-1700 kHz - AM Radio broadcast

AM radio broadcasting has assigned channels, ranging from 540 kHz to 1,700 kHz and spaced at 10-kHz intervals. If your desired station is at 750 kHz, adjacent channels lie 10 kHz above and below at 760 and 740 kHz. Each 10-kHz channel contains an upper sideband (USB) and a lower sideband (LSB). The RTL-SDR needs to hav it's radio changed to "AM", and "Direct Sampling" set to "Q Branch". Ensure that "Squelch" and "Carrier AGC" options enabled. Also set the radio sampling size to "10000Hz".

### 88-108 MHz - FM Radio broadcast

The FM broadcast in the United States and Canada starts at 88.0 MHz and ends at 108.0 MHz. The band is divided into 100 channels, each 200 kHz (0.2 MHz) wide. The center frequency is located at 1/2 the bandwidth of the FM Channel, or 100 kHz (0.1 MHz) up from the lower end of the channel. THe RTL-SDR needs to have the source "Direct Sampling" set to disabled, "RTL AGC" enabled, and the Radio set to "WFM" (wide FM), with a radio sampling size of "220000Hz", with "Stereo" enabled and the "Squelch" options enabled.

References and frequency tables:

- https://www.radioreference.com/db/aid/7766
- https://www.offshoreblue.com/comms/vhf-ca.php
- https://ised-isde.canada.ca/site/spectrum-management-telecommunications/en/learn-more/key-documents/consultations/canadian-table-frequency-allocations-sf10759

## Software

The UI should be in the design and theme of a vintage radio tuner, like one that you would see in military combat and spy movies. I want to use off-the-shelf open-source components, assets and libraries wherever possible. The UI should not be web-based, and should run as a native application that is installable either by debian package, appimage or a flatpak.
For the audio output, we should support ALSA and PulseAudio at the bare minimum and would be nice to also support USB Audio and HDMI outputs as well.
As for standard settings, we need the following user controllable settings:

- Customizable 7 band Audio Equalizer for audio output, with presets.
  - Two equalizer modes, "Modern" and "Nostalgic".
    - Modern EQ bands: 50Hz,125Hz,315Hz,750Hz,2.2kHz,6kHz,16kHz.
    - Nostalgic EQ bands: 60Hz,150Hz,400Hz,1kHz,2.4kHz,6kHz,15kHz
  - built-in pre-amp gain range is set to +/-12dB on all bands, and can be scaled up to +/-30dB max.
  - Default EQ preset is "Flat" with each band set to 0dB gain. Other presets could include "Full Bass and Treble", "Bass Boosted", "Treble Cut", "Flat", "Radio" and any other that you think we may be missing.
- Audio device selection
- Change the default sample rate between 44.1kHz, 48kHz and 192kHz.
- Change default sample fromat from 16bit (s16le) to 24bit (s24le).
- Radio device gain setting (not the pre-amp or equalizer gain)
  - There is 0dB to 46dB gain on the RTL-SDR hardware
- Squelch setting. This is from 0dB to -100dB. Our default will be -20dB.
- A "Reset all to defaults" button, for when things get all messed-up.

i think the primary elements in the Vintage Radio UI, would be the frequency, the radio mode (AM/FM-Wide/FM-Narrow/Etc...), the tuning (SW/AM Broadcast[or MW]/LW/CB Radio/HAM/FM Broadcast/HAM/ect..)
