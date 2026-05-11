#!/usr/bin/env bash
# Layer 5 — Visual Regression Test Runner
#
# Runs VPR twice (same two passes as generate_goldens.sh: one for
# placement_done + routing_done overlays, one for routing_initial
# congestion) to produce the current images, then compares each named
# case against the corresponding golden image using SSIM
# (compare_images.py).
#
# Usage:
#   ./run_visual_regression.sh                                              # use defaults below
#   ./run_visual_regression.sh <vpr_binary> <arch_dir> <bench_dir> [golden_dir] [threshold]
#
# Defaults (resolved relative to the repo root containing this script):
#   <vpr_binary> = build/vpr/vpr
#   <arch_dir>   = vtr_flow/arch/timing
#   <bench_dir>  = vtr_flow/benchmarks/microbenchmarks
#   [golden_dir] = vpr/test/gui/golden  (next to this script). When the
#                  caller does NOT pass an explicit dir, the runner first
#                  looks for a platform-specific subdirectory
#                  (golden/linux-x86_64/, golden/darwin-arm64/, ...) and
#                  falls back to the shared dir if none exists. This keeps
#                  per-platform AA / font-hinting drift from being a test
#                  failure.
#   [threshold]  = $VPR_GUI_SSIM_THRESHOLD or 0.98
#                  0.98 catches real regressions while tolerating sub-pixel
#                  drift between Qt minor versions / platforms. Override
#                  via env var or positional arg when running locally
#                  against a single-platform corpus (e.g. 0.999 for a
#                  same-host self-comparison).
#
# Exit: 0 if all comparisons pass, 1 otherwise.

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

# CI offscreen Mesa renders differ from goldens captured on developer hosts
# (different GL implementation, AA, font hinting). Skip under CI=true so
# the SSIM regression gate only runs against same-host goldens. Exit 2 is
# honoured by ctest's SKIP_RETURN_CODE on test_vpr_gui_visual.
if [[ "${CI:-}" == "true" ]]; then
    echo "SKIP: CI offscreen Mesa cannot be compared against developer-host goldens" >&2
    exit 2
fi

VPR_ARG="${1:-${REPO_ROOT}/build/vpr/vpr}"
ARCH_ARG="${2:-${REPO_ROOT}/vtr_flow/arch/timing}"
BENCH_ARG="${3:-${REPO_ROOT}/vtr_flow/benchmarks/microbenchmarks}"

readonly VPR="$(cd "$(dirname "${VPR_ARG}")" && pwd)/$(basename "${VPR_ARG}")"
readonly ARCH_DIR="$(cd "${ARCH_ARG}" && pwd)"
readonly BENCH_DIR="$(cd "${BENCH_ARG}" && pwd)"

# Platform-aware golden lookup: golden/<os>-<arch>/ if it exists, else the
# shared golden/ directory. Caller can still override with $4.
uname_os="$(uname -s | tr '[:upper:]' '[:lower:]')"
uname_arch="$(uname -m)"
_default_shared="${SCRIPT_DIR}/golden"
_default_platform="${SCRIPT_DIR}/golden/${uname_os}-${uname_arch}"
if [[ -z "${4:-}" && -d "${_default_platform}" \
      && -n "$(ls -A "${_default_platform}"/*.png 2>/dev/null)" ]]; then
    GOLDEN_DIR="${_default_platform}"
else
    GOLDEN_DIR="${4:-${_default_shared}}"
fi
readonly GOLDEN_DIR

readonly THRESHOLD="${5:-${VPR_GUI_SSIM_THRESHOLD:-0.98}}"
readonly ARCH="${ARCH_DIR}/k6_N10_40nm.xml"
readonly BENCH="${BENCH_DIR}/mult_4x4.blif"
readonly COMPARE="${SCRIPT_DIR}/compare_images.py"

# Shared case list + graphics_commands strings + visual_run_pass helper.
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/visual_cases.sh"

# Up-front validation
require_file() {
    local label="$1" path="$2"
    if [[ ! -e "${path}" ]]; then
        echo "ERROR: ${label} not found: ${path}" >&2
        exit 2
    fi
}
require_file "VPR binary" "${VPR}"
require_file "architecture" "${ARCH}"
require_file "benchmark" "${BENCH}"
require_file "compare_images.py" "${COMPARE}"

