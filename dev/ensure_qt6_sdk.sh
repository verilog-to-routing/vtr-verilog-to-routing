#!/bin/bash
#
# Ensure a Qt6 >= the supported floor is available for VTR's GUI build, with
# NO root.
#
# Qt 6.9.3 is the minimum required version: earlier Qt6 releases have internal
# bugs in the QRhi subsystem that cause rendering failures on our targets.
#
# Strategy:
#   1. If the SYSTEM already provides Qt6 >= ${VTR_QT_VERSION} (e.g. installed
#      via apt), do nothing — the system Qt is used.
#   2. Otherwise install a private copy via `aqt` (aqtinstall) into a
#      user-writable, repo-local prefix (no sudo).
#
# Invoked by the CI graphics jobs and the Dockerfile, after install_apt_packages.sh.
#
# Configuration (environment variables):
#
#   VTR_QT_PREFIX=/path/to/qt-install-root
#       Install root. Default: "<repo>/qt6". aqt installs the SDK under
#       "${VTR_QT_PREFIX}/<version>/gcc_64". The prefix (or its parent) must be
#       writable by the current user — this script never uses sudo.
#
#   VTR_QT_VERSION=6.9.3
#       Minimum/target Qt version. Default 6.9.3 (the supported floor). A system
#       Qt at or above this is accepted as-is; otherwise this exact version is
#       provisioned via aqt.
#
# Idempotent: if a suitable system Qt or the repo-local SDK is already present
# it does nothing.

set -e

QT_VERSION="${VTR_QT_VERSION:-6.9.3}"

# Repo root is the parent of this script's dev/ directory.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "${SCRIPT_DIR}")"

QT_PREFIX="${VTR_QT_PREFIX:-${REPO_ROOT}/qt6}"
QT_HOME="${QT_PREFIX}/${QT_VERSION}/gcc_64"

# ---------------------------------------------------------------------------
# Prefer a system Qt6 >= ${QT_VERSION}: if one is installed, no aqt is needed.
# ---------------------------------------------------------------------------
# version_ge A B  ->  success (0) if A >= B
version_ge() {
    [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]
}

# Detect a system Qt6 version (X.Y.Z) that CMake's find_package(Qt6) would
# actually use. We rely ONLY on the apt package qt6-base-dev, because it
# installs the Qt6 CMake config into /usr — a default find_package() search
# path. We deliberately do NOT consult `qmake6` or `pkg-config`: a qmake6 on
# PATH (or a tools-only / non-default install) can report a Qt that
# find_package() cannot find, which would make us wrongly skip aqt and leave
# the build unable to locate Qt6.
detect_system_qt6() {
    command -v dpkg-query >/dev/null 2>&1 || return 0
    dpkg-query -W -f='${Version}' qt6-base-dev 2>/dev/null \
        | grep -oE '^[0-9]+(\.[0-9]+){1,2}' | head -n1
}

SYS_QT_VERSION="$(detect_system_qt6)"
if [ -n "${SYS_QT_VERSION}" ] && version_ge "${SYS_QT_VERSION}" "${QT_VERSION}"; then
    echo "System Qt6 ${SYS_QT_VERSION} satisfies >= ${QT_VERSION} — using system Qt, skipping aqt."
    exit 0
fi
echo "System Qt6 (${SYS_QT_VERSION:-none found}) does not satisfy >= ${QT_VERSION}; provisioning via aqt..."

# ---------------------------------------------------------------------------
# Idempotency: repo-local SDK already installed?
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
