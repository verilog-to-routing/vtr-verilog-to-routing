#!/usr/bin/env bash
# Layer 5 — Generate golden images for visual regression testing.
#
# Runs VPR twice — one invocation for placement_done + routing_done overlays,
# one for routing_initial congestion — and saves every named PNG from
# visual_cases.sh into the golden/ directory. These images serve as the
# reference baseline for SSIM comparison.
#
# Usage:
#   ./generate_goldens.sh                                              # use defaults below
#   ./generate_goldens.sh <vpr_binary> <arch_dir> <bench_dir> [golden_dir]
#
# Defaults (resolved relative to the repo root containing this script):
#   <vpr_binary> = build/vpr/vpr
#   <arch_dir>   = vtr_flow/arch/timing
#   <bench_dir>  = vtr_flow/benchmarks/microbenchmarks
#   [golden_dir] = vpr/test/gui/golden  (next to this script)
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

# Shared case list + graphics_commands strings + visual_run_pass helper.
# shellcheck disable=SC1091
source "${SCRIPT_DIR}/visual_cases.sh"

export QT_SCALE_FACTOR=1

GEN_TMPDIR=$(mktemp -d)
trap 'rm -rf "${GEN_TMPDIR}"' EXIT

mkdir -p "${GOLDEN_DIR}"

echo "=== Generating Golden Images ==="
echo "    VPR:     ${VPR}"
echo "    Arch:    ${ARCH}"
echo "    Bench:   ${BENCH}"
echo "    Output:  ${GOLDEN_DIR}"
echo "    Cases:   ${#VISUAL_CASE_NAMES[@]}"
echo ""

# Both VPR invocations write into the same flat dir; case names are unique
# across invocations so there's no clash.
echo "--- VPR run 1: placement + routing overlays"
visual_run_pass "${VPR}" "${ARCH}" "${BENCH}" "${GEN_TMPDIR}" \
    "${GEN_TMPDIR}/vpr_placement_routing.log" "${PLACEMENT_ROUTING_CMDS}" --pack --place --route

echo "--- VPR run 2: routing_initial congestion"
visual_run_pass "${VPR}" "${ARCH}" "${BENCH}" "${GEN_TMPDIR}" \
    "${GEN_TMPDIR}/vpr_routing_initial.log" "${ROUTING_INITIAL_CMDS}" --pack --place --route

# --- Collect: every named case must have been emitted by some invocation -----
echo ""
echo "--- Collecting PNGs into ${GOLDEN_DIR}"
missing=()
for name in "${VISUAL_CASE_NAMES[@]}"; do
    src="${GEN_TMPDIR}/${name}.png"
    if [[ ! -f "${src}" ]]; then
        missing+=("${name}")
        echo "    MISSING: ${name}.png — not emitted by any VPR invocation"
        continue
    fi
    mv "${src}" "${GOLDEN_DIR}/${name}.png"
    size=$(wc -c < "${GOLDEN_DIR}/${name}.png")
    echo "    OK: ${GOLDEN_DIR}/${name}.png (${size} bytes)"
done

if [[ ${#missing[@]} -gt 0 ]]; then
    echo ""
    echo "ERROR: ${#missing[@]} case(s) failed to produce a PNG. Check:"
    echo "    ${GEN_TMPDIR}/vpr_placement_routing.log"
    echo "    ${GEN_TMPDIR}/vpr_routing_initial.log"
    exit 1
fi

echo ""
echo "=== Golden Generation Complete ==="
echo "    Files in: ${GOLDEN_DIR}"
ls -la "${GOLDEN_DIR}"/*.png 2>/dev/null || echo "    (no PNG files found)"
