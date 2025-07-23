#!/bin/bash
# Make all build scripts executable

chmod +x build.sh
chmod +x rebuild.sh
chmod +x build-deb.sh
chmod +x build-deb-proper.sh
chmod +x build-appimage.sh
chmod +x build-flatpak.sh
chmod +x run-optimized.sh
chmod +x make-all-executable.sh
chmod +x verify-prd-alignment.sh
chmod +x debian/rules

echo "All build scripts are now executable!"
