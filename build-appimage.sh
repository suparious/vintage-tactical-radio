#!/bin/bash
# AppImage build script for Vintage Tactical Radio

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building Vintage Tactical Radio AppImage ===${NC}"

# Build the application first
if [ ! -f "build/vintage-tactical-radio" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    ./build.sh
fi

# Create AppDir structure
echo -e "${YELLOW}Creating AppDir structure...${NC}"
rm -rf AppDir
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/lib
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps
mkdir -p AppDir/usr/share/vintage-tactical-radio

# Copy binary
cp build/vintage-tactical-radio AppDir/usr/bin/

# Copy desktop file
cp packaging/vintage-tactical-radio.desktop AppDir/usr/share/applications/

# Copy icon
if [ -f "packaging/vintage-tactical-radio.png" ]; then
    cp packaging/vintage-tactical-radio.png AppDir/usr/share/icons/hicolor/256x256/apps/
else
    # Convert SVG to PNG if possible
    if command -v convert &> /dev/null; then
        convert -background none -resize 256x256 assets/images/radio-icon.svg AppDir/usr/share/icons/hicolor/256x256/apps/vintage-tactical-radio.png
    fi
fi

# Copy assets
cp -r assets/images AppDir/usr/share/vintage-tactical-radio/
cp -r assets/themes AppDir/usr/share/vintage-tactical-radio/
mkdir -p AppDir/usr/share/vintage-tactical-radio/sounds

# Create AppRun script
cat > AppDir/AppRun << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins:${QT_PLUGIN_PATH}"
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS}"
exec "${HERE}/usr/bin/vintage-tactical-radio" "$@"
EOF

chmod +x AppDir/AppRun

# Download appimagetool if not present
if [ ! -f "appimagetool-x86_64.AppImage" ]; then
    echo -e "${YELLOW}Downloading appimagetool...${NC}"
    wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x appimagetool-x86_64.AppImage
fi

# Use linuxdeploy if available for better dependency handling
if command -v linuxdeploy &> /dev/null; then
    echo -e "${YELLOW}Running linuxdeploy...${NC}"
    linuxdeploy --appdir AppDir --plugin qt --output appimage
else
    echo -e "${YELLOW}Creating AppImage with appimagetool...${NC}"
    # Copy dependencies manually (simplified)
    echo -e "${YELLOW}Note: For better dependency handling, install linuxdeploy${NC}"
    
    # Copy some critical libraries
    for lib in librtlsdr.so* libusb-1.0.so*; do
        if [ -f "/usr/lib/x86_64-linux-gnu/$lib" ]; then
            cp /usr/lib/x86_64-linux-gnu/$lib AppDir/usr/lib/
        fi
    done
    
    # Generate AppImage
    ARCH=x86_64 ./appimagetool-x86_64.AppImage AppDir vintage-tactical-radio-x86_64.AppImage
fi

echo -e "${GREEN}=== AppImage created successfully ===${NC}"
echo -e "${GREEN}AppImage: $(pwd)/vintage-tactical-radio-x86_64.AppImage${NC}"
