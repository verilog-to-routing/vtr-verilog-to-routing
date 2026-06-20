#!/usr/bin/env bash
# Layer 5 — Generate per-renderer golden images for visual regression testing.
#
# For each renderer in {rhi, immediate, deferred}, runs VPR twice under
# --analysis against the checked-in fixtures under vpr/test/gui/fixtures/
# (clean .net/.place/.route for the placement+routing pass, congested
# fixture for the routing_done congestion pass — same as
# run_visual_regression.sh) and saves every named PNG from visual_cases.sh
# into golden/<renderer>/<case>.png.
#
# Why --analysis: pack/place/route run in LOAD mode, so the rendered scenes
# depend only on the on-disk fixture files, not on the host VPR version's
# heuristics. Goldens stay valid across VPR upgrades.
#
# Also produces developer-debug triptychs under golden/tmp/, one per
# non-rhi renderer per case:
#   golden/tmp/<case>_immediate.png = [rhi | immediate | 8x-amplified-diff]
#   golden/tmp/<case>_deferred.png  = [rhi | deferred  | 8x-amplified-diff]
# These are NOT goldens; they exist so a human can eyeball cross-renderer
# divergence (dash phase, sub-pixel stroke shift, ...) after a regeneration.
#
# Usage:
#   ./generate_goldens.sh                                              # use defaults below
#   ./generate_goldens.sh <vpr_binary> <arch_dir> <bench_dir> [golden_dir]
#
# Defaults (resolved relative to the repo root containing this script):
#   <vpr_binary> = build/vpr/vpr
#   <arch_dir>   = vtr_flow/arch/timing
#   <bench_dir>  = vtr_flow/benchmarks/microbenchmarks
#   [golden_dir] = vpr/test/gui/golden  (next to this script; per-renderer
#                  subdirs and the tmp/ debug dir all live underneath)
#
# NOTE: Re-generating goldens invalidates all previous comparisons.
#       Only regenerate when intentional visual changes are made.

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

readonly VPR_DEFAULT="${REPO_ROOT}/build/vpr/vpr"
readonly ARCH_DEFAULT="${REPO_ROOT}/vtr_flow/arch/timing"
readonly BENCH_DEFAULT="${REPO_ROOT}/vtr_flow/benchmarks/microbenchmarks"
readonly GOLDEN_DEFAULT="${SCRIPT_DIR}/golden"

VPR_ARG="${1:-${VPR_DEFAULT}}"
ARCH_ARG="${2:-${ARCH_DEFAULT}}"
BENCH_ARG="${3:-${BENCH_DEFAULT}}"

readonly VPR="$(cd "$(dirname "${VPR_ARG}")" && pwd)/$(basename "${VPR_ARG}")"
readonly ARCH_DIR="$(cd "${ARCH_ARG}" && pwd)"
readonly BENCH_DIR="$(cd "${BENCH_ARG}" && pwd)"
readonly GOLDEN_DIR="${4:-${GOLDEN_DEFAULT}}"
readonly ARCH="${ARCH_DIR}/k6_N10_40nm.xml"
readonly BENCH="${BENCH_DIR}/mult_4x4.blif"
readonly COMPARE="${SCRIPT_DIR}/compare_images.py"

# Shared case list + graphics_commands strings + visual_run_pass helper.
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/visual_cases.sh"

# Renderer matrix. Order matters: rhi is index 0 and used as the reference
# for the cross-renderer debug triptychs below.
readonly RENDERERS=(rhi immediate deferred)

export QT_SCALE_FACTOR=1

GEN_TMPDIR=$(mktemp -d)
trap 'rm -rf "${GEN_TMPDIR}"' EXIT

mkdir -p "${GOLDEN_DIR}"

echo "=== Generating Golden Images ==="
echo "    VPR:       ${VPR}"
echo "    Arch:      ${ARCH}"
echo "    Bench:     ${BENCH}"
echo "    Output:    ${GOLDEN_DIR}"
echo "    Renderers: ${RENDERERS[*]}"
echo "    Cases:     ${#VISUAL_CASE_NAMES[@]}"
echo ""

# --- VPR: run each renderer through both graphics_commands phases ------------
for renderer in "${RENDERERS[@]}"; do
    R_TMPDIR="${GEN_TMPDIR}/${renderer}"
    mkdir -p "${R_TMPDIR}"

    echo "--- VPR [${renderer}] run 1: placement + routing overlays (clean fixture, --analysis)"
    visual_run_pass_clean "${VPR}" "${ARCH}" "${BENCH}" "${R_TMPDIR}" \
        "${R_TMPDIR}/vpr_placement_routing.log" "${PLACEMENT_ROUTING_CMDS}" \
        --renderer "${renderer}"

    echo "--- VPR [${renderer}] run 2: routing_done congestion (congested fixture, --analysis)"
    visual_run_pass_congested "${VPR}" "${ARCH}" "${BENCH}" "${R_TMPDIR}" \
        "${R_TMPDIR}/vpr_routing_done_congestion.log" "${ROUTING_DONE_CONGESTION_CMDS}" \
        --renderer "${renderer}"
