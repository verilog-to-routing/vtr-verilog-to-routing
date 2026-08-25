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
# using SSIM (compare_images.py).
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

readonly GOLDEN_DIR="${4:-${SCRIPT_DIR}/golden}"
readonly THRESHOLD="${5:-${VPR_GUI_SSIM_THRESHOLD:-0.98}}"
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

# Rendered PNGs and per-case diff PNGs go to a stable, predictable dir under
# the cmake build tree so a human can open them after the run. The whole
# artifacts dir is wiped at the start of every run so stale outputs from a
# previous session can't be mistaken for the current one.
#
# Diff-write policy:
#   default              → diff written only when SSIM < threshold (FAIL).
#   VPR_GUI_DEBUG=1      → diff written for every case (PASS and FAIL),
#                          set by `run_all_tests.sh --debug`.
readonly TEST_OUTDIR="${REPO_ROOT}/build/vpr/test/gui/artifacts"
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
echo "    Threshold: ${THRESHOLD}"
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

# --- Compare each (renderer, case) pair against its per-renderer golden ------
echo ""
echo "--- Comparing ${#VISUAL_CASE_NAMES[@]} cases × ${#RENDERERS[@]} renderers"
for renderer in "${RENDERERS[@]}"; do
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
            --threshold "${THRESHOLD}"
            --diff-out "${R_DIFF}/${name}.png"
        )
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

echo "    RESULT: PASS"
exit 0
