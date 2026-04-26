#!/usr/bin/env bash
# Layer 1 — Headless Smoke Tests
#
# Validates that VPR's graphics subsystem initialises and runs without
# segfaults under Qt offscreen rendering.
#
# Usage:
#   ./run_headless_smoke.sh <vpr_binary> <arch_dir> <bench_dir> [work_dir]
#
# Exit: 0 if all tests pass, 1 otherwise.

set -euo pipefail

readonly VPR="$(cd "$(dirname "${1:?Usage: $0 <vpr_binary> <arch_dir> <bench_dir> [work_dir]}")" && pwd)/$(basename "$1")"
readonly ARCH_DIR="$(cd "${2:?}" && pwd)"
readonly BENCH_DIR="$(cd "${3:?}" && pwd)"
readonly WORK_DIR="${4:-$(mktemp -d)}"
readonly ARCH="${ARCH_DIR}/k6_N10_40nm.xml"

export QT_SCALE_FACTOR=1

PASS=0
FAIL=0
XFAIL=0

run_test() {
    local name="$1"; shift
    local desc="$1"; shift
    local expect_fail="${1:-no}"; shift
    echo "--- [SMOKE] ${name}: ${desc}"
    local run_dir="${WORK_DIR}/${name}_run"
    mkdir -p "${run_dir}"
    if (cd "${run_dir}" && "$@") > "${WORK_DIR}/${name}.log" 2>&1; then
        echo "    PASS (exit 0)"
        (( PASS++ )) || true
    else
        local rc=$?
        if [[ "${expect_fail}" == "xfail" ]]; then
            echo "    XFAIL (exit ${rc}) — known issue, see defect log"
            (( XFAIL++ )) || true
        else
            echo "    FAIL (exit ${rc})"
            echo "    Log: ${WORK_DIR}/${name}.log"
            (( FAIL++ )) || true
        fi
    fi
}

check_file() {
    local name="$1"
    local filepath="$2"
    local expect_fail="${3:-no}"
    if [[ -f "${filepath}" && -s "${filepath}" ]]; then
        echo "    PASS: ${filepath} exists ($(wc -c < "${filepath}") bytes)"
        (( PASS++ )) || true
    else
        if [[ "${expect_fail}" == "xfail" ]]; then
            echo "    XFAIL: ${filepath} missing — known issue (DEF-004)"
            (( XFAIL++ )) || true
        else
            echo "    FAIL: ${filepath} missing or empty"
            (( FAIL++ )) || true
        fi
    fi
}

echo "=== Layer 1: Headless Smoke Tests ==="
echo "    VPR:       ${VPR}"
echo "    Arch:      ${ARCH}"
echo "    Benchmarks:${BENCH_DIR}"
echo "    Work dir:  ${WORK_DIR}"
echo ""

# ---- Test 1: --disp off (NO_GRAPHICS code path) ----------------------------
run_test "smoke_disp_off" "VPR runs with --disp off" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/and.blif" \
    --disp off --pack --place --seed 1

# ---- Test 2: graphics init + immediate exit --------------------------------
# Exercises Qt offscreen graphics initialisation without rendering or saving.
run_test "smoke_gfx_init_exit" "graphics init + exit 0" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/and.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "exit 0"

# ---- Test 3: set draw-state commands (no save) -----------------------------
# Exercises draw-state setters: set_nets, set_draw_block_internals,
# set_draw_block_text, set_draw_block_outlines, set_macros — then exits.
run_test "smoke_draw_state" "draw-state commands without save" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "set_nets 1; set_draw_block_internals 2; set_draw_block_text 0; set_draw_block_outlines 1; set_macros 1; exit 0"

# ---- Test 4: routing + draw-state commands ---------------------------------
# Full P&R flow with routing draw-state changes (congestion, routing_util, cpd).
run_test "smoke_route_draw_state" "route + congestion/cpd/util draw-state" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --route --seed 1 \
    --graphics_commands "set_congestion 1; set_routing_util 1; set_cpd 1; set_draw_net_max_fanout 10; exit 0"

# ---- Test 5: different benchmark (and_latch) P&R ---------------------------
# Validates a sequential circuit with a latch completes pack/place/route.
run_test "smoke_and_latch_pnr" "and_latch P&R with --disp off" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/and_latch.blif" \
    --disp off --pack --place --route --seed 1

# ---- Test 6: pack-only (no placement or routing) ---------------------------
run_test "smoke_pack_only" "pack-only with --disp off" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_2x2.blif" \
    --disp off --pack --seed 1

# ---- Test 7: offscreen graphics + save_graphics ----------------------------
# XFAIL: DEF-004 — print_png fails in Qt offscreen mode (QPainter not active)
run_test "smoke_save_png" "offscreen graphics init + PNG export" "xfail" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "save_graphics smoke_output.png; exit 0"

echo "    Checking PNG output..."
check_file "smoke_save_png_file" "${WORK_DIR}/smoke_save_png_run/smoke_output.png" "xfail"

# ---- Test 8: placement + routing + save_graphics ---------------------------
# XFAIL: DEF-004 — print_png fails in Qt offscreen mode (QPainter not active)
run_test "smoke_route_save" "placement + routing + PNG export" "xfail" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --route --seed 1 \
    --graphics_commands "save_graphics smoke_routing.png; exit 0"

echo "    Checking PNG output..."
check_file "smoke_route_save_file" "${WORK_DIR}/smoke_route_save_run/smoke_routing.png" "xfail"

# ---- Summary ---------------------------------------------------------------
echo ""
echo "=== Smoke Test Summary ==="
echo "    Passed: ${PASS}"
echo "    Failed: ${FAIL}"
echo "    XFail:  ${XFAIL}"
echo "    Work:   ${WORK_DIR}"

if [[ "${FAIL}" -gt 0 ]]; then
    echo "    RESULT: FAIL"
    exit 1
fi

echo "    RESULT: PASS"
exit 0
