#!/usr/bin/env bash
# Shared case definitions for generate_goldens.sh and run_visual_regression.sh.
# Sourced — do not invoke directly.
#
# Layout: two VPR invocations sweep the graphics-state variables at
# stage barriers.
#
#   PLACEMENT_ROUTING_CMDS (--pack --place --route)
#     placement_done   : nets / critical-path / block-internals overlays
#     routing_done     : routed-net + routing-aware critical-path overlays
#
#   ROUTING_INITIAL_CMDS (--pack --place --route)
#     routing_initial  : congestion overlays (mid-flight route_ctx state)
#
# Naming is functional, not numeric — "critical_path_delays" instead of
# "cpd=2". Every PNG name listed in VISUAL_CASE_NAMES must be emitted by
# exactly one save_graphics line in PLACEMENT_ROUTING_CMDS or
# ROUTING_INITIAL_CMDS.

# All cases the visual-regression and golden-generation pipelines exercise.
# Keep this list aligned with the save_graphics lines in
# PLACEMENT_ROUTING_CMDS / ROUTING_INITIAL_CMDS below — both halves are
# validated by run_visual_regression.sh.
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

    # --- ROUTING_INITIAL_CMDS, routing_initial barrier ------------------
    routing_initial_congestion_off
    routing_initial_congestion_nodes
    routing_initial_congestion_nodes_and_nets
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

# Congestion at routing_initial (first routing-stage screen update, mid-flight
# route_ctx). A separate VPR invocation so the only saves in this run are the
# congestion ones — keeps the initial-route checkpoint isolated from the
# placement_done / routing_done barriers above.
#
# set_congestion : 0=off, 1=congested nodes, 2=congested nodes + nets.
ROUTING_INITIAL_CMDS="\
wait_for_stage routing_initial; \
save_graphics routing_initial_congestion_off.png; \
set_congestion 1; save_graphics routing_initial_congestion_nodes.png; set_congestion 0; \
set_congestion 2; save_graphics routing_initial_congestion_nodes_and_nets.png; set_congestion 0; \
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
