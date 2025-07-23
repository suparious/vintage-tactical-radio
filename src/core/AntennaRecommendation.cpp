#include "AntennaRecommendation.h"

const QVector<AntennaRecommendation::FrequencyBand>& AntennaRecommendation::getBands() {
    static const QVector<FrequencyBand> bands = {
        // AM Broadcast (530-1700 kHz)
        {530e3, 1700e3, {
            "Long Wire with 9:1 Unun",
            "530-1700 kHz (AM Broadcast)",
            "-3 to +6 dBi",
            "450Ω → 50Ω",
            "9:1 Unun Required",
            "Minimum 30m wire recommended. Height improves reception. Ground connection beneficial.",
            "wire"
        }},
        
        // HF Amateur (1.8-30 MHz)
        {1.8e6, 30e6, {
            "Dipole or Long Wire",
            "1.8-30 MHz (HF)",
            "2.15 dBi (dipole)",
            "50-75Ω",
            "May need tuner",
            "Dipole: λ/2 length = 468/f(MHz) feet. Long wire with tuner for multiband.",
            "dipole"
        }},
        
        // FM Broadcast (88-108 MHz)
        {88e6, 108e6, {
            "Dipole or Discone",
            "88-108 MHz (FM Broadcast)",
            "2.15 dBi (dipole), Unity (discone)",
            "75Ω (dipole), 50Ω (discone)",
            "Direct connection",
            "Horizontal dipole for local stations. Discone for wide coverage. ~1m dipole length.",
            "dipole"
        }},
        
        // Aviation Band (108-137 MHz)
        {108e6, 137e6, {
            "Discone or Vertical",
            "108-137 MHz (Aviation)",
            "Unity to 3 dBi",
            "50Ω",
            "Direct connection",
            "Discone excellent for scanning. 1/4 wave vertical ~50cm. Higher is better.",
            "vertical"
        }},
        
        // VHF Marine (156-162 MHz)
        {156e6, 162e6, {
            "Marine Whip or Yagi",
            "156-162 MHz (Marine VHF)",
            "3-6 dBi (whip), 6-10 dBi (yagi)",
            "50Ω",
            "Direct connection",
            "Marine whip for boats. Yagi for shore stations. Gain improves range.",
            "whip"
        }},
        
        // VHF Communications (138-174 MHz)
        {138e6, 174e6, {
            "Discone or Yagi",
            "138-174 MHz (VHF Comms)",
            "Unity to 10 dBi",
            "50Ω",
            "Direct connection",
            "Discone for scanning. Yagi for specific direction. 5/8 wave for base station.",
            "vertical"
        }},
        
        // UHF Business (450-470 MHz)
        {450e6, 470e6, {
            "Discone or Vertical",
            "450-470 MHz (UHF Business)",
            "Unity to 5 dBi",
            "50Ω",
            "Direct connection",
            "1/4 wave ~16cm. Discone covers wide range. Collinear for more gain.",
            "vertical"
        }},
        
        // 70cm Amateur (420-450 MHz)
        {420e6, 450e6, {
            "Yagi or Vertical",
            "420-450 MHz (70cm Amateur)",
            "5-15 dBi",
            "50Ω",
            "Direct connection",
            "Yagi for weak signals. Simple 1/4 wave vertical for local. Higher gain = narrower pattern.",
            "yagi"
        }},
        
        // Cellular/Public Safety (806-960 MHz)
        {806e6, 960e6, {
            "Discone or Yagi",
            "806-960 MHz (Cellular/Public Safety)",
            "Unity to 10 dBi",
            "50Ω",
            "Direct connection",
            "Discone for trunked systems. Yagi for specific sites. Keep cable short at UHF.",
            "vertical"
        }},
        
        // ADS-B (1090 MHz)
        {1090e6, 1090e6, {
            "Vertical or Collinear",
            "1090 MHz (ADS-B)",
            "3-6 dBi",
            "50Ω",
            "Direct connection",
            "1/4 wave ground plane or collinear. Avoid too much gain (aircraft overhead).",
            "vertical"
        }},
        
        // 23cm Amateur (1240-1300 MHz)
        {1240e6, 1300e6, {
            "Yagi or Helix",
            "1240-1300 MHz (23cm Amateur)",
            "10-15 dBi",
            "50Ω",
            "Direct connection",
            "High gain needed at 23cm. Use low-loss cable. Preamp recommended.",
            "yagi"
        }},
        
        // General UHF (300-1700 MHz)
        {300e6, 1700e6, {
            "Discone",
            "300-1700 MHz (General UHF)",
            "Unity gain",
            "50Ω",
            "Direct connection",
            "Wideband coverage but unity gain. Good general purpose scanner antenna.",
            "discone"
        }}
    };
    
    return bands;
}

AntennaRecommendation::Recommendation AntennaRecommendation::getRecommendation(double frequencyHz) {
    const auto& bands = getBands();
    
    // Find the best matching band
    for (const auto& band : bands) {
        if (frequencyHz >= band.minFreq && frequencyHz <= band.maxFreq) {
            return band.recommendation;
        }
    }
    
    // Default recommendation if frequency is out of range
    if (frequencyHz < 530e3) {
        return {
            "Long Wire or Active Antenna",
            "Below 530 kHz",
            "Varies",
            "High impedance",
            "Matching unit required",
            "VLF/LF requires very long antennas or active designs. Consider loop antenna.",
            "loop"
        };
    }
    
    return {
        "Discone or Log Periodic",
        "General Coverage",
        "Unity to 6 dBi",
        "50Ω",
        "Direct connection",
        "For wide frequency coverage, use discone or LPDA antenna.",
        "discone"
    };
}

QString AntennaRecommendation::getAntennaAdvice(double frequencyHz) {
    auto rec = getRecommendation(frequencyHz);
    
    QString advice = QString("<b>Recommended Antenna:</b> %1<br>")
                    .arg(rec.antennaType);
    advice += QString("<b>Frequency Range:</b> %1<br>").arg(rec.frequencyRange);
    advice += QString("<b>Typical Gain:</b> %1<br>").arg(rec.gain);
    advice += QString("<b>Impedance:</b> %1<br>").arg(rec.impedance);
    
    if (!rec.matchingRequired.isEmpty() && rec.matchingRequired != "Direct connection") {
        advice += QString("<b>Matching:</b> %1<br>").arg(rec.matchingRequired);
    }
    
    advice += QString("<br><b>Tips:</b> %1").arg(rec.notes);
    
    return advice;
}
