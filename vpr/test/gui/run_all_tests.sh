#!/usr/bin/env bash
# run_all_tests.sh — Orchestrates all VPR GUI test layers
#
# Runs Layer 3 (Catch2), Layer 1 (Smoke), and Layer 5 (Visual Regression)
# in sequence. Reports a unified pass/fail summary.
#
# Usage:
#   ./run_all_tests.sh <vpr_binary> <arch_dir> <bench_dir>
#
# Or from the build directory:
#   cd build/vpr/test/gui && ctest --output-on-failure
#
# Exit: 0 if all layers pass, 1 otherwise.

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly VPR="$(cd "$(dirname "${1:?Usage: $0 <vpr_binary> <arch_dir> <bench_dir>}")" && pwd)/$(basename "$1")"
readonly ARCH_DIR="$(cd "${2:?}" && pwd)"
readonly BENCH_DIR="$(cd "${3:?}" && pwd)"

# Resolve test_vpr_gui binary — expect it next to vpr binary (../test/gui/)
# or in the same build tree.
readonly VPR_DIR="$(dirname "${VPR}")"
TEST_BIN=""
for candidate in \
    "${VPR_DIR}/test/gui/test_vpr_gui" \
    "${VPR_DIR}/../vpr/test/gui/test_vpr_gui" \
    "${VPR_DIR}/test_vpr_gui"; do
    if [[ -x "${candidate}" ]]; then
        TEST_BIN="${candidate}"
        break
    fi
done

# Required for test_vpr_gui (Catch2), which creates Qt directly without going
# through VPR's --disp flag. VPR itself sets this automatically when --disp is
# off, so scripts that only invoke VPR do not need it.
export QT_QPA_PLATFORM=offscreen
export QT_SCALE_FACTOR=1

LAYER_PASS=0
LAYER_FAIL=0
LAYER_SKIP=0

run_layer() {
    local layer="$1"
    local desc="$2"
    shift 2
    echo ""
    echo "================================================================"
    echo "  ${layer}: ${desc}"
    echo "================================================================"
    if "$@"; then
        echo ""
        echo "  >> ${layer}: PASS"
        (( LAYER_PASS++ )) || true
    else
        local rc=$?
        if [[ ${rc} -eq 2 ]]; then
            echo ""
            echo "  >> ${layer}: SKIP"
            (( LAYER_SKIP++ )) || true
        else
            echo ""
            echo "  >> ${layer}: FAIL (exit ${rc})"
            (( LAYER_FAIL++ )) || true
        fi
    fi
}

echo "================================================================"
echo "  VPR GUI Test Suite — All Layers"
echo "================================================================"
echo "  VPR:         ${VPR}"
echo "  Arch dir:    ${ARCH_DIR}"
echo "  Bench dir:   ${BENCH_DIR}"
echo "  Test binary: ${TEST_BIN:-NOT FOUND}"

# ---- Layer 3: Catch2 Integration Tests ------------------------------------
if [[ -n "${TEST_BIN}" ]]; then
    run_layer "Layer 3" "Catch2 Integration Tests" \
        "${TEST_BIN}" --colour-mode ansi
else
    echo ""
    echo "WARNING: test_vpr_gui binary not found — skipping Layer 3"
    echo "Build with: cmake --build build --target test_vpr_gui"
    (( LAYER_SKIP++ )) || true
fi

# ---- Layer 1: Headless Smoke Tests ----------------------------------------
run_layer "Layer 1" "Headless Smoke Tests" \
    bash "${SCRIPT_DIR}/run_headless_smoke.sh" \
        "${VPR}" "${ARCH_DIR}" "${BENCH_DIR}"

# ---- Layer 5: Visual Regression Tests -------------------------------------
run_layer "Layer 5" "Visual Regression Tests" \
    bash "${SCRIPT_DIR}/run_visual_regression.sh" \
        "${VPR}" "${ARCH_DIR}" "${BENCH_DIR}"

# ---- Unified Summary ------------------------------------------------------
echo ""
echo "================================================================"
echo "  UNIFIED SUMMARY"
echo "================================================================"
echo "    Layers passed:  ${LAYER_PASS}"
echo "    Layers failed:  ${LAYER_FAIL}"
echo "    Layers skipped: ${LAYER_SKIP}"
echo ""

if [[ "${LAYER_FAIL}" -gt 0 ]]; then
    echo "  OVERALL RESULT: FAIL"
    exit 1
fi

echo "  OVERALL RESULT: PASS"
exit 0
