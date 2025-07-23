#!/bin/bash
# Clean build script - removes build directory and rebuilds from scratch

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Clean Build of Vintage Tactical Radio ===${NC}"

# Confirm with user
echo -e "${YELLOW}This will remove the build directory and rebuild from scratch.${NC}"
echo -n "Continue? (y/N) "
read -r response

if [[ ! "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo -e "${RED}Cancelled.${NC}"
    exit 1
fi

# Clean build directory
echo -e "${GREEN}Removing build directory...${NC}"
rm -rf build

# Clean any generated files
echo -e "${GREEN}Cleaning generated files...${NC}"
rm -f vintage-tactical-radio-*.AppImage 2>/dev/null || true
rm -f vintage-tactical-radio-*.deb 2>/dev/null || true
rm -f vintage-tactical-radio.flatpak 2>/dev/null || true
rm -rf _build _repo 2>/dev/null || true
rm -rf AppDir 2>/dev/null || true

echo -e "${GREEN}Starting fresh build...${NC}"
./build.sh

if [ $? -eq 0 ]; then
    echo -e "${GREEN}=== Clean build completed successfully! ===${NC}"
    echo ""
    echo -e "${GREEN}To run:${NC} ./build/vintage-tactical-radio"
    echo -e "${GREEN}Or use:${NC} ./run-optimized.sh"
else
    echo -e "${RED}Clean build failed!${NC}"
    exit 1
fi