done

# --- Collect: every (renderer, case) pair must have produced a PNG -----------
echo ""
echo "--- Collecting PNGs into ${GOLDEN_DIR}/{${RENDERERS[*]// /,}}/"
missing=()
for renderer in "${RENDERERS[@]}"; do
    R_TMPDIR="${GEN_TMPDIR}/${renderer}"
    R_GOLDEN="${GOLDEN_DIR}/${renderer}"
    mkdir -p "${R_GOLDEN}"
    for name in "${VISUAL_CASE_NAMES[@]}"; do
        src="${R_TMPDIR}/${name}.png"
        if [[ ! -f "${src}" ]]; then
            missing+=("${renderer}/${name}")
            echo "    MISSING: ${renderer}/${name}.png — not emitted by VPR"
            continue
        fi
        mv "${src}" "${R_GOLDEN}/${name}.png"
        size=$(wc -c < "${R_GOLDEN}/${name}.png")
        echo "    OK: ${R_GOLDEN}/${name}.png (${size} bytes)"
    done
done

if [[ ${#missing[@]} -gt 0 ]]; then
    echo ""
    echo "ERROR: ${#missing[@]} (renderer, case) pair(s) failed to produce a PNG. Check:"
    for renderer in "${RENDERERS[@]}"; do
        echo "    ${GEN_TMPDIR}/${renderer}/vpr_placement_routing.log"
        echo "    ${GEN_TMPDIR}/${renderer}/vpr_routing_done_congestion.log"
    done
    exit 1
fi

# --- Debug triptychs: [rhi | <renderer> | 8x-diff] under golden/tmp/ ---------
# Resolve a Python interpreter that has skimage/PIL/numpy available; same
# fallback chain used by run_visual_regression.sh.
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

echo ""
# Always wipe the debug-triptych dir at the start of a regeneration so stale
# PNGs from a previous run can't linger — even when the Python deps below
# are missing and we skip the rewrite.
readonly TMP_DIR="${GOLDEN_DIR}/tmp"
rm -rf "${TMP_DIR}"
if ! PYTHON="$(resolve_python)"; then
    echo "WARNING: no Python with scikit-image/Pillow/numpy on PATH —"
    echo "         skipping cross-renderer debug triptychs."
    echo "         Install with:  python3 -m pip install scikit-image Pillow numpy"
else
    mkdir -p "${TMP_DIR}"
    echo "--- Writing debug triptychs into ${TMP_DIR}"
    echo "    Columns: <case>                              SSIM=<score>  (delta=1-SSIM)"
    for renderer in "${RENDERERS[@]}"; do
        [[ "${renderer}" == "rhi" ]] && continue
        echo "  rhi vs ${renderer}:"
        for name in "${VISUAL_CASE_NAMES[@]}"; do
            rhi_png="${GOLDEN_DIR}/rhi/${name}.png"
            cur_png="${GOLDEN_DIR}/${renderer}/${name}.png"
            out_png="${TMP_DIR}/${name}_${renderer}.png"
            # --threshold 0.0 → always exit 0 so set -e doesn't kill us on
            # unrelated SSIM-below-threshold differences; the triptych is
            # always written when --diff-on-fail-only is omitted. Drop
            # --quiet so we can extract the SSIM score and surface it
            # alongside the case name.
            cmp_out=$("${PYTHON}" "${COMPARE}" "${rhi_png}" "${cur_png}" \
                --threshold 0.0 --diff-out "${out_png}") || cmp_out="ERROR"
            ssim=$(printf '%s\n' "${cmp_out}" | awk '/^SSIM:/ {print $2}')
            ssim="${ssim:-?}"
            # Match the runner's default SSIM threshold (0.98). Any case
            # below that gets a [WARNING] tag in the leading column so a
            # scan of the output surfaces real cross-renderer drift.
            if [[ "${ssim}" == "?" ]]; then
                delta="?"
                tag="[ERROR]   "
            else
                delta=$(awk "BEGIN { printf \"%.6f\", 1 - ${ssim} }")
                if awk "BEGIN { exit !(${ssim} < 0.98) }"; then
                    tag="[WARNING] "
                else
                    tag="          "
                fi
            fi
            printf "    %s%-50s  SSIM=%s  delta=%s\n" "${tag}" "${name}" "${ssim}" "${delta}"
        done
    done
    echo "    Wrote $(ls "${TMP_DIR}"/*.png 2>/dev/null | wc -l) triptychs."
fi

echo ""
echo "=== Golden Generation Complete ==="
echo "    Goldens under: ${GOLDEN_DIR}/{${RENDERERS[*]// /,}}/"
for renderer in "${RENDERERS[@]}"; do
    count=$(ls "${GOLDEN_DIR}/${renderer}"/*.png 2>/dev/null | wc -l)
    echo "      ${renderer}/: ${count} files"
done
echo "    Debug triptychs: ${GOLDEN_DIR}/tmp/ (rhi-vs-{immediate,deferred} per case)"
