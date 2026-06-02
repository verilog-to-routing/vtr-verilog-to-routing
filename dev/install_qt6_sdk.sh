#!/bin/bash
#
# Provision the Qt6 SDK that VTR's GUI build links against, with NO root.
#
# Qt 6.9.3 is the minimum required version: earlier Qt6 releases have internal
# bugs in the QRhi subsystem that cause rendering failures on our targets.
# Ubuntu's apt typically ships an older Qt6, so this script installs a private
# copy via `aqt` (aqtinstall) into a user-writable, repo-local prefix.
#
# This is the single implementation shared by:
#   * `make qt6sdk`            (the convenience target)
#   * install_apt_packages.sh  (the dependency installer, when system Qt is old)
#
# Configuration (environment variables):
#
#   VTR_QT_PREFIX=/path/to/qt-install-root
#       Install root. Default: "<repo>/qt6". aqt installs the SDK under
#       "${VTR_QT_PREFIX}/<version>/gcc_64". The prefix (or its parent) must be
#       writable by the current user — this script never uses sudo.
#
#   VTR_QT_VERSION=6.9.3
#       Qt version to install. Default 6.9.3 (the supported floor).
#
# Idempotent: if the SDK is already present it does nothing.

set -e

QT_VERSION="${VTR_QT_VERSION:-6.9.3}"

# Repo root is the parent of this script's dev/ directory.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "${SCRIPT_DIR}")"

QT_PREFIX="${VTR_QT_PREFIX:-${REPO_ROOT}/qt6}"
QT_HOME="${QT_PREFIX}/${QT_VERSION}/gcc_64"

# ---------------------------------------------------------------------------
# Idempotency: already installed?
# ---------------------------------------------------------------------------
if [ -x "${QT_HOME}/bin/qmake" ]; then
    echo "Qt ${QT_VERSION} already present at ${QT_HOME} — nothing to do."
    exit 0
fi

# ---------------------------------------------------------------------------
# Root-free precondition: the install root must be user-writable.
# ---------------------------------------------------------------------------
mkdir -p "${QT_PREFIX}" 2>/dev/null || true
if [ ! -w "${QT_PREFIX}" ]; then
    echo "ERROR: ${QT_PREFIX} is not writable by $(id -un)." >&2
    echo "       This script does not use sudo. Choose a writable location via" >&2
    echo "       VTR_QT_PREFIX=/some/writable/dir, or fix the permissions." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# aqtinstall in an isolated venv (kept beside the SDK, never on PATH so it
# cannot collide with the project venv). The "*/gcc_64" CMake glob ignores it.
# ---------------------------------------------------------------------------
# aqtinstall is pinned to 3.3.0 for reproducible Qt installs across machines.
AQT_VERSION="${VTR_AQT_VERSION:-3.3.0}"
AQT_VENV="${QT_PREFIX}/aqt-venv"
if [ ! -x "${AQT_VENV}/bin/aqt" ]; then
    echo "Installing aqtinstall==${AQT_VERSION} into ${AQT_VENV}..."
    python3 -m venv "${AQT_VENV}"
    "${AQT_VENV}/bin/pip" install --upgrade pip
    "${AQT_VENV}/bin/pip" install "aqtinstall==${AQT_VERSION}"
fi

# ---------------------------------------------------------------------------
# Install Qt. qtshadertools is required by the QRhi shader pipeline.
# ---------------------------------------------------------------------------
echo "Installing Qt ${QT_VERSION} into ${QT_PREFIX} via aqt (no sudo)..."
"${AQT_VENV}/bin/aqt" install-qt linux desktop "${QT_VERSION}" \
    linux_gcc_64 --outputdir "${QT_PREFIX}" --modules qtshadertools

echo "Qt ${QT_VERSION} installed at ${QT_HOME}"
