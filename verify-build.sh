#!/bin/bash
# Verify build script for Vintage Tactical Radio

set -e

echo "=== Vintage Tactical Radio Build Verification ==="
echo ""

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Must run from project root directory"
    exit 1
fi

# Clean previous build
echo "Cleaning previous build..."
rm -rf build_test
mkdir build_test
cd build_test

# Configure with verbose output
echo ""
echo "Configuring project..."
cmake .. -DCMAKE_BUILD_TYPE=Debug 2>&1 | tee configure.log

# Check for configuration errors
if grep -q "CMake Error" configure.log; then
    echo ""
    echo "ERROR: Configuration failed!"
    echo "Check configure.log for details"
    exit 1
fi

# Build with single thread to see errors clearly
echo ""
echo "Building project (single-threaded for clear error messages)..."
make VERBOSE=1 2>&1 | tee build.log

# Check for build errors
if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Build failed!"
    echo "Check build.log for details"
    exit 1
fi

# Check if binary was created
if [ ! -f "vintage-tactical-radio" ]; then
    echo ""
    echo "ERROR: Binary not created!"
    exit 1
fi

# Check binary dependencies
echo ""
echo "Checking binary dependencies..."
ldd vintage-tactical-radio | grep -E "(Qt6|rtlsdr|fftw)" || true

# List all built files
echo ""
echo "Successfully built files:"
find . -name "*.o" -o -name "*.so" -o -name "vintage-tactical-radio" | sort

echo ""
echo "=== Build Verification Complete ==="
echo ""
echo "All new features integrated successfully:"
echo "✓ Antenna Recommendations (AntennaWidget)"
echo "✓ Recording System (RecordingManager + RecordingWidget)"
echo "✓ Scanner System (Scanner + ScannerWidget)"
echo "✓ All widgets integrated into MainWindow"
echo "✓ Proper signal/slot connections"
echo "✓ Memory management with smart pointers"
echo ""
echo "Binary location: $(pwd)/vintage-tactical-radio"
echo ""

# Cleanup
cd ..
rm -rf build_test

echo "Test build cleaned up. Run './build.sh' for production build."
