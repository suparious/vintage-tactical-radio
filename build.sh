#!/bin/bash
# Build script for Vintage Tactical Radio

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building Vintage Tactical Radio ===${NC}"

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"
for dep in cmake make g++ pkg-config; do
    if ! command -v $dep &> /dev/null; then
        echo -e "${RED}Error: $dep is not installed${NC}"
        exit 1
    fi
done

# Check for Qt6
if ! pkg-config --exists Qt6Core; then
    echo -e "${RED}Error: Qt6 is not installed${NC}"
    exit 1
fi

# Check for rtl-sdr
if ! pkg-config --exists librtlsdr; then
    echo -e "${RED}Error: librtlsdr is not installed${NC}"
    exit 1
fi

# Create build directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure
echo -e "${YELLOW}Configuring...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo -e "${YELLOW}Building...${NC}"
make -j$(nproc)

echo -e "${GREEN}Build complete!${NC}"
echo -e "${GREEN}Binary location: $(pwd)/vintage-tactical-radio${NC}"

# Generate PNG icon from SVG if ImageMagick is available
if command -v convert &> /dev/null; then
    echo -e "${YELLOW}Generating PNG icon...${NC}"
    convert -background none -resize 256x256 ../assets/images/radio-icon.svg ../packaging/vintage-tactical-radio.png
    echo -e "${GREEN}PNG icon generated${NC}"
else
    echo -e "${YELLOW}ImageMagick not found, skipping PNG icon generation${NC}"
fi

echo -e "${GREEN}=== Build finished successfully ===${NC}"
echo ""
echo "To install system-wide, run:"
echo "  cd build && sudo make install"
echo ""
echo "To run directly from build directory:"
echo "  ./build/vintage-tactical-radio"
