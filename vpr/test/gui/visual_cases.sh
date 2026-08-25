#!/usr/bin/env bash
# Shared case definitions for generate_goldens.sh and run_visual_regression.sh.
# Sourced — do not invoke directly.
#
# Layout: two VPR invocations sweep the graphics-state variables at stage
# barriers. Both use --analysis against checked-in .net / .place / .route
# fixtures under vpr/test/gui/fixtures/ so the rendered scenes do NOT depend
# on the host VPR version's pack/place/route heuristics.
#
#   PLACEMENT_ROUTING_CMDS (clean fixture, --analysis):
#     placement_done      : nets / critical-path / block-internals overlays
#     routing_done        : routed-net + routing-aware critical-path overlays
#
#   ROUTING_DONE_CONGESTION_CMDS (congested fixture, --analysis --check_route off
#                                 --route_chan_width must match what was saved):
#     routing_done        : congestion overlays (overuse encoded in fixture)
#
# Naming is functional, not numeric — "critical_path_delays" instead of
# "cpd=2". Every PNG name listed in VISUAL_CASE_NAMES must be emitted by
# exactly one save_graphics line in PLACEMENT_ROUTING_CMDS or
# ROUTING_DONE_CONGESTION_CMDS.

# Fixture file locations (relative to this script). Goldens become stable
# across VPR versions because pack/place/route are LOAD instead of DO:
# different heuristics no longer move blocks/wires.
readonly _VC_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly FIXTURES_DIR="${_VC_SCRIPT_DIR}/fixtures"
readonly CLEAN_NET="${FIXTURES_DIR}/clean/mult_4x4.net"
readonly CLEAN_PLACE="${FIXTURES_DIR}/clean/mult_4x4.place"
readonly CLEAN_ROUTE="${FIXTURES_DIR}/clean/mult_4x4.route"
# Channel width the clean fixture was routed at (min-W found by the binary
# search at fixture-generation time). vpr_load_routing fatals if chan_width
# is left at NO_FIXED_CHANNEL_WIDTH (-1), so this must be set on the
# --analysis command line.
readonly CLEAN_CHAN_WIDTH=16

readonly CONGESTED_NET="${FIXTURES_DIR}/congested/mult_4x4.net"
readonly CONGESTED_PLACE="${FIXTURES_DIR}/congested/mult_4x4.place"
readonly CONGESTED_ROUTE="${FIXTURES_DIR}/congested/mult_4x4.route"
# Channel width the congested fixture was routed at; must match the
# --route_chan_width passed at fixture-generation time so the loaded
# routing isn't reshaped, and is also required by vpr_load_routing.
readonly CONGESTED_CHAN_WIDTH=30

# All cases the visual-regression and golden-generation pipelines exercise.
# Keep this list aligned with the save_graphics lines in
# PLACEMENT_ROUTING_CMDS / ROUTING_DONE_CONGESTION_CMDS below — both halves
# are validated by run_visual_regression.sh.
VISUAL_CASE_NAMES=(
    # --- PLACEMENT_ROUTING_CMDS, placement_done barrier -----------------
    placement_default
    placement_nets_flylines
    placement_critical_path_flylines
    placement_critical_path_delays
    placement_critical_path_flylines_and_delays
    placement_block_internals

    # --- PLACEMENT_ROUTING_CMDS, routing_done barrier -------------------
    routing_default
    routing_nets_routed
    routing_critical_path_routed_wires
    routing_critical_path_flylines_and_routed
    routing_critical_path_flylines_delays_routed

    # --- ROUTING_DONE_CONGESTION_CMDS, routing_done barrier -------------
    routing_done_congestion_off
    routing_done_congestion_nodes
    routing_done_congestion_nodes_and_nets
)

# Placement + routing overlays in one VPR run. Every set_* command is paired
# with its inverse so the next save_graphics starts from a known baseline.
#
# set_nets : 0=off, 1=flylines (placement), 2=routed (routing).
# set_cpd  : 0=off; bitmask 1|2|4 = flylines | delays | routed-wires.
#            Bit 2 (routed-wire highlight) is only meaningful at routing.
PLACEMENT_ROUTING_CMDS="\
wait_for_stage placement_done; \
save_graphics placement_default.png; \
set_nets 1; save_graphics placement_nets_flylines.png; set_nets 0; \
set_cpd 1; save_graphics placement_critical_path_flylines.png; set_cpd 0; \
set_cpd 2; save_graphics placement_critical_path_delays.png; set_cpd 0; \
set_cpd 3; save_graphics placement_critical_path_flylines_and_delays.png; set_cpd 0; \
set_draw_block_internals 2; save_graphics placement_block_internals.png; set_draw_block_internals 0; \
wait_for_stage routing_done; \
save_graphics routing_default.png; \
set_nets 2; save_graphics routing_nets_routed.png; set_nets 0; \
set_cpd 4; save_graphics routing_critical_path_routed_wires.png; set_cpd 0; \
set_cpd 5; save_graphics routing_critical_path_flylines_and_routed.png; set_cpd 0; \
set_cpd 7; save_graphics routing_critical_path_flylines_delays_routed.png; set_cpd 0; \
exit 0"

