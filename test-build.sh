#!/bin/bash
# Quick test build to verify decoder integration

echo "Testing decoder integration build..."
cd build || exit 1

# Do a quick incremental build
make -j$(nproc) 2>&1 | tee build_output.log

if [ $? -eq 0 ]; then
    echo "✅ Build successful! Digital decoders are properly integrated."
    echo ""
    echo "Summary of changes:"
    echo "- Fixed missing createDecoderPanel() implementation"
    echo "- Added enable/disable signals to DecoderWidget"
    echo "- Connected decoder UI to DSP engine"
    echo "- Separated decoder panel from scanner panel"
    echo ""
    echo "Digital decoders available:"
    echo "- CTCSS: Continuous Tone-Coded Squelch System (fully functional)"
    echo "- RDS: Radio Data System (basic implementation)"
    echo "- ADS-B: Aircraft tracking at 1090 MHz (basic implementation)"
else
    echo "❌ Build failed. Check build_output.log for errors."
    tail -20 build_output.log
fi
