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
    qtshadertools
    qtsvg
)

# Required for compression support
packages_to_install+=(
    zlib
)

# Required for GUI test infrastructure (Layer 7 coverage tooling).
# llvm provides `llvm-cov gcov` shim used by gcovr on macOS clang builds.
# gnu-time provides `gtime`, the GNU `time -v` implementation that
# run_vtr_flow.py uses for per-stage memory tracking; macOS BSD
# /usr/bin/time does not support `-v` (DEF-006).
packages_to_install+=(
    gcovr
    llvm
    gnu-time
)

echo "Installing VTR build dependencies via Homebrew..."
brew install "${packages_to_install[@]}"

# ---------------------------------------------------------------------------
# Python runtime dependencies (requirements.txt)
# ---------------------------------------------------------------------------
# run_vtr_flow.py and run_reg_test.py import prettytable, lxml, psutil,
# pandas, numpy, scipy. Layer 5 visual regression additionally needs
# scikit-image and Pillow. Without these, the GUI test plan's Session S0
# fails on a fresh checkout. We install into a project-local virtualenv
# (.venv at the repo root) to avoid PEP 668 "externally managed"
# breakage on Homebrew Python 3.12+.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/.venv"

echo ""
echo "Setting up Python virtualenv at ${VENV_DIR}..."
if [ ! -d "${VENV_DIR}" ]; then
    python3 -m venv "${VENV_DIR}"
fi
# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"
pip install --upgrade pip
pip install -r "${SCRIPT_DIR}/requirements.txt"
pip install scikit-image Pillow  # Layer 5 SSIM compare
deactivate

echo ""
echo "====================================================================="
echo "  macOS build dependencies installed successfully."
echo ""
echo "  NOTE: macOS ships with outdated versions of bison and flex."
echo "  The Makefile will automatically use Homebrew's versions."
echo ""
echo "  Activate the Python venv before running VTR scripts:"
echo "    source ${VENV_DIR}/bin/activate"
echo ""
echo "  To build VTR, run:"
echo "    make -j\$(sysctl -n hw.ncpu)"
echo "====================================================================="
