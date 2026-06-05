#!/bin/bash
#
# Install all build dependencies for VTR:
#   * system packages via apt          (install_apt_packages.sh)
#   * a Qt6 SDK >= the supported floor  (dev/ensure_qt6_sdk.sh)
#
# dev/ensure_qt6_sdk.sh is system-first: if the apt-installed Qt6 already
# satisfies the >= floor it does nothing; otherwise it provisions a private
# copy via aqt (no root) into a repo-local prefix.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"${SCRIPT_DIR}/install_apt_packages.sh"
"${SCRIPT_DIR}/dev/ensure_qt6_sdk.sh"
