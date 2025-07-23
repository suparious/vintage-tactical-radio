#!/bin/bash
# Verify PRD alignment for Vintage Tactical Radio

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Vintage Tactical Radio PRD Alignment Verification ===${NC}\n"

# Function to check if a feature is implemented
check_feature() {
    local feature="$1"
    local file="$2"
    local pattern="$3"
    
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $feature"
        return 0
    else
        echo -e "${RED}✗${NC} $feature"
        return 1
    fi
}

echo -e "${YELLOW}Checking RTL-SDR Features:${NC}"
check_feature "Gain range 0-49.6dB" "src/ui/MainWindow.cpp" "gainKnob_->setRange(0, 49.6)"
check_feature "Direct sampling mode" "src/core/RTLSDRDevice.h" "DirectSamplingMode"
check_feature "Bias-T control" "src/ui/MainWindow.cpp" "biasTCheck_"
check_feature "PPM correction" "src/ui/MainWindow.cpp" "ppmSpin_"
check_feature "Sample rate control" "src/ui/MainWindow.cpp" "rtlSampleRateCombo_"

echo -e "\n${YELLOW}Checking Audio Features:${NC}"
check_feature "192kHz sample rate" "src/ui/MainWindow.cpp" "192 kHz"
check_feature "7-band EQ presets" "src/audio/VintageEqualizer.cpp" "Voice.*Music.*DX"
check_feature "Modern/Nostalgic EQ modes" "src/audio/VintageEqualizer.cpp" "modernFrequencies_.*nostalgicFrequencies_"

echo -e "\n${YELLOW}Checking Packaging:${NC}"
check_feature "Debian packaging" "debian/control" "Package: vintage-tactical-radio"
check_feature "AppImage support" "build-appimage.sh" "linuxdeploy"
check_feature "Flatpak manifest" "io.github.vintage_tactical_radio.yml" "app-id: io.github.vintage_tactical_radio"

echo -e "\n${YELLOW}Checking Build Scripts:${NC}"
for script in build.sh build-deb-proper.sh build-appimage.sh build-flatpak.sh run-optimized.sh; do
    if [ -x "$script" ]; then
        echo -e "${GREEN}✓${NC} $script is executable"
    else
        echo -e "${RED}✗${NC} $script is not executable"
    fi
done

echo -e "\n${YELLOW}Checking Source Files:${NC}"
echo -n "Total source files: "
find src -name "*.cpp" -o -name "*.h" | wc -l

echo -n "Total lines of code: "
find src -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1 | awk '{print $1}'

echo -e "\n${YELLOW}Missing Features (from PRD):${NC}"
echo -e "${YELLOW}•${NC} Recording capabilities (audio/IQ)"
echo -e "${YELLOW}•${NC} Scanning functions"
echo -e "${YELLOW}•${NC} Network features"
echo -e "${YELLOW}•${NC} Digital mode decoding (RDS, CTCSS, ADS-B)"
echo -e "${YELLOW}•${NC} Antenna recommendations UI"

echo -e "\n${GREEN}=== Verification Complete ===${NC}"
