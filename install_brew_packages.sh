#!/bin/bash
#
# Install required packages for building VTR on macOS using Homebrew.
#
# Usage:
#   ./install_brew_packages.sh
#
# Prerequisites:
#   Homebrew must be installed (https://brew.sh)
#   Xcode Command Line Tools: xcode-select --install

set -euo pipefail

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew is not installed."
    echo "Install it from https://brew.sh"
    exit 1
fi

# Base packages to compile and run basic regression tests
packages_to_install=(
    cmake
    pkg-config
    bison
    flex
    python3
)

# Packages for more complex features of VTR that most people will use.
packages_to_install+=(
    tbb
    eigen
)

# Required for parsing SDC files (see LibSDCParse)
packages_to_install+=(
    tcl-tk
    swig
)

# Required for Qt6 graphics (ezgl Qt layer)
packages_to_install+=(
    qtbase
    qtsvg
)

# Required for compression support
packages_to_install+=(
    zlib
)

echo "Installing VTR build dependencies via Homebrew..."
brew install "${packages_to_install[@]}"

echo ""
echo "====================================================================="
echo "  macOS build dependencies installed successfully."
echo ""
echo "  NOTE: macOS ships with outdated versions of bison and flex."
echo "  The Makefile will automatically use Homebrew's versions."
echo ""
echo "  To build VTR, run:"
echo "    make -j\$(sysctl -n hw.ncpu)"
echo "====================================================================="
