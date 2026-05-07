#!/usr/bin/env bash
# Layer 1 — Headless Smoke Tests
#
# Validates that VPR's graphics subsystem initialises and runs without
# segfaults under Qt offscreen rendering.
#
# Usage:
#   ./run_headless_smoke.sh                                          # use defaults below
#   ./run_headless_smoke.sh <vpr_binary> <arch_dir> <bench_dir> [work_dir]
#
# Defaults (resolved relative to the repo root containing this script):
#   <vpr_binary> = build/vpr/vpr
#   <arch_dir>   = vtr_flow/arch/timing
#   <bench_dir>  = vtr_flow/benchmarks/microbenchmarks
#   [work_dir]   = $(mktemp -d)
#
# Exit: 0 if all tests pass, 1 otherwise.

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

VPR_ARG="${1:-${REPO_ROOT}/build/vpr/vpr}"
ARCH_ARG="${2:-${REPO_ROOT}/vtr_flow/arch/timing}"
BENCH_ARG="${3:-${REPO_ROOT}/vtr_flow/benchmarks/microbenchmarks}"

readonly VPR="$(cd "$(dirname "${VPR_ARG}")" && pwd)/$(basename "${VPR_ARG}")"
readonly ARCH_DIR="$(cd "${ARCH_ARG}" && pwd)"
readonly BENCH_DIR="$(cd "${BENCH_ARG}" && pwd)"
readonly WORK_DIR="${4:-$(mktemp -d)}"
readonly ARCH="${ARCH_DIR}/k6_N10_40nm.xml"

export QT_SCALE_FACTOR=1

# Up-front validation — fail loudly with one clear line instead of letting
# VPR error out 8 times with the same root cause buried in vpr.log.
require_file() {
    local label="$1" path="$2"
    if [[ ! -e "${path}" ]]; then
        echo "ERROR: ${label} not found: ${path}" >&2
        exit 2
    fi
}
require_file "VPR binary" "${VPR}"
require_file "architecture" "${ARCH}"
for bench in and.blif and_latch.blif mult_2x2.blif mult_4x4.blif; do
    require_file "benchmark" "${BENCH_DIR}/${bench}"
done

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
    if [[ "${SHOW_CMD:-0}" == "1" ]]; then
        printf 'VPR CMD:\n'
        printf '%q' "$1"
        printf ' %q' "${@:2}"
        printf '\n'
    fi
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

