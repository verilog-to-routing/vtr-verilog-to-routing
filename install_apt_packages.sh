#!/bin/bash
#
# Linux build-dependency provisioner for VTR.
#
# Default behaviour (matches CI): install system packages with `sudo
# apt-get`, install Qt 6.9.3 to /opt/qt6 with `sudo aqt`, and create a
# project-local virtualenv at <repo>/.venv.
#
# Environment variables for hosts where the developer does not have
# sudo (e.g. a shared lab machine):
#
#   VTR_SKIP_APT=1
#       Skip the `sudo apt-get` step. The system packages listed in
#       this script must already be present (either pre-installed by an
#       admin or unavailable but tolerable for the build you intend to
#       run). The script does NOT verify them — that is the operator's
#       responsibility.
#
#   VTR_QT_PREFIX=/path/to/qt-install-root
#       Where Qt 6.9.3 is installed (default: /opt/qt6). When this path
#       (or its parent) is writable by the current user, the aqt step
#       runs without sudo. A common no-sudo choice is
#       "$HOME/software-pkgs/qt6"; once an admin moves the tree to a
#       shared location (e.g. /opt/qt6 or /opt/vtr-tools/qt6), set
#       VTR_QT_PREFIX accordingly and re-run.
#
# After the script finishes, source the per-shell environment helper it
# prints before building or running tests.

set -e

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
)

# qt6-shadertools-dev is only available on Ubuntu Noble (24.04+); on
# Jammy (22.04) it is missing from apt and the build instead relies on
# the qtshadertools module installed by aqt (see VTR_QT_PREFIX path).
if apt-cache show qt6-shadertools-dev >/dev/null 2>&1; then
    packages_to_install+=(qt6-shadertools-dev)
fi

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

# ---------------------------------------------------------------------------
# System packages (apt) — skippable on hosts without sudo.
# ---------------------------------------------------------------------------
if [ "${VTR_SKIP_APT:-0}" = "1" ]; then
    echo "VTR_SKIP_APT=1: skipping 'sudo apt-get install'."
    echo "  The packages listed above are assumed to be already present."
else
    sudo apt-get update
    sudo apt-get install -y "${packages_to_install[@]}"
fi

# ---------------------------------------------------------------------------
# Qt 6.9.3 via aqtinstall.
# 6.9.3 is the minimum required version: earlier Qt6 releases have internal
# bugs in the QRhi subsystem that cause rendering failures on our targets.
# Install root is configurable via VTR_QT_PREFIX (default /opt/qt6); when
# the root (or its parent) is writable by the current user the install
# runs without sudo, which is what enables the no-admin path.
# ---------------------------------------------------------------------------
QT_PREFIX="${VTR_QT_PREFIX:-/opt/qt6}"
QT_PARENT="$(dirname "${QT_PREFIX}")"

if [ -w "${QT_PREFIX}" ] || { [ ! -e "${QT_PREFIX}" ] && [ -w "${QT_PARENT}" ]; }; then
    AQT_SUDO=""
    PIP_SUDO=""
    PIP_USER_FLAG="--user"
else
    AQT_SUDO="sudo"
    PIP_SUDO="sudo"
    PIP_USER_FLAG=""
fi

# Install aqtinstall in an isolated venv so it never collides with the
# project venv (and so the no-sudo path does not need pip --user).
AQT_VENV="${QT_PARENT}/aqt-venv"
if [ ! -x "${AQT_VENV}/bin/aqt" ]; then
    if [ -n "${AQT_SUDO}" ]; then
        ${AQT_SUDO} python3 -m venv "${AQT_VENV}"
        ${AQT_SUDO} "${AQT_VENV}/bin/pip" install --upgrade pip
        ${AQT_SUDO} "${AQT_VENV}/bin/pip" install aqtinstall
    else
        python3 -m venv "${AQT_VENV}"
        "${AQT_VENV}/bin/pip" install --upgrade pip
        "${AQT_VENV}/bin/pip" install aqtinstall
    fi
fi

if [ ! -x "${QT_PREFIX}/6.9.3/gcc_64/bin/qmake" ]; then
    ${AQT_SUDO} "${AQT_VENV}/bin/aqt" install-qt linux desktop 6.9.3 \
        linux_gcc_64 --outputdir "${QT_PREFIX}" --modules qtshadertools
else
    echo "Qt 6.9.3 already present at ${QT_PREFIX}/6.9.3/gcc_64 — skipping aqt install."
fi

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
echo "  Qt 6.9.3 is at: ${QT_PREFIX}/6.9.3/gcc_64"
echo ""
echo "  Before building or running GUI tests in any new shell:"
echo "    export QT6_HOME=\"${QT_PREFIX}/6.9.3/gcc_64\""
echo "    export PATH=\"\$QT6_HOME/bin:\$PATH\""
echo "    export LD_LIBRARY_PATH=\"\$QT6_HOME/lib:\${LD_LIBRARY_PATH:-}\""
echo "    export CMAKE_PREFIX_PATH=\"\$QT6_HOME:\${CMAKE_PREFIX_PATH:-}\""
echo "    export QT_PLUGIN_PATH=\"\$QT6_HOME/plugins\""
echo "    source ${VENV_DIR}/bin/activate"
echo "====================================================================="
