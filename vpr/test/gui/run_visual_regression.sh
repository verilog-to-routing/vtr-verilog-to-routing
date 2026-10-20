#!/usr/bin/env bash
# Layer 5 — Visual Regression Test Runner
#
# For each renderer in {rhi, immediate, deferred}, runs VPR twice under
# --analysis with the shared graphics_commands sequences from
# visual_cases.sh (one invocation against the clean fixture for the
# placement_done + routing_done overlays, one against the congested fixture
# for the routing_done congestion overlays) — passing --renderer <renderer>
# explicitly each time — then compares each emitted PNG against the
# matching per-renderer golden at vpr/test/gui/golden/<renderer>/<case>.png
# with an exact pixel comparison (compare_images.py).
#
# Goldens stay valid across VPR versions because --analysis runs only the
# analyzer (pack/place/route LOAD from disk), so different VPR heuristics
# no longer shift block locations or wire trees.
#
# Matrix: ${#VISUAL_CASE_NAMES[@]} cases × 3 renderers = total comparisons.
# Goldens are PER-RENDERER: each renderer is compared against its own
# baseline so legitimate cross-renderer differences (dash phase, 0.5px
# stroke shift, etc.) don't masquerade as regressions.
#
# Usage:
#   ./run_visual_regression.sh                                              # use defaults below
#   ./run_visual_regression.sh <vpr_binary> <arch_dir> <bench_dir> [golden_dir] [threshold]
#
# Defaults (resolved relative to the repo root containing this script):
#   <vpr_binary> = build/vpr/vpr
#   <arch_dir>   = vtr_flow/arch/timing
#   <bench_dir>  = vtr_flow/benchmarks/microbenchmarks
#   [golden_dir] = vpr/test/gui/golden  (parent of per-renderer subdirs,
#                  next to this script). Per-renderer goldens live under
#                  golden/<renderer>/<case>.png. Missing goldens FAIL their
#                  case (strict mode).
#   [threshold]  = $VPR_GUI_SSIM_THRESHOLD, else unset. An optional SSIM
#                  floor applied on top of the exact pixel gate; it never
#                  loosens it.
#
# Exit: 0 if every comparison ran and passed, 1 if any failed, 2 if any were
#       skipped or nothing was compared (ctest SKIP_RETURN_CODE).

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

VPR_ARG="${1:-${REPO_ROOT}/build/vpr/vpr}"
ARCH_ARG="${2:-${REPO_ROOT}/vtr_flow/arch/timing}"
BENCH_ARG="${3:-${REPO_ROOT}/vtr_flow/benchmarks/microbenchmarks}"

readonly VPR="$(cd "$(dirname "${VPR_ARG}")" && pwd)/$(basename "${VPR_ARG}")"
readonly ARCH_DIR="$(cd "${ARCH_ARG}" && pwd)"
readonly BENCH_DIR="$(cd "${BENCH_ARG}" && pwd)"

readonly GOLDEN_DIR="${4:-${SCRIPT_DIR}/golden}"
# Unset by default so compare_images.py owns the value. Hard-coding it here
# is how the runner drifted to 0.98 while ctest passed 0.999.
readonly THRESHOLD="${5:-${VPR_GUI_SSIM_THRESHOLD:-}}"
readonly ARCH="${ARCH_DIR}/k6_N10_40nm.xml"
readonly BENCH="${BENCH_DIR}/mult_4x4.blif"
readonly COMPARE="${SCRIPT_DIR}/compare_images.py"

# Renderer matrix. Order matters only for log/output ordering — comparisons
# are independent across renderers.
readonly RENDERERS=(rhi immediate deferred)

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
# shellcheck source=find_gui_python.sh
source "${SCRIPT_DIR}/find_gui_python.sh"
if ! PYTHON="$(resolve_gui_python)"; then
    echo "ERROR: No Python interpreter with scikit-image / Pillow / numpy found." >&2
    gui_python_hint
    exit 2
fi
readonly PYTHON

export QT_SCALE_FACTOR=1

