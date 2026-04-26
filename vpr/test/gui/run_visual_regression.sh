#!/usr/bin/env bash
# Layer 5 — Visual Regression Test Runner
#
# Runs VPR to produce current images, then compares each against the
# corresponding golden image using SSIM (compare_images.py).
#
# Usage:
#   ./run_visual_regression.sh <vpr_binary> <arch_dir> <bench_dir> [golden_dir] [threshold]
#
# Exit: 0 if all comparisons pass, 1 otherwise.

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly VPR="$(cd "$(dirname "${1:?Usage: $0 <vpr_binary> <arch_dir> <bench_dir> [golden_dir] [threshold]}")" && pwd)/$(basename "$1")"
readonly ARCH_DIR="$(cd "${2:?}" && pwd)"
readonly BENCH_DIR="$(cd "${3:?}" && pwd)"
readonly GOLDEN_DIR="${4:-${SCRIPT_DIR}/golden}"
readonly THRESHOLD="${5:-0.999}"
readonly ARCH="${ARCH_DIR}/k6_N10_40nm.xml"
readonly COMPARE="${SCRIPT_DIR}/compare_images.py"

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

    if python3 "${COMPARE}" "${golden}" "${current}" --threshold "${THRESHOLD}"; then
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
