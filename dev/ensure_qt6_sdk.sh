#!/bin/bash
#
# Thin shim — the real logic lives in ensure_qt6_sdk.py (single source of
# truth). This wrapper is kept so existing callers (Makefile, CI jobs, the
# Dockerfile) that invoke the .sh path keep working unchanged. exec replaces
# this process so the Python script's exit code propagates directly.
#
# See ensure_qt6_sdk.py for what this does and the supported environment
# variables (VTR_QT_VERSION, VTR_QT_PREFIX, VTR_AQT_VERSION).
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "${SCRIPT_DIR}/ensure_qt6_sdk.py" "$@"
