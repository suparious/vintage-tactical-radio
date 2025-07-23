#!/bin/bash
# Run Vintage Tactical Radio with optimized settings

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting Vintage Tactical Radio with optimized settings...${NC}"

# Check if binary exists
if [ ! -f "build/vintage-tactical-radio" ]; then
    echo -e "${RED}Error: Application not built. Run ./build.sh first.${NC}"
    exit 1
fi

# Set CPU governor to performance (if available)
if command -v cpupower >/dev/null 2>&1; then
    echo -e "${YELLOW}Setting CPU governor to performance mode...${NC}"
    sudo cpupower frequency-set -g performance 2>/dev/null || true
fi

# Increase USB buffer size for RTL-SDR
if [ -w /sys/module/usbcore/parameters/usbfs_memory_mb ]; then
    echo -e "${YELLOW}Increasing USB buffer size...${NC}"
    echo 16 | sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null
fi

# Check for RTL-SDR device
if lsusb | grep -q "RTL2838\|RTL2832"; then
    echo -e "${GREEN}RTL-SDR device detected${NC}"
else
    echo -e "${YELLOW}Warning: No RTL-SDR device detected${NC}"
fi

# Set real-time priority if possible
if groups | grep -q audio; then
    echo -e "${GREEN}Running with real-time priority...${NC}"
    # Use chrt for real-time scheduling
    if command -v chrt >/dev/null 2>&1; then
        exec chrt -f 50 ./build/vintage-tactical-radio "$@"
    else
        exec ./build/vintage-tactical-radio "$@"
    fi
else
    echo -e "${YELLOW}Note: Add yourself to 'audio' group for real-time priority:${NC}"
    echo -e "  sudo usermod -a -G audio $USER"
    echo -e "  (logout and login required)"
    exec ./build/vintage-tactical-radio "$@"
fi