# Resolve a Python interpreter that has skimage/PIL/numpy available.
# `make gui-tests` invokes this script with whatever PATH the build was
# launched from, which often is the system python lacking scikit-image.
# Prefer (in order): $VPR_GUI_PYTHON, repo .venv, an env-active VIRTUAL_ENV,
# any python3/python on PATH that can import skimage. Fail fast with one
# actionable message if none works.
resolve_python() {
    local candidates=()
    [[ -n "${VPR_GUI_PYTHON:-}" ]] && candidates+=("${VPR_GUI_PYTHON}")
    [[ -x "${REPO_ROOT}/.venv/bin/python3" ]] && candidates+=("${REPO_ROOT}/.venv/bin/python3")
    [[ -n "${VIRTUAL_ENV:-}" && -x "${VIRTUAL_ENV}/bin/python3" ]] && candidates+=("${VIRTUAL_ENV}/bin/python3")
    candidates+=(python3 python)
    for cand in "${candidates[@]}"; do
        local resolved
        resolved="$(command -v "${cand}" 2>/dev/null || true)"
        [[ -z "${resolved}" ]] && continue
        if "${resolved}" -c 'import skimage, PIL, numpy' >/dev/null 2>&1; then
            echo "${resolved}"
            return 0
        fi
    done
    return 1
}
if ! PYTHON="$(resolve_python)"; then
    echo "ERROR: No Python interpreter with scikit-image / Pillow / numpy found." >&2
    echo "       Tried: \$VPR_GUI_PYTHON, ${REPO_ROOT}/.venv/bin/python3, \$VIRTUAL_ENV, python3, python." >&2
    echo "       Install with:  python3 -m pip install scikit-image Pillow numpy" >&2
    echo "       Or point at one explicitly:  VPR_GUI_PYTHON=/path/to/python make gui-tests" >&2
    exit 2
fi
readonly PYTHON

export QT_SCALE_FACTOR=1

TEST_TMPDIR=$(mktemp -d)
trap 'rm -rf "${TEST_TMPDIR}"' EXIT

PASS=0
FAIL=0
SKIP=0

# Check golden directory
if [[ ! -d "${GOLDEN_DIR}" ]] || [[ -z "$(ls -A "${GOLDEN_DIR}"/*.png 2>/dev/null)" ]]; then
    echo "WARNING: No golden images in ${GOLDEN_DIR}"
    echo "Run generate_goldens.sh first (requires DEF-004 to be fixed)."
    echo ""
    echo "=== Visual Regression Summary ==="
    echo "    Passed:  0"
    echo "    Failed:  0"
    echo "    Skipped: ALL (no golden images — blocked by DEF-004)"
    echo "    RESULT: SKIP"
    exit 2
fi

echo "=== Layer 5: Visual Regression Tests ==="
echo "    VPR:       ${VPR}"
echo "    Bench:     ${BENCH}"
echo "    Goldens:   ${GOLDEN_DIR}"
echo "    Python:    ${PYTHON}"
echo "    Threshold: ${THRESHOLD}"
echo "    Cases:     ${#VISUAL_CASE_NAMES[@]}"
echo ""

# --- Pass 1: placement_done + routing_done overlays --------------------------
PASS1_WORK="${TEST_TMPDIR}/pass1"
echo "--- Pass 1: placement + routing overlays"
visual_run_pass "${VPR}" "${ARCH}" "${BENCH}" "${PASS1_WORK}" \
    "${PASS1_CMDS}" --pack --place --route

# --- Pass 2: routing_initial congestion --------------------------------------
PASS2_WORK="${TEST_TMPDIR}/pass2"
echo "--- Pass 2: routing_initial congestion"
visual_run_pass "${VPR}" "${ARCH}" "${BENCH}" "${PASS2_WORK}" \
    "${PASS2_CMDS}" --pack --place --route

# --- Compare each named case against its golden ------------------------------
echo ""
echo "--- Comparing ${#VISUAL_CASE_NAMES[@]} cases against goldens"
for name in "${VISUAL_CASE_NAMES[@]}"; do
    echo "--- [VISUAL] ${name}"

    local_golden="${GOLDEN_DIR}/${name}.png"
    if [[ ! -f "${local_golden}" ]]; then
        echo "    SKIP: no golden image"
        (( SKIP++ )) || true
        continue
    fi

    local_current=""
    if [[ -f "${PASS1_WORK}/${name}.png" ]]; then
        local_current="${PASS1_WORK}/${name}.png"
    elif [[ -f "${PASS2_WORK}/${name}.png" ]]; then
        local_current="${PASS2_WORK}/${name}.png"
    else
        echo "    FAIL: ${name}.png not produced by either pass"
        echo "    See: ${PASS1_WORK}/vpr.log, ${PASS2_WORK}/vpr.log"
        (( FAIL++ )) || true
        continue
    fi

    if "${PYTHON}" "${COMPARE}" "${local_golden}" "${local_current}" --threshold "${THRESHOLD}"; then
        (( PASS++ )) || true
    else
        (( FAIL++ )) || true
        echo "    Golden:  ${local_golden}"
        echo "    Current: ${local_current}"
    fi
done

# ---- Summary ---------------------------------------------------------------
echo ""
echo "=== Visual Regression Summary ==="
echo "    Passed:  ${PASS}"
echo "    Failed:  ${FAIL}"
echo "    Skipped: ${SKIP}"

if [[ "${FAIL}" -gt 0 ]]; then
    echo "    RESULT: FAIL"
    exit 1
fi

echo "    RESULT: PASS"
exit 0
