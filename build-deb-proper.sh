#!/bin/bash
# Improved Debian package build script using proper debian/ structure

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building Debian package for Vintage Tactical Radio...${NC}"

# Check for required tools
command -v dpkg-buildpackage >/dev/null 2>&1 || { 
    echo -e "${RED}dpkg-buildpackage not found. Please install dpkg-dev:${NC}"
    echo "sudo apt install dpkg-dev devscripts"
    exit 1
}

# Get version from debian changelog
VERSION=$(dpkg-parsechangelog -S Version 2>/dev/null || echo "1.0.0-1")
PKG_NAME="vintage-tactical-radio"

echo -e "${YELLOW}Building version ${VERSION}...${NC}"

# Make debian/rules executable
chmod +x debian/rules

# Clean previous builds
echo -e "${GREEN}Cleaning previous builds...${NC}"
rm -rf ../build-deb
mkdir -p ../build-deb

# Create orig tarball if not exists
ORIG_VERSION=$(echo $VERSION | cut -d'-' -f1)
ORIG_TARBALL="../${PKG_NAME}_${ORIG_VERSION}.orig.tar.gz"

if [ ! -f "$ORIG_TARBALL" ]; then
    echo -e "${GREEN}Creating source tarball...${NC}"
    tar --exclude=build --exclude=build-deb --exclude=.git --exclude=debian \
        --exclude=*.deb --exclude=*.ddeb --exclude=*.changes \
        -czf "$ORIG_TARBALL" .
fi

# Build the package
echo -e "${GREEN}Building package...${NC}"

# Use debuild for unsigned builds
debuild -us -uc -b 2>&1 | tee ../build-deb/build.log

# Move packages to build-deb directory
echo -e "${GREEN}Moving packages...${NC}"
mv ../*.deb ../build-deb/ 2>/dev/null || true
mv ../*.changes ../build-deb/ 2>/dev/null || true
mv ../*.buildinfo ../build-deb/ 2>/dev/null || true

echo -e "${GREEN}Debian package built successfully!${NC}"
echo -e "${YELLOW}Package location: ../build-deb/${NC}"
ls -la ../build-deb/*.deb 2>/dev/null || echo -e "${RED}No .deb files found!${NC}"

# Show package info
if [ -f ../build-deb/*.deb ]; then
    echo -e "\n${YELLOW}Package information:${NC}"
    dpkg-deb --info ../build-deb/*.deb
fi
