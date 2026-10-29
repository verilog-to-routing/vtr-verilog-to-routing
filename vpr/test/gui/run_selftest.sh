#!/usr/bin/env bash
# ctest entry point for the layer-5 comparator self-test.
#
# Exists so the self-test resolves its interpreter the same way the two layer-5
# runners do. Invoking `python3` directly would skip the test on any host whose
# deps live in a virtualenv -- which is VTR's documented setup, and would mean
# the test that guards the gate quietly never runs.
#
# Exit: 0 pass, 1 fail, 2 skipped (ctest SKIP_RETURN_CODE).

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=find_gui_python.sh
source "${SCRIPT_DIR}/find_gui_python.sh"

if ! PYTHON="$(resolve_gui_python)"; then
    echo "SKIP: no Python interpreter with scikit-image / Pillow / numpy found." >&2
    gui_python_hint
    exit 2
fi

exec "${PYTHON}" "${SCRIPT_DIR}/test_compare_images.py"
