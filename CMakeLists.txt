cmake_minimum_required(VERSION 3.16)
project(vintage-tactical-radio VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia OpenGL)
find_package(PkgConfig REQUIRED)

# Find RTL-SDR library
pkg_check_modules(RTLSDR REQUIRED librtlsdr)

# Find FFTW3
pkg_check_modules(FFTW3 REQUIRED fftw3f)

# Find audio libraries
pkg_check_modules(ALSA alsa)
pkg_check_modules(PULSE libpulse)

# Optional: Find spdlog for logging
find_package(spdlog)

# Set up includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RTLSDR_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

# Collect source files
set(SOURCES
    src/main.cpp
    src/core/RTLSDRDevice.cpp
    src/core/DSPEngine.cpp
    src/core/RingBuffer.cpp
    src/audio/AudioOutput.cpp
    src/audio/VintageEqualizer.cpp
    src/dsp/AMDemodulator.cpp
    src/dsp/FMDemodulator.cpp
    src/dsp/AGC.cpp
    src/dsp/Squelch.cpp
    src/dsp/NoiseReduction.cpp
    src/ui/MainWindow.cpp
    src/ui/VintageKnob.cpp
    src/ui/VintageMeter.cpp
    src/ui/FrequencyDial.cpp
    src/ui/SpectrumDisplay.cpp
    src/ui/VintageTheme.cpp
    src/config/Settings.cpp
    src/config/MemoryChannel.cpp
)

set(HEADERS
    src/core/RTLSDRDevice.h
    src/core/DSPEngine.h
    src/core/RingBuffer.h
    src/audio/AudioOutput.h
    src/audio/VintageEqualizer.h
    src/dsp/AMDemodulator.h
    src/dsp/FMDemodulator.h
    src/dsp/AGC.h
    src/dsp/Squelch.h
    src/dsp/NoiseReduction.h
    src/ui/MainWindow.h
    src/ui/VintageKnob.h
    src/ui/VintageMeter.h
    src/ui/FrequencyDial.h
    src/ui/SpectrumDisplay.h
    src/ui/VintageTheme.h
    src/config/Settings.h
    src/config/MemoryChannel.h
)

# Resources
qt6_add_resources(RESOURCE_FILES assets/resources.qrc)

# Create executable
add_executable(vintage-tactical-radio ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

# Link libraries
target_link_libraries(vintage-tactical-radio
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::OpenGL
    ${RTLSDR_LIBRARIES}
    ${FFTW3_LIBRARIES}
    pthread
)

# Link optional libraries
if(ALSA_FOUND)
    target_link_libraries(vintage-tactical-radio ${ALSA_LIBRARIES})
    target_compile_definitions(vintage-tactical-radio PRIVATE HAS_ALSA=1)
endif()

if(PULSE_FOUND)
    target_link_libraries(vintage-tactical-radio ${PULSE_LIBRARIES})
    target_compile_definitions(vintage-tactical-radio PRIVATE HAS_PULSE=1)
endif()

if(spdlog_FOUND)
    target_link_libraries(vintage-tactical-radio spdlog::spdlog)
    target_compile_definitions(vintage-tactical-radio PRIVATE HAS_SPDLOG=1)
endif()

# Set compiler flags
target_compile_options(vintage-tactical-radio PRIVATE
    -Wall -Wextra -Wpedantic
    -O3 -march=native
    -ffast-math
)

# Installation
install(TARGETS vintage-tactical-radio RUNTIME DESTINATION bin)
install(DIRECTORY assets/images DESTINATION share/vintage-tactical-radio)
install(DIRECTORY assets/sounds DESTINATION share/vintage-tactical-radio)
install(DIRECTORY assets/themes DESTINATION share/vintage-tactical-radio)
install(FILES packaging/vintage-tactical-radio.desktop DESTINATION share/applications)
install(FILES packaging/vintage-tactical-radio.png DESTINATION share/pixmaps)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "vintage-tactical-radio")
set(CPACK_PACKAGE_VENDOR "Vintage Radio Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RTL-SDR based vintage military radio application")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6multimedia6, librtlsdr0, libfftw3-single3")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vintage Radio Team")

include(CPack)