# Rendered PNGs and per-case diff PNGs go to a stable, predictable dir under
# the cmake build tree so a human can open them after the run. The whole
# artifacts dir is wiped at the start of every run so stale outputs from a
# previous session can't be mistaken for the current one.
#
# Diff-write policy:
#   default              → diff written only when the case FAILs.
#   VPR_GUI_DEBUG=1      → diff written for every case (PASS and FAIL),
#                          set by `run_all_tests.sh --debug`.
# REPO_ROOT is VTR's SOURCE root; under a nested build <src>/build is not the
# build tree, and this dir gets rm -rf'd below.
# CMake passes the real one; the default only serves a standalone build.
readonly TEST_OUTDIR="${VPR_GUI_ARTIFACT_DIR:-${REPO_ROOT}/build/vpr/test/gui/artifacts}"
rm -rf "${TEST_OUTDIR}"

PASS=0
FAIL=0
SKIP=0

# Fresh checkout: no goldens at all under golden/<renderer>/. Skip the
# whole layer instead of producing 42 FAILs with the same root cause.
have_any_goldens=0
for r in "${RENDERERS[@]}"; do
    if [[ -d "${GOLDEN_DIR}/${r}" ]] \
       && [[ -n "$(ls -A "${GOLDEN_DIR}/${r}"/*.png 2>/dev/null)" ]]; then
        have_any_goldens=1
        break
    fi
done
if [[ "${have_any_goldens}" -eq 0 ]]; then
    echo "WARNING: No golden images in ${GOLDEN_DIR}/{${RENDERERS[*]// /,}}/"
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
echo "    Threshold: ${THRESHOLD:-none (exact pixel match only)}"
echo "    Renderers: ${RENDERERS[*]}"
echo "    Cases:     ${#VISUAL_CASE_NAMES[@]} × ${#RENDERERS[@]} = $(( ${#VISUAL_CASE_NAMES[@]} * ${#RENDERERS[@]} ))"
echo ""

# --- VPR: run each renderer through both graphics_commands phases ------------
for renderer in "${RENDERERS[@]}"; do
    R_OUTDIR="${TEST_OUTDIR}/${renderer}"
    mkdir -p "${R_OUTDIR}/diff"

    echo "--- VPR [${renderer}] run 1: placement + routing overlays (clean fixture, --analysis)"
    visual_run_pass_clean "${VPR}" "${ARCH}" "${BENCH}" "${R_OUTDIR}" \
        "${R_OUTDIR}/vpr_placement_routing.log" "${PLACEMENT_ROUTING_CMDS}" \
        --renderer "${renderer}"

    echo "--- VPR [${renderer}] run 2: routing_done congestion (congested fixture, --analysis)"
    visual_run_pass_congested "${VPR}" "${ARCH}" "${BENCH}" "${R_OUTDIR}" \
        "${R_OUTDIR}/vpr_routing_done_congestion.log" "${ROUTING_DONE_CONGESTION_CMDS}" \
        --renderer "${renderer}"
done

# A host without QRhi support (no GPU, older Mesa) silently falls back to the
# immediate rasterizer, so --renderer rhi yields immediate output. Comparing
# that against an rhi golden is meaningless in both directions: it hides real
# rhi regressions, and regenerating from it overwrites the rhi baseline with
# immediate pixels. Detect the fallback and skip the renderer instead.
# Detect by output identity, not by log text: the Qt diagnostic differs per
# backend ("QRhi is not supported", "QRhiGles2: Failed to create context", ...)
# and EZGL still reports "active renderer backend: rhi" after a failed context.
# If rhi rendered the same bytes as immediate, it fell back. A false positive is
# harmless -- identical output means the rhi comparison adds nothing anyway.
# Probe several cases, not one: the simplest scene could coincide across
# backends, and a single match would then silently retire all 14 rhi
# comparisons. Require every probe to agree, and refuse to judge on empty files
# (cmp -s calls two empty files identical, which would mask a render failure as
# a fallback).
rhi_fell_back() {
    local case_name probe_rhi probe_imm
    for case_name in "${VISUAL_CASE_NAMES[@]:0:3}"; do
        probe_rhi="${TEST_OUTDIR}/rhi/${case_name}.png"
        probe_imm="${TEST_OUTDIR}/immediate/${case_name}.png"
        [[ -s "${probe_rhi}" && -s "${probe_imm}" ]] || return 1
        cmp -s "${probe_rhi}" "${probe_imm}" || return 1
    done
    return 0
}

