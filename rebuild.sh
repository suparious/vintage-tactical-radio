#!/bin/bash
# Quick rebuild script for development

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Rebuilding Vintage Tactical Radio...${NC}"

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Build directory not found. Running full build..."
    ./build.sh
    exit 0
fi

cd build

# Run make
echo -e "${YELLOW}Compiling...${NC}"
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
    echo "Run with: ./build/vintage-tactical-radio"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
