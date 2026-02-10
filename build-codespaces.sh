#!/bin/bash
# build-codespaces.sh
# Cross-compile the project for Windows from GitHub Codespaces (Linux)
# Produces a standalone .exe you can download and run on Windows.
#
# Usage:
#   ./build-codespaces.sh          # Build Release (default)
#   ./build-codespaces.sh Debug    # Build Debug
#   ./build-codespaces.sh clean    # Remove build directory

set -e

BUILD_DIR="build-windows-cross"
BUILD_TYPE="${1:-Release}"
TOOLCHAIN_FILE="mingw-w64-toolchain.cmake"

# Handle 'clean' argument
if [ "$BUILD_TYPE" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "Done."
    exit 0
fi

# Check that the MinGW cross-compiler is installed
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "MinGW-w64 cross-compiler not found. Installing..."
    sudo apt-get update -qq && sudo apt-get install -y -qq mingw-w64 gcc-mingw-w64-x86-64
fi

echo "============================================"
echo " Building for Windows ($BUILD_TYPE)"
echo " Using MinGW-w64 cross-compiler"
echo "============================================"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -G "Unix Makefiles"

# Build (use all available cores)
cmake --build . --config "$BUILD_TYPE" -- -j$(nproc)

echo ""
echo "============================================"
echo " Build successful!"
echo "============================================"

EXE_PATH=$(find . -maxdepth 1 -name "*.exe" -type f | head -1)
if [ -n "$EXE_PATH" ]; then
    echo " Executable: $BUILD_DIR/$EXE_PATH"
    echo " Size: $(du -h "$EXE_PATH" | cut -f1)"
    echo ""
    echo " To download: right-click the .exe in the VS Code"
    echo " file explorer and select 'Download'"
else
    echo " Warning: No .exe found. Check build output above."
fi
echo "============================================"
