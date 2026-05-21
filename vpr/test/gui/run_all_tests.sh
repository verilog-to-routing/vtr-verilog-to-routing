#!/usr/bin/env bash
# run_all_tests.sh — Orchestrates all VPR GUI test layers
#
# Runs Layer 3 (Catch2), Layer 1 (Smoke), and Layer 5 (Visual Regression)
# in sequence. Reports a unified pass/fail summary.
#
# Usage:
#   ./run_all_tests.sh                                          # use defaults below
#   ./run_all_tests.sh [--debug] <vpr_binary> <arch_dir> <bench_dir>
#
# Options:
#   --debug, -d   Also write a [golden | current | diff] triptych PNG for
#                 every visual-regression case (not just failures). Without
#                 this flag, the triptych is written only when SSIM fails.
#
# Layer 5 sweeps the matrix of cases × {rhi, immediate, deferred} renderers.
# Rendered output PNGs and triptychs (when written) are kept under
# build/vpr/test/gui/artifacts/<renderer>/. The whole artifacts/ dir is wiped
# at the start of every Layer-5 run so stale PNGs from a previous session
# can't be mistaken for the current one; the parent dir (which holds the
# cmake build tree) is left untouched.
#
# Defaults (resolved relative to the repo root containing this script):
#   <vpr_binary> = build/vpr/vpr
#   <arch_dir>   = vtr_flow/arch/timing
#   <bench_dir>  = vtr_flow/benchmarks/microbenchmarks
#
# Or from the build directory:
#   cd build/vpr/test/gui && ctest --output-on-failure
#
# Exit: 0 if all layers pass, 1 otherwise.

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

DEBUG=0
POSITIONAL=()
for arg in "$@"; do
    case "${arg}" in
        --debug|-d) DEBUG=1 ;;
        --help|-h)
            sed -n '2,29p' "$0"
            exit 0
            ;;
        *) POSITIONAL+=("${arg}") ;;
    esac
done
set -- "${POSITIONAL[@]+"${POSITIONAL[@]}"}"
export VPR_GUI_DEBUG="${DEBUG}"

VPR_ARG="${1:-${REPO_ROOT}/build/vpr/vpr}"
ARCH_ARG="${2:-${REPO_ROOT}/vtr_flow/arch/timing}"
BENCH_ARG="${3:-${REPO_ROOT}/vtr_flow/benchmarks/microbenchmarks}"

readonly VPR="$(cd "$(dirname "${VPR_ARG}")" && pwd)/$(basename "${VPR_ARG}")"
readonly ARCH_DIR="$(cd "${ARCH_ARG}" && pwd)"
readonly BENCH_DIR="$(cd "${BENCH_ARG}" && pwd)"

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
echo "  Artifacts:   ${REPO_ROOT}/build/vpr/test/gui/artifacts/"
if [[ "${DEBUG}" -eq 1 ]]; then
    echo "  Debug mode:  on — triptych diffs written for every case"
else
    echo "  Debug mode:  off — triptych diffs only on SSIM failure"
fi

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
