#!/bin/bash
# Build Flatpak for Vintage Tactical Radio

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

FLATPAK_ID="io.github.vintage_tactical_radio"
FLATPAK_REPO="repo"
FLATPAK_BUNDLE="vintage-tactical-radio.flatpak"

echo -e "${GREEN}Building Flatpak for Vintage Tactical Radio...${NC}"

# Check for required tools
command -v flatpak >/dev/null 2>&1 || { 
    echo -e "${RED}flatpak not found. Please install flatpak and flatpak-builder:${NC}"
    echo "sudo apt install flatpak flatpak-builder"
    exit 1
}

command -v flatpak-builder >/dev/null 2>&1 || { 
    echo -e "${RED}flatpak-builder not found. Please install it:${NC}"
    echo "sudo apt install flatpak-builder"
    exit 1
}

# Add flathub repository if not already added
echo -e "${GREEN}Ensuring Flathub repository is available...${NC}"
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install required runtime and SDK
echo -e "${GREEN}Installing required runtime and SDK...${NC}"
flatpak install -y flathub org.freedesktop.Platform//23.08 org.freedesktop.Sdk//23.08

# Clean previous builds
echo -e "${GREEN}Cleaning previous builds...${NC}"
rm -rf _build _repo
mkdir -p _build

# Build the Flatpak
echo -e "${GREEN}Building Flatpak...${NC}"
flatpak-builder --force-clean --ccache --repo=${FLATPAK_REPO} _build ${FLATPAK_ID}.yml

# Create a single-file bundle
echo -e "${GREEN}Creating Flatpak bundle...${NC}"
flatpak build-bundle ${FLATPAK_REPO} ${FLATPAK_BUNDLE} ${FLATPAK_ID}

echo -e "${GREEN}Flatpak built successfully!${NC}"
echo -e "${YELLOW}Bundle: $(pwd)/${FLATPAK_BUNDLE}${NC}"
echo -e "\n${GREEN}To install:${NC}"
echo -e "  flatpak install ${FLATPAK_BUNDLE}"
echo -e "\n${GREEN}To run:${NC}"
echo -e "  flatpak run ${FLATPAK_ID}"

# Show bundle info
echo -e "\n${YELLOW}Bundle information:${NC}"
ls -lh ${FLATPAK_BUNDLE}

# Offer to install locally for testing
echo -e "\n${YELLOW}Would you like to install and test the Flatpak locally? (y/N)${NC}"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo -e "${GREEN}Installing Flatpak...${NC}"
    flatpak install -y --user ${FLATPAK_BUNDLE}
    
    echo -e "${GREEN}Running Flatpak...${NC}"
    flatpak run ${FLATPAK_ID}
fi
