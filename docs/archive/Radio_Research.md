# AM/FM Radio App

## Hardware

RTL-SDR v3 or compatible dongle (like a NESDR SMArt RTL-SDR v5) with a RTL2832U 1PPM TCXO chip. This will come with a R820T2 (R860-based) tuner chip that can handle 100 kHz to 1.75 GHz and has up to 3.2 MHz of instantaneous bandwidth. In debian linux 13, when I pug the USB, it shows up as "Realtek Semiconductor Corp. RTL2838 DVB-T" in 'lsusb'. I assume that it is easy to find out how to use this hardware. It doesn't need any drivers, and seemrd to work using apps like gnuradio and SDR++ out-of-the-box.

## Software

The UI should be in the design and theme of a modern radio, like one that you would see in a nice GMC/Chevy/Pontiac type of car. I have attached an example of a GMC Radio, and an example of an ugly basic radio. Somehow, we have to figure out how to make our radio look nice, while using off-the-shelf open-source components and assets. The UI should not be web-based, and should run as a native application that is installable either by debian package, appimage or a flatpak.

## Antennae

144/430MHz-Dual Band, VHF / UHF, NMO style for GMRS
 - HYSHIKRA GMRS NMO Antenna, 462-467Mhz UHF 17.7inches NMO Antenna with NMO Mount 4meter(13.1ft) PL259(UHF Male) RG58 Coax Cable and L Shape Fender Bracket Mount for Trunk Car Mobile Radio Transceiver

27MHz - HYSHIKRA CB Antenna 11Meter, 27Mhz SMA-Female

## 530-1700 kHz - AM Radio broadcast

https://worldradiomap.com/ca/vancouver

- 730   CKNW Global

## 88-108 MHz - FM Radio broadcast

https://worldradiomap.com/ca/vancouver

- Freq  Name              Signal  Genres
- 88.1  CBU CBC Radio One 5/5     Public Radio
- 90.9  CBUX ICI Musique  4/5     Public Radio
- 93.1  CKYE Red FM       4/5     International
- 93.7  CJJR JR Country   5/5     Pop Country
- 94.5  CFBT Virgin Radio 1/5     R&B, Rap
- 95.3  CKZZ Z95.3        1/5     Pop, Dance
- 96.1  CHKG China Radio  5/5     International
- 96.9  CJAX Jack FM      5/5     Urban Country
- 97.7  CBUF ICI Premiere 2/5     Public Radio
- 98.3  CIWV Wave FM      5/5     Chill Out
- 99.3  CFOX The Fox      3/5     Alternative Rock
- 101.1 CFMI Rock 101     5/5     Classic Rock
- 101.9 CITR Discorder    5/5     Variety
- 103.5 CHQM Move         4/5     Urban Pop
- 104.3 CHLG The Breeze   2/5     Alternative Rock
- 105.7 CBU CBC Music     5/5     Public Radio
- 106.3 CJNY Journey      5/4     Rock Vibes
