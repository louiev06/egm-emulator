#!/bin/bash

# Simple build script for EGM Emulator (when CMake is not available)

set -e

echo "Building EGM Emulator C++ Project"
echo "=================================="

# Compiler settings
CXX=${CXX:-g++}
CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -I./include -pthread"
SRCDIR="src"
BUILDDIR="build_manual"
OBJDIR="$BUILDDIR/obj"

# Create build directories
mkdir -p "$OBJDIR/event"
mkdir -p "$OBJDIR/io"
mkdir -p "$OBJDIR/sas"
mkdir -p "$OBJDIR/simulator"

echo "Compiler: $CXX"
echo "Flags: $CXXFLAGS"
echo ""

# Compile source files
echo "Compiling source files..."

echo "  EventService.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/event/EventService.cpp" -o "$OBJDIR/event/EventService.o"

echo "  CommChannel.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/io/CommChannel.cpp" -o "$OBJDIR/io/CommChannel.o"

echo "  SimulatedPlatform.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/io/SimulatedPlatform.cpp" -o "$OBJDIR/io/SimulatedPlatform.o"

echo "  SASConstants.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/sas/SASConstants.cpp" -o "$OBJDIR/sas/SASConstants.o"

echo "  Game.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/simulator/Game.cpp" -o "$OBJDIR/simulator/Game.o"

echo "  Machine.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/simulator/Machine.cpp" -o "$OBJDIR/simulator/Machine.o"

echo "  main.cpp"
$CXX $CXXFLAGS -c "$SRCDIR/simulator/main.cpp" -o "$OBJDIR/simulator/main.o"

# Link executable
echo ""
echo "Linking executable..."
$CXX $CXXFLAGS \
    "$OBJDIR/event/EventService.o" \
    "$OBJDIR/io/CommChannel.o" \
    "$OBJDIR/io/SimulatedPlatform.o" \
    "$OBJDIR/sas/SASConstants.o" \
    "$OBJDIR/simulator/Game.o" \
    "$OBJDIR/simulator/Machine.o" \
    "$OBJDIR/simulator/main.o" \
    -o "$BUILDDIR/egm_simulator"

echo ""
echo "=================================="
echo "Build complete!"
echo "Executable: $BUILDDIR/egm_simulator"
echo ""
echo "To run: ./$BUILDDIR/egm_simulator"
