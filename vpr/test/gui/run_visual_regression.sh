#!/usr/bin/env bash
# Layer 5 — Visual Regression Test Runner
#
# Runs VPR to produce current images, then compares each against the
# corresponding golden image using SSIM (compare_images.py).
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
readonly COMPARE="${SCRIPT_DIR}/compare_images.py"

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
require_file "benchmark" "${BENCH_DIR}/mult_4x4.blif"
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

# Helper: run VPR, produce current image, compare against golden
regression_test() {
    local name="$1"; shift
    local circuit="$1"; shift
    local vpr_flags=("$@")

    echo "--- [VISUAL] ${name}"

    local golden="${GOLDEN_DIR}/${name}.png"
    if [[ ! -f "${golden}" ]]; then
        echo "    SKIP: no golden image"
        (( SKIP++ )) || true
        return
    fi

    local work="${TEST_TMPDIR}/${name}"
    mkdir -p "${work}"

    if ! (cd "${work}" && "${VPR}" "${ARCH}" "${BENCH_DIR}/${circuit}" \
        --disp off --seed 1 \
        "${vpr_flags[@]}") \
        > "${work}/vpr.log" 2>&1; then
        echo "    FAIL: VPR exited with error"
        (( FAIL++ )) || true
        return
    fi

    local current="${work}/${name}.png"
    if [[ ! -f "${current}" ]]; then
        echo "    FAIL: ${name}.png not generated"
        (( FAIL++ )) || true
        return
    fi

    if "${PYTHON}" "${COMPARE}" "${golden}" "${current}" --threshold "${THRESHOLD}"; then
        (( PASS++ )) || true
    else
        (( FAIL++ )) || true
        echo "    Golden: ${golden}"
        echo "    Current: ${current}"
    fi
}

echo "=== Layer 5: Visual Regression Tests ==="
echo "    VPR:       ${VPR}"
echo "    Goldens:   ${GOLDEN_DIR}"
echo "    Python:    ${PYTHON}"
echo "    Threshold: ${THRESHOLD}"
echo ""

# 1. placement_default
regression_test "placement_default" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "save_graphics placement_default.png; exit 0"

# 2. placement_nets
regression_test "placement_nets" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "set_nets 1; save_graphics placement_nets.png; exit 0"

# 3. placement_congestion
regression_test "placement_congestion" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "set_congestion 1; save_graphics placement_congestion.png; exit 0"

# 4. routing_default
regression_test "routing_default" "mult_4x4.blif" \
    --pack --place --route \
    --graphics_commands "save_graphics routing_default.png; exit 0"

# 5. routing_timing
regression_test "routing_timing" "mult_4x4.blif" \
    --pack --place --route \
    --graphics_commands "set_cpd 1; save_graphics routing_timing.png; exit 0"

# 6. placement_block_internals
regression_test "placement_block_internals" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "set_draw_block_internals 2; save_graphics placement_block_internals.png; exit 0"

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