# Inverse of run_test: PASS only when VPR exits non-zero. Used by negative
# parser cases (GUI-T-004) where the contract is "the unknown-token branch in
# run_graphics_commands raises VPR_ERROR_DRAW and aborts".
run_test_expect_nonzero() {
    local name="$1"; shift
    local desc="$1"; shift
    echo "--- [SMOKE] ${name}: ${desc} (expect non-zero exit)"
    local run_dir="${WORK_DIR}/${name}_run"
    mkdir -p "${run_dir}"
    if (cd "${run_dir}" && "$@") > "${WORK_DIR}/${name}.log" 2>&1; then
        echo "    FAIL (exited 0 but a non-zero exit was contracted)"
        echo "    Log: ${WORK_DIR}/${name}.log"
        (( FAIL++ )) || true
    else
        local rc=$?
        echo "    PASS (exit ${rc} as contracted)"
        (( PASS++ )) || true
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
run_test "smoke_save_png" "offscreen graphics init + PNG export" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "save_graphics smoke_output.png; exit 0"

echo "    Checking PNG output..."
check_file "smoke_save_png_file" "${WORK_DIR}/smoke_save_png_run/smoke_output.png" "no"

# ---- Test 8: placement + routing + save_graphics ---------------------------
run_test "smoke_route_save" "placement + routing + PNG export" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --route --seed 1 \
    --graphics_commands "save_graphics smoke_routing.png; exit 0"

echo "    Checking PNG output..."
check_file "smoke_route_save_file" "${WORK_DIR}/smoke_route_save_run/smoke_routing.png" "no"

# ===========================================================================
# GUI-T-004 — graphics_commands parser semantics
#
# Closes the "happy paths only" gap recorded in
# doc/src/dev/vpr_gui_complete_test_plan.rst §12.2:
#   * set_clip_routing_util — implemented but no scenario currently drives it.
#   * {i} substitution — vtr::replace_all("{i}", sequence_number) in
#     run_graphics_commands has no behavioural test pinning the substitution.
#   * Multi-token sequencing — most existing scenarios issue ≤ 2 tokens; the
#     for-loop in run_graphics_commands is barely exercised.
#   * Unknown-token negative — the VPR_ERROR_DRAW "Unrecognized graphics
#     command" branch (draw.cpp run_graphics_commands) was previously
#     unreachable from any test; missing it lets a future typo or removed
#     token fail silently.
# ===========================================================================

# ---- T-004.a: set_clip_routing_util after route ----------------------------
# Drives the only currently-implemented setter no scenario invokes. Requires
# --route so the routing-util state is meaningful.
run_test "t004_clip_routing_util" "set_clip_routing_util after route" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --route --seed 1 \
    --graphics_commands "set_routing_util 1; set_clip_routing_util 1; exit 0"

# ---- T-004.b: {i} substitution -> "0" --------------------------------------
# A single --graphics_commands invocation runs with sequence_number == 0
# throughout the command list (it is incremented once at the end of
# run_graphics_commands). Therefore "out_{i}.png" must produce a file named
# literally "out_0.png" — NOT "out_{i}.png". This pins vtr::replace_all().
run_test "t004_substitution_i_to_zero" "save_graphics out_{i}.png -> out_0.png" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "save_graphics out_{i}.png; exit 0"

echo "    Checking substituted filename..."
check_file "t004_substitution_i_to_zero_file" \
    "${WORK_DIR}/t004_substitution_i_to_zero_run/out_0.png" "no"
# Negative companion: the un-substituted literal must NOT exist. If both
# files appear the substitution is being skipped silently.
if [[ -e "${WORK_DIR}/t004_substitution_i_to_zero_run/out_{i}.png" ]]; then
    echo "    FAIL: literal 'out_{i}.png' exists — {i} substitution did not run"
    (( FAIL++ )) || true
else
    echo "    PASS: literal 'out_{i}.png' absent — {i} substitution effective"
    (( PASS++ )) || true
fi

# ---- T-004.c: multi-token sequence dispatch --------------------------------
# Three setters in a single --graphics_commands string followed by save +
# exit. Verifies the for-loop in run_graphics_commands dispatches every
# token (a regression that drops a token mid-list would still produce a
# valid PNG today, but the log line "Unrecognized graphics command" or a
# missing setter echo would catch it on closer inspection).
run_test "t004_sequence_three" "set_nets + set_macros + set_cpd + save" "no" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/mult_4x4.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "set_nets 1; set_macros 1; set_cpd 1; save_graphics seq.png; exit 0"

echo "    Checking sequence PNG..."
check_file "t004_sequence_three_file" \
    "${WORK_DIR}/t004_sequence_three_run/seq.png" "no"
# Audit the log: each setter echoes the resulting state on its own line via
# VTR_LOG. We expect to see the three command tokens printed by the parser's
# "VTR_LOG("%s ", item)" loop. Absence of any of these is a silent regression.
seq_log="${WORK_DIR}/t004_sequence_three.log"
for tok in set_nets set_macros set_cpd save_graphics; do
    if grep -q -- "${tok}" "${seq_log}"; then
        echo "    PASS: parser echoed '${tok}' token"
        (( PASS++ )) || true
    else
        echo "    FAIL: parser did not echo '${tok}' (silent token drop)"
        echo "    Log: ${seq_log}"
        (( FAIL++ )) || true
    fi
done

# ---- T-004.d: unknown-token negative ---------------------------------------
# Per draw.cpp run_graphics_commands, an unrecognised command raises
# VPR_ERROR_DRAW("Unrecognized graphics command '...'"), which aborts.
# A future change that turns the unknown-token branch into a silent skip
# would cause this test to FAIL (because VPR would exit 0).
run_test_expect_nonzero "t004_unknown_token_neg" \
    "unknown command surfaces VPR_ERROR_DRAW" \
    "${VPR}" "${ARCH}" "${BENCH_DIR}/and.blif" \
    --disp off --pack --place --seed 1 \
    --graphics_commands "set_no_such_thing 1; exit 0"

# Confirm the error message is the contracted one (a different abort would
# match the non-zero exit too — narrow the contract to the actual branch).
neg_log="${WORK_DIR}/t004_unknown_token_neg.log"
if grep -q "Unrecognized graphics command" "${neg_log}"; then
    echo "    PASS: error message matches 'Unrecognized graphics command'"
    (( PASS++ )) || true
else
    echo "    FAIL: non-zero exit was for a different reason"
    echo "    Log: ${neg_log}"
    (( FAIL++ )) || true
fi

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
