#!/bin/bash

# Base packages to compile and run basic regression tests
packages_to_install=(
    make
    cmake
    build-essential
    pkg-config
    bison
    flex
    python3-dev
    python3-venv
)

# Packages for more complex features of VTR that most people will use.
packages_to_install+=(
    libxml2-utils
    libtbb-dev
    libeigen3-dev
)

# Required by Qt6 GUI components
packages_to_install+=(
    libxkbcommon-dev
    libgl-dev
    libegl-dev
    libopengl0
    libegl-mesa0
    libgl1-mesa-dri
    qt6-shadertools-dev
)

# Required for parsing SDC files (see LibSDCParse)
packages_to_install+=(
    tcl-dev
    swig
)

# Required for parmys front-end from https://github.com/YosysHQ/yosys
packages_to_install+=(
    build-essential
    clang
    bison
    flex
    libreadline-dev
    gawk
    tcl-dev
    libffi-dev
    git
    graphviz
    xdot
    pkg-config
    python3-dev
    libboost-system-dev
    libboost-python-dev
    libboost-filesystem-dev
    default-jre
    zlib1g-dev
)

# Required for code formatting
# NOTE: clang-format-18 may only be found on specific distributions. Only
#       install it if the distribution has this version of clang format.
if apt-cache search '^clang-format-18$' | grep -q 'clang-format-18'; then
    packages_to_install+=(
        clang-format-18
    )
else
    echo "clang-format-18 not found in apt-cache. Skipping installation."
fi

# Required for GUI test infrastructure (Layer 7 coverage tooling).
packages_to_install+=(
    gcovr
)

sudo apt-get update
sudo apt-get install -y "${packages_to_install[@]}"

# Install Qt 6.9.3 via aqtinstall into /opt/qt6.
# 6.9.3 is the minimum required version: earlier Qt6 releases have internal
# bugs in the QRhi subsystem that cause rendering failures on our targets.
sudo pip3 install aqtinstall
sudo aqt install-qt linux desktop 6.9.3 linux_gcc_64 --outputdir /opt/qt6

# ---------------------------------------------------------------------------
# Python runtime dependencies (requirements.txt)
# ---------------------------------------------------------------------------
# run_vtr_flow.py and run_reg_test.py import prettytable, lxml, psutil,
# pandas, numpy, scipy. Layer 5 visual regression additionally needs
# scikit-image and Pillow. Without these, the GUI test plan's Session S0
# fails on a fresh checkout. We install into a project-local virtualenv
# (.venv at the repo root) to avoid PEP 668 "externally managed"
# breakage on Ubuntu 24.04+.
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
echo "  Linux build dependencies installed successfully."
echo ""
echo "  Activate the Python venv before running VTR scripts:"
echo "    source ${VENV_DIR}/bin/activate"
echo "====================================================================="