# Congestion at routing_done against the congested fixture (saved with
# --max_router_iterations 1 at --route_chan_width 30, leaving real overuse
# in the .route file). A separate VPR invocation so the only saves in this
# run are the congestion ones, AND so it can pass --check_route off (which
# would mask real routing bugs in the clean pass).
#
# set_congestion : 0=off, 1=congested nodes, 2=congested nodes + nets.
ROUTING_DONE_CONGESTION_CMDS="\
wait_for_stage routing_done; \
save_graphics routing_done_congestion_off.png; \
set_congestion 1; save_graphics routing_done_congestion_nodes.png; set_congestion 0; \
set_congestion 2; save_graphics routing_done_congestion_nodes_and_nets.png; set_congestion 0; \
exit 0"

# Run one VPR invocation with the given graphics_commands string. ${outdir}
# becomes VPR's cwd so each save_graphics writes its PNG flat into outdir;
# stdout+stderr land in ${logfile} (callers pick distinct logfile names so
# multiple invocations sharing a single outdir don't clobber each other).
#
# Args: <vpr_binary> <arch_xml> <bench_blif> <outdir> <logfile>
#       <graphics_commands> [extra VPR flags...]
# Returns 0 on success; on failure prints the log path to stderr and
# returns 1.
visual_run_pass() {
    local vpr="$1"; shift
    local arch="$1"; shift
    local bench="$1"; shift
    local outdir="$1"; shift
    local logfile="$1"; shift
    local cmds="$1"; shift

    mkdir -p "${outdir}"
    mkdir -p "$(dirname "${logfile}")"

    if [[ "${SHOW_CMD:-0}" == "1" ]]; then
        printf 'VPR CMD:\n'
        printf '%q %q %q --disp off --seed 1' \
            "${vpr}" "${arch}" "${bench}"
        printf ' %q' "$@"
        printf ' --graphics_commands %q\n' "${cmds}"
    fi

    if ! (cd "${outdir}" && "${vpr}" "${arch}" "${bench}" \
        --disp off --seed 1 "$@" \
        --graphics_commands "${cmds}") \
        > "${logfile}" 2>&1; then
        echo "ERROR: VPR invocation failed; see ${logfile}" >&2
        return 1
    fi
    return 0
}

# Convenience wrapper: invoke visual_run_pass with --analysis against the
# CLEAN fixture (post-place / post-route overlays).
#
# Args: <vpr> <arch> <bench> <outdir> <logfile> <cmds> [extra VPR flags...]
visual_run_pass_clean() {
    local vpr="$1" arch="$2" bench="$3" outdir="$4" logfile="$5" cmds="$6"
    shift 6
    visual_run_pass "${vpr}" "${arch}" "${bench}" "${outdir}" "${logfile}" "${cmds}" \
        --analysis \
        --route_chan_width "${CLEAN_CHAN_WIDTH}" \
        --net_file "${CLEAN_NET}" \
        --place_file "${CLEAN_PLACE}" \
        --route_file "${CLEAN_ROUTE}" \
        "$@"
}

# Convenience wrapper: invoke visual_run_pass with --analysis against the
# CONGESTED fixture (illegal routing — needs --check_route off to bypass the
# legality VPR_ERROR, and --route_chan_width matching the saved width so the
# routing isn't reshaped).
#
# Args: <vpr> <arch> <bench> <outdir> <logfile> <cmds> [extra VPR flags...]
visual_run_pass_congested() {
    local vpr="$1" arch="$2" bench="$3" outdir="$4" logfile="$5" cmds="$6"
    shift 6
    visual_run_pass "${vpr}" "${arch}" "${bench}" "${outdir}" "${logfile}" "${cmds}" \
        --analysis --check_route off \
        --route_chan_width "${CONGESTED_CHAN_WIDTH}" \
        --net_file "${CONGESTED_NET}" \
        --place_file "${CONGESTED_PLACE}" \
        --route_file "${CONGESTED_ROUTE}" \
        "$@"
}
