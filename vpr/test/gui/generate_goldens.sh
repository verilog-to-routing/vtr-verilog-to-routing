#!/usr/bin/env bash
# Layer 5 — Generate golden images for visual regression testing.
#
# Runs VPR with specific display settings, saves PNGs to golden/ directory.
# These images serve as the reference baseline for SSIM comparison.
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

export QT_SCALE_FACTOR=1

GEN_TMPDIR=$(mktemp -d)
trap 'rm -rf "${GEN_TMPDIR}"' EXIT

mkdir -p "${GOLDEN_DIR}"

echo "=== Generating Golden Images ==="
echo "    VPR:     ${VPR}"
echo "    Arch:    ${ARCH}"
echo "    Output:  ${GOLDEN_DIR}"
echo ""

# Helper: run VPR and move the output PNG to golden/
generate() {
    local name="$1"; shift
    local circuit="$1"; shift
    local vpr_flags=("$@")

    echo "--- [GOLDEN] ${name}"
    local work="${GEN_TMPDIR}/${name}"
    mkdir -p "${work}"

    if [[ "${SHOW_CMD:-0}" == "1" ]]; then
        printf 'VPR CMD:\n'
        printf '%q %q %q --disp off --seed 1' \
            "${VPR}" "${ARCH}" "${BENCH_DIR}/${circuit}"
        printf ' %q' "${vpr_flags[@]}"
        printf '\n'
    fi

    if ! (cd "${work}" && "${VPR}" "${ARCH}" "${BENCH_DIR}/${circuit}" \
        --disp off --seed 1 \
        "${vpr_flags[@]}") \
        > "${work}/vpr.log" 2>&1; then
        echo "    ERROR: VPR exited with non-zero status"
        echo "    See: ${work}/vpr.log"
        return 1
    fi

    if [[ -f "${work}/${name}.png" ]]; then
        mv "${work}/${name}.png" "${GOLDEN_DIR}/${name}.png"
        local size
        size=$(wc -c < "${GOLDEN_DIR}/${name}.png")
        echo "    OK: ${GOLDEN_DIR}/${name}.png (${size} bytes)"
    else
        echo "    ERROR: ${name}.png not generated"
        echo "    See: ${work}/vpr.log"
        return 1
    fi
}

# 1. placement_default — default placement view
generate "placement_default" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "save_graphics placement_default.png; exit 0"

# 2. placement_nets — nets overlay on placement
generate "placement_nets" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "set_nets 1; save_graphics placement_nets.png; exit 0"

# 3. placement_congestion — congestion heatmap
generate "placement_congestion" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "set_congestion 1; save_graphics placement_congestion.png; exit 0"

# 4. routing_default — routed netlist
generate "routing_default" "mult_4x4.blif" \
    --pack --place --route \
    --graphics_commands "save_graphics routing_default.png; exit 0"

# 5. routing_timing — critical path highlighting
generate "routing_timing" "mult_4x4.blif" \
    --pack --place --route \
    --graphics_commands "set_cpd 1; save_graphics routing_timing.png; exit 0"

# 6. placement_block_internals — sub-block detail
generate "placement_block_internals" "mult_4x4.blif" \
    --pack --place \
    --graphics_commands "set_draw_block_internals 2; save_graphics placement_block_internals.png; exit 0"

echo ""
echo "=== Golden Generation Complete ==="
echo "    Files in: ${GOLDEN_DIR}"
ls -la "${GOLDEN_DIR}"/*.png 2>/dev/null || echo "    (no PNG files found)"
