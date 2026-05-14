#!/usr/bin/env bash
# Shared case definitions for generate_goldens.sh and run_visual_regression.sh.
# Sourced — do not invoke directly.
#
# Layout: two VPR invocations sweep the graphics-state variables at
# stage barriers.
#
#   Pass 1 (--pack --place --route)
#     placement_done   : nets / critical-path / block-internals overlays
#     routing_done     : routed-net + routing-aware critical-path overlays
#
#   Pass 2 (--pack --place --route)
#     routing_initial  : congestion overlays (mid-flight route_ctx state)
#
# Naming is functional, not numeric — "critical_path_delays" instead of
# "cpd=2". Every PNG name listed in VISUAL_CASE_NAMES must be emitted by
# exactly one save_graphics line in PASS1_CMDS or PASS2_CMDS.

# All cases the visual-regression and golden-generation pipelines exercise.
# Keep this list aligned with the save_graphics lines in PASS1_CMDS /
# PASS2_CMDS below — both halves are validated by run_visual_regression.sh.
VISUAL_CASE_NAMES=(
    # --- Pass 1, placement_done barrier ---------------------------------
    placement_default
    placement_nets_flylines
    placement_critical_path_flylines
    placement_critical_path_delays
    placement_critical_path_flylines_and_delays
    placement_block_internals

    # --- Pass 1, routing_done barrier -----------------------------------
    routing_default
    routing_nets_routed
    routing_critical_path_routed_wires
    routing_critical_path_flylines_and_routed
    routing_critical_path_flylines_delays_routed

    # --- Pass 2, routing_initial barrier --------------------------------
    routing_initial_congestion_off
    routing_initial_congestion_nodes
    routing_initial_congestion_nodes_and_nets
)

# Pass 1: placement + routing overlays in one VPR run. Every set_* command
# is paired with its inverse so the next save_graphics starts from a known
# baseline.
#
# set_nets : 0=off, 1=flylines (placement), 2=routed (routing).
# set_cpd  : 0=off; bitmask 1|2|4 = flylines | delays | routed-wires.
#            Bit 2 (routed-wire highlight) is only meaningful at routing.
PASS1_CMDS="\
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

# Pass 2: congestion at routing_initial (first routing-stage screen update,
# mid-flight route_ctx). A separate VPR invocation so the only saves in
# this run are the congestion ones — keeps the initial-route checkpoint
# isolated from the placement_done / routing_done barriers above.
#
# set_congestion : 0=off, 1=congested nodes, 2=congested nodes + nets.
PASS2_CMDS="\
wait_for_stage routing_initial; \
save_graphics routing_initial_congestion_off.png; \
set_congestion 1; save_graphics routing_initial_congestion_nodes.png; set_congestion 0; \
set_congestion 2; save_graphics routing_initial_congestion_nodes_and_nets.png; set_congestion 0; \
exit 0"

# Run one VPR pass with the given graphics_commands string. The working
# directory becomes the cwd of VPR, so each save_graphics writes its PNG
# next to vpr.log inside ${workdir}.
#
# Args: <vpr_binary> <arch_xml> <bench_blif> <workdir> <graphics_commands>
#       [extra VPR flags...]
# Returns 0 on success; on failure prints the log path to stderr and
# returns 1.
visual_run_pass() {
    local vpr="$1"; shift
    local arch="$1"; shift
    local bench="$1"; shift
    local workdir="$1"; shift
    local cmds="$1"; shift

    mkdir -p "${workdir}"

    if [[ "${SHOW_CMD:-0}" == "1" ]]; then
        printf 'VPR CMD:\n'
        printf '%q %q %q --disp off --seed 1' \
            "${vpr}" "${arch}" "${bench}"
        printf ' %q' "$@"
        printf ' --graphics_commands %q\n' "${cmds}"
    fi

    if ! (cd "${workdir}" && "${vpr}" "${arch}" "${bench}" \
        --disp off --seed 1 "$@" \
        --graphics_commands "${cmds}") \
        > "${workdir}/vpr.log" 2>&1; then
        echo "ERROR: VPR pass failed; see ${workdir}/vpr.log" >&2
        return 1
    fi
    return 0
}
