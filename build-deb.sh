#!/bin/bash
# Debian package build script for Vintage Tactical Radio

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building Debian package for Vintage Tactical Radio...${NC}"

# Get version from CMakeLists.txt
VERSION=$(grep "project(vintage-tactical-radio VERSION" CMakeLists.txt | cut -d' ' -f3 | tr -d ')')
ARCH=$(dpkg --print-architecture)
PACKAGE_NAME="vintage-tactical-radio_${VERSION}_${ARCH}"

echo -e "${YELLOW}Version: ${VERSION}${NC}"
echo -e "${YELLOW}Architecture: ${ARCH}${NC}"

# Clean previous builds
rm -rf debian_build
mkdir -p debian_build

# Build the project
echo -e "${GREEN}Building project...${NC}"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
cd ..

# Create debian package structure
echo -e "${GREEN}Creating package structure...${NC}"
mkdir -p debian_build/${PACKAGE_NAME}/DEBIAN
mkdir -p debian_build/${PACKAGE_NAME}/usr/bin
mkdir -p debian_build/${PACKAGE_NAME}/usr/share/applications
mkdir -p debian_build/${PACKAGE_NAME}/usr/share/pixmaps
mkdir -p debian_build/${PACKAGE_NAME}/usr/share/vintage-tactical-radio/images
mkdir -p debian_build/${PACKAGE_NAME}/usr/share/vintage-tactical-radio/sounds
mkdir -p debian_build/${PACKAGE_NAME}/usr/share/vintage-tactical-radio/themes

# Copy files
cp build/vintage-tactical-radio debian_build/${PACKAGE_NAME}/usr/bin/
cp packaging/vintage-tactical-radio.desktop debian_build/${PACKAGE_NAME}/usr/share/applications/
cp packaging/vintage-tactical-radio.png debian_build/${PACKAGE_NAME}/usr/share/pixmaps/
cp -r assets/images/* debian_build/${PACKAGE_NAME}/usr/share/vintage-tactical-radio/images/
cp -r assets/themes/* debian_build/${PACKAGE_NAME}/usr/share/vintage-tactical-radio/themes/

# Create control file
cat > debian_build/${PACKAGE_NAME}/DEBIAN/control << EOF
Package: vintage-tactical-radio
Version: ${VERSION}
Section: hamradio
Priority: optional
Architecture: ${ARCH}
Depends: libqt6core6, libqt6widgets6, libqt6multimedia6, libqt6opengl6, librtlsdr0, libfftw3-single3, libc6, libgcc1, libstdc++6
Maintainer: Vintage Radio Team <contact@vintage-radio.org>
Homepage: https://github.com/vintage-tactical-radio
Description: RTL-SDR based vintage military radio application
 Vintage Tactical Radio is a native Linux application that emulates
 a vintage military/tactical radio interface while leveraging modern
 RTL-SDR v3 hardware for software-defined radio reception.
 .
 Features include wide frequency coverage (500 kHz to 1.7 GHz),
 multiple demodulation modes, 7-band parametric equalizer,
 real-time spectrum display, and authentic military radio aesthetics.
EOF

# Create postinst script
cat > debian_build/${PACKAGE_NAME}/DEBIAN/postinst << 'EOF'
#!/bin/bash
set -e

# Update desktop database
if [ -x /usr/bin/update-desktop-database ]; then
    /usr/bin/update-desktop-database -q
fi

# Update icon cache
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    /usr/bin/gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor
fi

exit 0
EOF

chmod 755 debian_build/${PACKAGE_NAME}/DEBIAN/postinst

# Create postrm script
cat > debian_build/${PACKAGE_NAME}/DEBIAN/postrm << 'EOF'
#!/bin/bash
set -e

# Update desktop database
if [ -x /usr/bin/update-desktop-database ]; then
    /usr/bin/update-desktop-database -q
fi

# Update icon cache
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    /usr/bin/gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor
fi

exit 0
EOF

chmod 755 debian_build/${PACKAGE_NAME}/DEBIAN/postrm

# Set permissions
chmod 755 debian_build/${PACKAGE_NAME}/usr/bin/vintage-tactical-radio

# Build the package
echo -e "${GREEN}Building Debian package...${NC}"
cd debian_build
dpkg-deb --build ${PACKAGE_NAME}
cd ..

# Move package to main directory
mv debian_build/${PACKAGE_NAME}.deb .

echo -e "${GREEN}Package built successfully: ${PACKAGE_NAME}.deb${NC}"

# Verify package
echo -e "${YELLOW}Package information:${NC}"
dpkg-deb --info ${PACKAGE_NAME}.deb

# Clean up
rm -rf debian_build

echo -e "${GREEN}Done!${NC}"