# --- Compare each (renderer, case) pair against its per-renderer golden ------
echo ""
echo "--- Comparing ${#VISUAL_CASE_NAMES[@]} cases × ${#RENDERERS[@]} renderers"
for renderer in "${RENDERERS[@]}"; do
    if [[ "${renderer}" == "rhi" ]] && rhi_fell_back; then
        echo "--- [VISUAL rhi] SKIP: QRhi unsupported here; renders came from the"
        echo "                 immediate fallback and cannot be judged against rhi goldens."
        (( SKIP += ${#VISUAL_CASE_NAMES[@]} )) || true
        continue
    fi
    R_OUTDIR="${TEST_OUTDIR}/${renderer}"
    R_GOLDEN="${GOLDEN_DIR}/${renderer}"
    R_DIFF="${R_OUTDIR}/diff"

    for name in "${VISUAL_CASE_NAMES[@]}"; do
        echo "--- [VISUAL ${renderer}] ${name}"

        local_golden="${R_GOLDEN}/${name}.png"
        if [[ ! -f "${local_golden}" ]]; then
            echo "    FAIL: missing golden ${local_golden}"
            echo "          Promote with: cp ${R_OUTDIR}/${name}.png ${local_golden}"
            (( FAIL++ )) || true
            continue
        fi

        local_current="${R_OUTDIR}/${name}.png"
        if [[ ! -f "${local_current}" ]]; then
            echo "    FAIL: ${name}.png not produced by --renderer ${renderer}"
            echo "    See: ${R_OUTDIR}/vpr_placement_routing.log, ${R_OUTDIR}/vpr_routing_done_congestion.log"
            (( FAIL++ )) || true
            continue
        fi

        compare_args=(
            "${local_golden}" "${local_current}"
            --diff-out "${R_DIFF}/${name}.png"
        )
        [[ -n "${THRESHOLD}" ]] && compare_args+=(--threshold "${THRESHOLD}")
        if [[ "${VPR_GUI_DEBUG:-0}" != "1" ]]; then
            # Default: only write the triptych on FAIL — keeps passing runs cheap.
            # --debug flips this off so passing cases also produce diffs.
            compare_args+=(--diff-on-fail-only)
        fi
        if "${PYTHON}" "${COMPARE}" "${compare_args[@]}"; then
            (( PASS++ )) || true
        else
            (( FAIL++ )) || true
            echo "    Golden:  ${local_golden}"
            echo "    Current: ${local_current}"
        fi
    done
done

# ---- Summary ---------------------------------------------------------------
echo ""
echo "=== Visual Regression Summary ==="
echo "    Passed:  ${PASS}"
echo "    Failed:  ${FAIL}"
echo "    Skipped: ${SKIP}"
echo "    Artifacts: ${TEST_OUTDIR}"
echo "      <renderer>/<case>.png         rendered output PNGs (one subdir per --renderer)"
echo "      <renderer>/vpr_<phase>.log    stdout/stderr per VPR invocation"
if [[ "${VPR_GUI_DEBUG:-0}" == "1" ]]; then
    echo "      <renderer>/diff/<case>.png    diff triptychs vs golden (--debug: all cases)"
else
    echo "      <renderer>/diff/<case>.png    diff triptychs vs golden (failures only)"
fi

if [[ "${FAIL}" -gt 0 ]]; then
    echo "    RESULT: FAIL"
    exit 1
fi

# A run that compared nothing is not a pass. Guard this explicitly: the whole
# reason the goldens went stale for three months is that a layer reporting
# success without executing looks identical to one that worked.
if [[ "${PASS}" -eq 0 ]]; then
    echo "    RESULT: SKIP (nothing was compared)"
    exit 2
fi

# Partial skips (e.g. an rhi fallback retiring 14 of 42) must not read as a
# clean pass either -- exit 2 so ctest reports Skipped, not Passed.
if [[ "${SKIP}" -gt 0 ]]; then
    echo "    RESULT: SKIP (${PASS} compared, ${SKIP} skipped -- see above)"
    exit 2
fi

echo "    RESULT: PASS"
exit 0
