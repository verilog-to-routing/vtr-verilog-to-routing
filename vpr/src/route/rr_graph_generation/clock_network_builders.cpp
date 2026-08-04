#include "clock_network_builders.h"

#include "globals.h"

#include "get_parallel_segs.h"
#include "rr_graph_sbox.h"
#include "rr_rc_data.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include <algorithm>

void static populate_segment_values(int seg_index,
                                    std::string name,
                                    int length,
                                    MetalLayer layer,
                                    std::vector<t_segment_inf>& segment_inf,
                                    e_parallel_axis parallel_axis,
                                    e_directionality directionality);

void populate_segment_values(int seg_index,
                             std::string name,
                             int length,
                             MetalLayer layer,
                             std::vector<t_segment_inf>& segment_inf,
                             e_parallel_axis parallel_axis,
                             e_directionality directionality) {
    segment_inf[seg_index].name = name;
    segment_inf[seg_index].length = length;
    segment_inf[seg_index].frequency = 1;
    segment_inf[seg_index].Rmetal = layer.r_metal;
    segment_inf[seg_index].Cmetal = layer.c_metal;
    segment_inf[seg_index].directionality = directionality;
    segment_inf[seg_index].longline = false;
    segment_inf[seg_index].parallel_axis = parallel_axis;

    segment_inf[seg_index].seg_index = seg_index;

    // unused values tagged with -1 (only used RR graph creation)
    segment_inf[seg_index].arch_wire_switch = -1;
    segment_inf[seg_index].arch_opin_switch = -1;
    segment_inf[seg_index].frac_cb = -1;
    segment_inf[seg_index].frac_sb = -1;
    segment_inf[seg_index].res_type = SegResType::GCLK;

    // Clock segments share the same segment_inf list used for regular routing
    // tracks (see alloc_and_load_seg_details()), so they need cb/sb patterns
    // just like architecture-defined segments. Default to fully connectable,
    // matching the default used when a <segment> doesn't specify <cb>/<sb>.
    segment_inf[seg_index].cb.resize(length, true);
    segment_inf[seg_index].sb.resize(length + 1, true);
}

/*
 * ClockNetwork (getters)
 */

int ClockNetwork::get_num_inst() const {
    return num_inst_;
}

std::string ClockNetwork::get_name() const {
    return clock_name_;
}

/*
 * ClockNetwork (setters)
 */

void ClockNetwork::set_clock_name(std::string clock_name) {
    clock_name_ = clock_name;
}

void ClockNetwork::set_num_instance(int num_inst) {
    num_inst_ = num_inst;
}

/*
 * ClockNetwork (Member functions)
 */

void ClockNetwork::create_rr_nodes_for_clock_network_wires(ClockRRGraphBuilder& clock_graph,
                                                           t_rr_graph_storage* rr_nodes,
                                                           RRGraphBuilder& rr_graph_builder,
                                                           t_rr_edge_info_set* rr_edges_to_create,
                                                           int num_segments) {
    for (int inst_num = 0; inst_num < get_num_inst(); inst_num++) {
        create_rr_nodes_and_internal_edges_for_one_instance(clock_graph, rr_nodes, rr_graph_builder, rr_edges_to_create, num_segments);
    }
}

/*********************************************************************************
 *********************************************************************************
 *********************** ClockRib Function Implementations ***********************
 *********************************************************************************
 *********************************************************************************/

/*
 * ClockRib (getters)
 */

ClockType ClockRib::get_network_type() const {
    return ClockType::RIB;
}

/*
 * ClockRib (setters)
 */

void ClockRib::set_metal_layer(float r_metal, float c_metal) {
    x_chan_wire_.layer.r_metal = r_metal;
    x_chan_wire_.layer.c_metal = c_metal;
}

void ClockRib::set_metal_layer(MetalLayer metal_layer) {
    x_chan_wire_.layer = metal_layer;
}

void ClockRib::set_initial_wire_location(int start_x, int end_x, int y) {
    if (end_x <= start_x) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Clock Network wire cannot have negative or zero length. "
                        "Wire end: %d < wire start: %d\n",
                        end_x, start_x);
    }

    x_chan_wire_.start = start_x;
    x_chan_wire_.length = end_x - start_x;
    x_chan_wire_.position = y;
}

void ClockRib::set_wire_repeat(int repeat_x, int repeat_y, int repeat_y_max) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinite loop when creating ribs
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
                        repeat_x, repeat_y);
    }

    repeat_.x = repeat_x;
    repeat_.y = repeat_y;
    repeat_.y_max = repeat_y_max;
}

void ClockRib::set_drive_location(int offset_x) {
    drive_.offset = offset_x;
}

void ClockRib::set_drive_switch(int switch_idx) {
    drive_.switch_idx = switch_idx;
}

void ClockRib::set_drive_name(std::string name) {
    drive_.name = name;
}

void ClockRib::set_tap_locations(int offset_x, int increment_x) {
    tap_.offset = offset_x;
    tap_.increment = increment_x;
}

void ClockRib::set_tap_name(std::string name) {
    tap_.name = name;
}

/*
 * ClockRib (member functions)
 */

void ClockRib::create_segments(std::vector<t_segment_inf>& segment_inf) {
    int index;
    std::string name;
    int length;

    // Drive point segment
    segment_inf.emplace_back();
    drive_seg_idx_ = segment_inf.size() - 1;

    index = drive_seg_idx_;
    name = clock_name_ + "_drive";
    length = 1; // Since drive segment has one length, the left and right segments have length - 1

    /*AA: ClockRibs are assumed to be horizontal currently. */

    // The drive point node is a degenerate zero-length hub (see create_chanx_wire),
    // not a real wire span, and is tagged Direction::BIDIR; the left/right "wing"
    // nodes it feeds are already unidirectional (Direction::DEC/INC respectively).
    // Segment directionality here is tagged to match each node's real Direction.
    populate_segment_values(index, name, length, x_chan_wire_.layer, segment_inf, e_parallel_axis::X_AXIS, BI_DIRECTIONAL);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx_ = segment_inf.size() - 1;

    index = right_seg_idx_;
    name = clock_name_ + "_right";
    // Clamped to a minimum of 1: when the drive point sits at the rib's right
    // endpoint, no right segment is ever built (see
    // create_rr_nodes_and_internal_edges_for_one_instance) and no RR node ever
    // references this segment_inf entry. A length of 1 (rather than 0) is used
    // as its nominal placeholder length, since a length of 0 makes inv_length
    // (1/length, see alloc_and_load_rr_indexed_data.cpp) infinite, which can
    // corrupt router lookahead cost-map construction even though the segment
    // itself is unused.
    length = std::max((x_chan_wire_.length - drive_.offset) - 1, 1);

    populate_segment_values(index, name, length, x_chan_wire_.layer, segment_inf, e_parallel_axis::X_AXIS, UNI_DIRECTIONAL);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx_ = segment_inf.size() - 1;

    index = left_seg_idx_;
    name = clock_name_ + "_left";
    // Clamped to a minimum of 1 -- see the comment on the right segment above.
    length = std::max(drive_.offset - 1, 1);

    populate_segment_values(index, name, length, x_chan_wire_.layer, segment_inf, e_parallel_axis::X_AXIS, UNI_DIRECTIONAL);

    // Snapshot the unified-space indices for map_relative_seg_indices to remap from.
    drive_seg_idx_unified_ = drive_seg_idx_;
    right_seg_idx_unified_ = right_seg_idx_;
    left_seg_idx_unified_ = left_seg_idx_;
}

size_t ClockRib::estimate_additional_nodes(const DeviceGrid& grid) {
    // Avoid an infinite loop
    VTR_ASSERT(repeat_.y > 0);
    VTR_ASSERT(repeat_.x > 0);

    size_t num_additional_nodes = 0;
    unsigned y_tile_end = std::min<unsigned>(grid.height() - 1, repeat_.y_max);
    for (unsigned y = x_chan_wire_.position; y < y_tile_end; y += repeat_.y) {
        for (unsigned x_start = x_chan_wire_.start; x_start < grid.width() - 1; x_start += repeat_.x) {
            unsigned drive_x = x_start + drive_.offset;
            unsigned x_end = x_start + x_chan_wire_.length;

            // FIXME: this clamp (and its 3 siblings in this file, plus the matching
            // boundary-shift heuristic in ClockToClockConneciton::create_switches in
            // clock_connection_builders.cpp) hardcodes the assumption that I/O lives on
            // the device perimeter, so rib/spine wires must stop 1 tile short of the
            // edge ("dont go above the LB"). That assumption doesn't generally hold --
            // nothing guarantees I/O is perimeter-only -- so this logic should be
            // reworked to derive the valid span from the actual device/block layout
            // instead of a hardcoded "-2" offset. Needs investigation.
            // Adjust for boundary conditions
            int x_offset = 0;
            if ((x_start == 0) ||               // CHANX wires left boundary
                (x_start + repeat_.x == x_end)) // Avoid overlap
            {
                x_offset = 1;
            }
            if (x_end > grid.width() - 2) {
                x_end = grid.width() - 2; // CHANX wires right boundary
            }

            unsigned x_start_adj = x_start + x_offset;

            // Dont create rib if drive point is not reachable
            if (drive_x > grid.width() - 2 || drive_x < x_start_adj || drive_x > x_end) {
                continue;
            }

            // Dont create rib if wire segment is too small
            if (x_start_adj >= x_end) {
                continue;
            }

            num_additional_nodes += 3;
        }
    }

    return num_additional_nodes;
}

void ClockRib::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                   t_rr_graph_storage* rr_nodes,
                                                                   RRGraphBuilder& rr_graph_builder,
                                                                   t_rr_edge_info_set* rr_edges_to_create,
                                                                   int num_segments) {
    // Only chany wires need to know the number of segments inorder
    // to calculate the cost_index
    (void)num_segments;

    const auto& grid = clock_graph.grid();

    int ptc_num = clock_graph.get_and_increment_chanx_ptc_num(); // used for drawing

    // Avoid an infinite loop
    VTR_ASSERT(repeat_.y > 0);
    VTR_ASSERT(repeat_.x > 0);

    // TODO: This function is not adapted to the multi-layer grid
    VTR_ASSERT(g_vpr_ctx.device().grid.get_num_layers() == 1);
    int layer_num = 0;

    unsigned y_tile_end = std::min<unsigned>(grid.height() - 1, repeat_.y_max);
    for (unsigned y = x_chan_wire_.position; y < y_tile_end; y += repeat_.y) {
        for (unsigned x_start = x_chan_wire_.start; x_start < grid.width() - 1; x_start += repeat_.x) {
            unsigned drive_x = x_start + drive_.offset;
            unsigned x_end = x_start + x_chan_wire_.length;

            // FIXME: see the perimeter-I/O assumption note on the matching clamp in
            // ClockRib::estimate_additional_nodes above.
            // Adjust for boundary conditions
            int x_offset = 0;
            if ((x_start == 0) ||               // CHANX wires left boundary
                (x_start + repeat_.x == x_end)) // Avoid overlap
            {
                x_offset = 1;
            }
            if (x_end > grid.width() - 2) {
                x_end = grid.width() - 2; // CHANX wires right boundary
            }

            unsigned x_start_adj = x_start + x_offset;

            // Dont create rib if the drive point falls outside the buildable span.
            // The drive point itself is allowed to sit on either endpoint (x_start_adj
            // or x_end): in that case the segment on that side is simply omitted rather
            // than being an inverted/empty range.
            if (drive_x > grid.width() - 2 || drive_x < x_start_adj || drive_x > x_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "A rib part of clock network '%s' was not"
                                    " created because the drive point is not reachable. "
                                    "This can lead to an unroutable architecture.\n",
                                    clock_name_.c_str());
                continue;
            }

            // Dont create rib if wire segment is too small
            if (x_start_adj >= x_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "Rib start '%d' and end '%d' values are "
                                    "not successive for clock network '%s' due to not meeting boundary conditions."
                                    " This can lead to an unroutable architecture.\n",
                                    x_start_adj, x_end, clock_name_.c_str());
                continue;
            }

            bool has_left = drive_x > x_start_adj;
            bool has_right = drive_x < x_end;

            // create drive point (length zero wire)
            auto drive_node_idx = create_chanx_wire(layer_num,
                                                    drive_x,
                                                    drive_x,
                                                    y,
                                                    ptc_num,
                                                    Direction::BIDIR,
                                                    rr_nodes,
                                                    rr_graph_builder);
            clock_graph.add_switch_location(get_name(), drive_.name, drive_x, y, drive_node_idx);

            // create rib wire to the right and/or left of the drive point. When the
            // drive point is placed at one of the rib's endpoints, that side is skipped
            // entirely instead of being built as a degenerate zero/negative-length wire.
            int left_node_idx = -1;
            if (has_left) {
                left_node_idx = create_chanx_wire(layer_num,
                                                  x_start_adj,
                                                  drive_x - 1,
                                                  y,
                                                  ptc_num,
                                                  Direction::DEC,
                                                  rr_nodes,
                                                  rr_graph_builder);
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(left_node_idx), drive_.switch_idx, false);
            }

            int right_node_idx = -1;
            if (has_right) {
                right_node_idx = create_chanx_wire(layer_num,
                                                   drive_x + 1,
                                                   x_end,
                                                   y,
                                                   ptc_num,
                                                   Direction::INC,
                                                   rr_nodes,
                                                   rr_graph_builder);
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(right_node_idx), drive_.switch_idx, false);
            }

            record_tap_locations(x_start_adj,
                                 x_end,
                                 drive_x,
                                 y,
                                 drive_node_idx,
                                 left_node_idx,
                                 right_node_idx,
                                 clock_graph);
        }
    }
}

int ClockRib::create_chanx_wire(int layer,
                                int x_start,
                                int x_end,
                                int y,
                                int ptc_num,
                                Direction direction,
                                t_rr_graph_storage* rr_nodes,
                                RRGraphBuilder& rr_graph_builder) {
    rr_nodes->emplace_back();
    size_t node_index = rr_nodes->size() - 1;
    RRNodeId chanx_node = RRNodeId(node_index);

    rr_graph_builder.set_node_type(chanx_node, e_rr_type::CHANX);
    rr_graph_builder.set_node_coordinates(chanx_node, x_start, y, x_end, y);
    rr_graph_builder.set_node_layer(chanx_node, layer, layer);
    rr_graph_builder.set_node_capacity(chanx_node, 1);
    rr_graph_builder.set_node_track_num(chanx_node, ptc_num);
    // FIXME: Rmetal/Cmetal are per-unit-length (see doc/src/arch/reference.rst), but
    // here they're used as a flat per-node total with no scaling by (x_end - x_start).
    // This means wire delay doesn't scale with rib length/repeat spacing. Same bug as
    // ClockSpine::create_chany_wire below, and the same one already fixed for
    // ClockSwitchGrid::create_chanx_node/create_chany_node -- see the comment there
    // for the fix (scale by node tile span before calling find_create_rr_rc_data).
    const NodeRCIndex rc_index = find_create_rr_rc_data(x_chan_wire_.layer.r_metal, x_chan_wire_.layer.c_metal, g_vpr_ctx.mutable_device().rr_rc_data);
    rr_graph_builder.set_node_rc_index(chanx_node, rc_index);
    rr_graph_builder.set_node_direction(chanx_node, direction);

    // Rib wings are already unidirectional by construction: create_rr_nodes_and_
    // internal_edges_for_one_instance below always calls this with Direction::DEC
    // for the left wing and Direction::INC for the right wing, each a real wire
    // span flowing away from the drive point. Direction::BIDIR is only ever passed
    // for the drive point itself, a degenerate zero-length (x_start == x_end) hub
    // node, not a wire -- so it has no "direction" in any physical sense; BIDIR
    // here just reflects that it's enterable/exitable from either wing.
    short seg_index = 0;
    switch (direction) {
        case Direction::BIDIR:
            seg_index = drive_seg_idx_;
            break;
        case Direction::DEC:
            seg_index = left_seg_idx_;
            break;
        case Direction::INC:
            seg_index = right_seg_idx_;
            break;
        default:
            VTR_ASSERT_MSG(false, "Unidentified direction type for clock rib");
            break;
    }
    rr_graph_builder.set_node_cost_index(chanx_node, RRIndexedDataId(CHANX_COST_INDEX_START + seg_index)); // Actual value set later

    /* Add the node to spatial lookup */
    //auto& rr_graph = (*rr_nodes);
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    /* TODO: Will replace these codes with an API add_node_to_all_locs() of RRGraphBuilder */
    for (int ix = rr_graph.node_xlow(chanx_node); ix <= rr_graph.node_xhigh(chanx_node); ++ix) {
        for (int iy = rr_graph.node_ylow(chanx_node); iy <= rr_graph.node_yhigh(chanx_node); ++iy) {
            rr_graph_builder.node_lookup().add_node(chanx_node, layer, ix, iy, rr_graph.node_type(chanx_node), rr_graph.node_track_num(chanx_node));
        }
    }

    return node_index;
}

void ClockRib::record_tap_locations(unsigned x_start,
                                    unsigned x_end,
                                    unsigned drive_x,
                                    unsigned y,
                                    int drive_node_idx,
                                    int left_rr_node_idx,
                                    int right_rr_node_idx,
                                    ClockRRGraphBuilder& clock_graph) {
    // An increment of 0 (the default when xincr is omitted) means a single,
    // non-repeating tap point at the given offset, mirroring how ClockSwitchGrid
    // treats an unspecified xincr/yincr.
    unsigned x_step = (tap_.increment > 0) ? tap_.increment : x_end - x_start + 1;
    for (unsigned x = x_start + tap_.offset; x <= x_end; x += x_step) {
        // left_rr_node_idx spans [x_start, drive_x - 1], so the split must be at
        // drive_x itself, taken as-is from the caller rather than re-derived here
        // as x_start + drive_.offset: x_start at this point is already the
        // boundary-adjusted start (x_start_orig + x_offset), so re-adding
        // drive_.offset to it overshoots drive_x by x_offset whenever the rib
        // starts at the left device edge, misclassifying the leftmost column(s)
        // of the right node as belonging to left instead.
        if (x < drive_x) {
            clock_graph.add_switch_location(get_name(), tap_.name, x, y, left_rr_node_idx);
        } else if (x > drive_x) {
            clock_graph.add_switch_location(get_name(), tap_.name, x, y, right_rr_node_idx);
        } else {
            clock_graph.add_switch_location(get_name(), tap_.name, x, y, drive_node_idx);
        }
    }
}

/* AA: Map drive_seg_idx, left_seg_idx, and right_seg_idx to equivalent index in segment_inf_x as produced in rr_graph.cpp:build_rr_graph */
void ClockRib::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    // We have horizontal segments in clock-ribs so we search for X_AXIS.
    //
    // Always remap from the unified-space originals (*_seg_idx_unified_), not from
    // drive_seg_idx_/left_seg_idx_/right_seg_idx_ themselves: build_rr_graph (and
    // therefore this function) can run more than once per VPR invocation on this
    // same object, and re-remapping an already-remapped parallel-space index as if
    // it were still unified would silently corrupt it.

    int seg_idx;

    seg_idx = get_parallel_seg_index(drive_seg_idx_unified_, indices_map, e_parallel_axis::X_AXIS);
    drive_seg_idx_ = (seg_idx >= 0) ? seg_idx : drive_seg_idx_unified_;

    seg_idx = get_parallel_seg_index(left_seg_idx_unified_, indices_map, e_parallel_axis::X_AXIS);
    left_seg_idx_ = (seg_idx >= 0) ? seg_idx : left_seg_idx_unified_;

    seg_idx = get_parallel_seg_index(right_seg_idx_unified_, indices_map, e_parallel_axis::X_AXIS);
    right_seg_idx_ = (seg_idx >= 0) ? seg_idx : right_seg_idx_unified_;
}

/*********************************************************************************
 *********************************************************************************
 ********************** ClockSpine Function Implementations **********************
 *********************************************************************************
 *********************************************************************************/

/*
 * ClockSpine (getters)
 */

ClockType ClockSpine::get_network_type() const {
    return ClockType::SPINE;
}

/*
 * ClockSpine (setters)
 */

void ClockSpine::set_metal_layer(float r_metal, float c_metal) {
    y_chan_wire.layer.r_metal = r_metal;
    y_chan_wire.layer.c_metal = c_metal;
}

void ClockSpine::set_metal_layer(MetalLayer metal_layer) {
    y_chan_wire.layer = metal_layer;
}

void ClockSpine::set_initial_wire_location(int start_y, int end_y, int x) {
    if (end_y <= start_y) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Clock Network wire cannot have negative or zero length. "
                        "Wire end: %d < wire start: %d\n",
                        end_y, start_y);
    }

    y_chan_wire.start = start_y;
    y_chan_wire.length = end_y - start_y;
    y_chan_wire.position = x;
}

void ClockSpine::set_wire_repeat(int repeat_x, int repeat_y, int repeat_x_max) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinite loop when creating spines
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
                        repeat_x, repeat_y);
    }

    repeat.x = repeat_x;
    repeat.y = repeat_y;
    repeat.x_max = repeat_x_max;
}

void ClockSpine::set_drive_location(int offset_y) {
    drive.offset = offset_y;
}

void ClockSpine::set_drive_switch(int switch_idx) {
    drive.switch_idx = switch_idx;
}

void ClockSpine::set_drive_name(std::string name) {
    drive.name = name;
}

void ClockSpine::set_tap_locations(int offset_y, int increment_y) {
    tap.offset = offset_y;
    tap.increment = increment_y;
}

void ClockSpine::set_tap_name(std::string name) {
    tap.name = name;
}

/*
 * ClockSpine (member functions)
 */

void ClockSpine::create_segments(std::vector<t_segment_inf>& segment_inf) {
    int index;
    std::string name;
    int length;

    // Drive point segment
    segment_inf.emplace_back();
    drive_seg_idx = segment_inf.size() - 1;

    index = drive_seg_idx;
    name = clock_name_ + "_drive";
    length = 1; // Since drive segment has one length, the left and right segments have length - 1

    /* AA: ClockSpines are assumed to be vertical currently. */
    // See the matching comment in ClockRib::create_segments above: the drive point
    // is a degenerate BIDIR hub, the left/right wings are already unidirectional
    // (DEC/INC) nodes, and segment directionality here matches each accordingly.
    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, e_parallel_axis::Y_AXIS, BI_DIRECTIONAL);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx = segment_inf.size() - 1;

    index = right_seg_idx;
    name = clock_name_ + "_right";
    // Clamped to a minimum of 1 -- see the comment on ClockRib's right segment
    // in ClockRib::create_segments above; the same reasoning applies here.
    length = std::max((y_chan_wire.length - drive.offset) - 1, 1);

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, e_parallel_axis::Y_AXIS, UNI_DIRECTIONAL);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx = segment_inf.size() - 1;

    index = left_seg_idx;
    name = clock_name_ + "_left";
    // Clamped to a minimum of 1 -- see the comment above.
    length = std::max(drive.offset - 1, 1);

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, e_parallel_axis::Y_AXIS, UNI_DIRECTIONAL);

    // Snapshot the unified-space indices for map_relative_seg_indices to remap from.
    drive_seg_idx_unified = drive_seg_idx;
    right_seg_idx_unified = right_seg_idx;
    left_seg_idx_unified = left_seg_idx;
}

size_t ClockSpine::estimate_additional_nodes(const DeviceGrid& grid) {
    size_t num_additional_nodes = 0;

    // Avoid an infinite loop
    VTR_ASSERT(repeat.y > 0);
    VTR_ASSERT(repeat.x > 0);

    unsigned x_tile_end = std::min<unsigned>(grid.width() - 1, repeat.x_max);
    for (unsigned x = y_chan_wire.position; x < x_tile_end; x += repeat.x) {
        for (unsigned y_start = y_chan_wire.start; y_start < grid.height() - 1; y_start += repeat.y) {
            unsigned drive_y = y_start + drive.offset;
            unsigned y_end = y_start + y_chan_wire.length;

            // FIXME: see the perimeter-I/O assumption note on the matching clamp in
            // ClockRib::estimate_additional_nodes above.
            // Adjust for boundary conditions
            unsigned y_offset = 0;
            if ((y_start == 0) ||              // CHANY wires bottom boundary, start above the LB
                (y_start + repeat.y == y_end)) // Avoid overlap
            {
                y_offset = 1;
            }
            if (y_end > grid.height() - 2) {
                y_end = grid.height() - 2; // CHANY wires top boundary, dont go above the LB
            }

            unsigned y_start_adj = y_start + y_offset;

            // Dont create spine if drive point is not reachable
            if (drive_y > grid.width() - 2 || drive_y < y_start_adj || drive_y > y_end) {
                continue;
            }

            // Dont create spine if wire segment is too small
            if (y_start_adj >= y_end) {
                continue;
            }

            num_additional_nodes += 3;
        }
    }

    return num_additional_nodes;
}

void ClockSpine::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                     t_rr_graph_storage* rr_nodes,
                                                                     RRGraphBuilder& rr_graph_builder,
                                                                     t_rr_edge_info_set* rr_edges_to_create,
                                                                     int num_segments_x) {
    auto& grid = clock_graph.grid();

    int ptc_num = clock_graph.get_and_increment_chany_ptc_num(); // used for drawing

    // Avoid an infinite loop
    VTR_ASSERT(repeat.y > 0);
    VTR_ASSERT(repeat.x > 0);

    int layer_num = 0; //Function "FOR NOW" assumes that layer_num is always 0

    unsigned x_tile_end = std::min<unsigned>(grid.width() - 1, repeat.x_max);
    for (unsigned x = y_chan_wire.position; x < x_tile_end; x += repeat.x) {
        for (unsigned y_start = y_chan_wire.start; y_start < grid.height() - 1; y_start += repeat.y) {
            unsigned drive_y = y_start + drive.offset;
            unsigned y_end = y_start + y_chan_wire.length;

            // FIXME: see the perimeter-I/O assumption note on the matching clamp in
            // ClockRib::estimate_additional_nodes above. This is the specific clamp
            // that produces the node-placement quirk ClockToClockConneciton::create_switches
            // (in clock_connection_builders.cpp) works around with its boundary-shift
            // fallback -- that workaround only exists because of this assumption.
            // Adjust for boundary conditions
            unsigned y_offset = 0;
            if ((y_start == 0) ||              // CHANY wires bottom boundary, start above the LB
                (y_start + repeat.y == y_end)) // Avoid overlap
            {
                y_offset = 1;
            }
            if (y_end > grid.height() - 2) {
                y_end = grid.height() - 2; // CHANY wires top boundary, dont go above the LB
            }

            unsigned y_start_adj = y_start + y_offset;

            // Dont create spine if the drive point falls outside the buildable span.
            // The drive point itself is allowed to sit on either endpoint (y_start_adj
            // or y_end): in that case the segment on that side is simply omitted rather
            // than being an inverted/empty range.
            if (drive_y > grid.width() - 2 || drive_y < y_start_adj || drive_y > y_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "A spine part of clock network '%s' was not"
                                    " created because the drive point is not reachable. "
                                    "This can lead to an unroutable architecture.\n",
                                    clock_name_.c_str());
                continue;
            }

            // Dont create spine if wire segment is too small
            if (y_start_adj >= y_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "Spine start '%d' and end '%d' values are "
                                    "not successive for clock network '%s' due to not meeting boundary conditions."
                                    " This can lead to an unroutable architecture.\n",
                                    y_start_adj, y_end, clock_name_.c_str());
                continue;
            }

            bool has_left = drive_y > y_start_adj;
            bool has_right = drive_y < y_end;

            //create drive point (length zero wire)
            auto drive_node_idx = create_chany_wire(layer_num,
                                                    drive_y,
                                                    drive_y,
                                                    x,
                                                    ptc_num,
                                                    Direction::BIDIR,
                                                    rr_nodes,
                                                    rr_graph_builder,
                                                    num_segments_x);
            clock_graph.add_switch_location(get_name(), drive.name, x, drive_y, drive_node_idx);

            // create spine wire above and/or below the drive point. When the drive point
            // is placed at one of the spine's endpoints, that side is skipped entirely
            // instead of being built as a degenerate zero/negative-length wire.
            int left_node_idx = -1;
            if (has_left) {
                left_node_idx = create_chany_wire(layer_num,
                                                  y_start_adj,
                                                  drive_y - 1,
                                                  x,
                                                  ptc_num,
                                                  Direction::DEC,
                                                  rr_nodes,
                                                  rr_graph_builder,
                                                  num_segments_x);
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(left_node_idx), drive.switch_idx, false);
            }

            int right_node_idx = -1;
            if (has_right) {
                right_node_idx = create_chany_wire(layer_num,
                                                   drive_y + 1,
                                                   y_end,
                                                   x,
                                                   ptc_num,
                                                   Direction::INC,
                                                   rr_nodes,
                                                   rr_graph_builder,
                                                   num_segments_x);
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(right_node_idx), drive.switch_idx, false);
            }

            // Keep a record of the rr_node idx that we will use to connects switches to at
            // the tap point
            record_tap_locations(y_start_adj,
                                 y_end,
                                 drive_y,
                                 x,
                                 drive_node_idx,
                                 left_node_idx,
                                 right_node_idx,
                                 clock_graph);
        }
    }
}

int ClockSpine::create_chany_wire(int layer,
                                  int y_start,
                                  int y_end,
                                  int x,
                                  int ptc_num,
                                  Direction direction,
                                  t_rr_graph_storage* rr_nodes,
                                  RRGraphBuilder& rr_graph_builder,
                                  int num_segments_x) {
    rr_nodes->emplace_back();
    auto node_index = rr_nodes->size() - 1;
    RRNodeId chany_node = RRNodeId(node_index);

    rr_graph_builder.set_node_type(chany_node, e_rr_type::CHANY);
    rr_graph_builder.set_node_coordinates(chany_node, x, y_start, x, y_end);
    rr_graph_builder.set_node_layer(chany_node, layer, layer);
    rr_graph_builder.set_node_capacity(chany_node, 1);
    rr_graph_builder.set_node_track_num(chany_node, ptc_num);
    // FIXME: same unscaled Rmetal/Cmetal bug as ClockRib::create_chanx_wire above --
    // see the comment there.
    const NodeRCIndex rc_index = find_create_rr_rc_data(y_chan_wire.layer.r_metal, y_chan_wire.layer.c_metal, g_vpr_ctx.mutable_device().rr_rc_data);
    rr_graph_builder.set_node_rc_index(chany_node, rc_index);
    rr_graph_builder.set_node_direction(chany_node, direction);

    // See the matching comment in ClockRib::create_chanx_wire above: the spine
    // wings are already unidirectional (DEC below the drive point, INC above it);
    // BIDIR is only ever passed for the drive point's own degenerate zero-length
    // hub node.
    short seg_index = 0;
    switch (direction) {
        case Direction::BIDIR:
            seg_index = drive_seg_idx;
            break;
        case Direction::DEC:
            seg_index = left_seg_idx;
            break;
        case Direction::INC:
            seg_index = right_seg_idx;
            break;
        default:
            VTR_ASSERT_MSG(false, "Unidentified direction type for clock rib");
            break;
    }
    rr_graph_builder.set_node_cost_index(chany_node, RRIndexedDataId(CHANX_COST_INDEX_START + num_segments_x + seg_index));

    /* Add the node to spatial lookup */
    //auto& rr_graph = (*rr_nodes);
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    /* TODO: Will replace these codes with an API add_node_to_all_locs() of RRGraphBuilder */
    for (int ix = rr_graph.node_xlow(chany_node); ix <= rr_graph.node_xhigh(chany_node); ++ix) {
        for (int iy = rr_graph.node_ylow(chany_node); iy <= rr_graph.node_yhigh(chany_node); ++iy) {
            rr_graph_builder.node_lookup().add_node(chany_node, layer, ix, iy, rr_graph.node_type(chany_node), rr_graph.node_ptc_num(chany_node));
        }
    }

    return node_index;
}

void ClockSpine::record_tap_locations(unsigned y_start,
                                      unsigned y_end,
                                      unsigned drive_y,
                                      unsigned x,
                                      int drive_node_idx,
                                      int left_node_idx,
                                      int right_node_idx,
                                      ClockRRGraphBuilder& clock_graph) {
    // An increment of 0 (the default when yincr is omitted) means a single,
    // non-repeating tap point at the given offset, mirroring how ClockSwitchGrid
    // treats an unspecified xincr/yincr.
    unsigned y_step = (tap.increment > 0) ? tap.increment : y_end - y_start + 1;
    for (unsigned y = y_start + tap.offset; y <= y_end; y += y_step) {
        // left_node_idx spans [y_start, drive_y - 1], so the split must be at
        // drive_y itself, taken as-is from the caller rather than re-derived here
        // as y_start + drive.offset: y_start at this point is already the
        // boundary-adjusted start (y_start_orig + y_offset), so re-adding
        // drive.offset to it overshoots drive_y by y_offset whenever the spine
        // starts at the bottom device edge, misclassifying the bottommost row(s)
        // of the right node as belonging to left instead.
        if (y < drive_y) {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, left_node_idx);
        } else if (y > drive_y) {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, right_node_idx);
        } else {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, drive_node_idx);
        }
    }
}

/* AA: Map drive_seg_idx, left_seg_idx, and right_seg_idx to equivalent index in segment_inf_y as produced in rr_graph.cpp:build_rr_graph */
void ClockSpine::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    // We have vertical segments in clock-spines so we search for Y_AXIS.
    //
    // Always remap from the unified-space originals (*_seg_idx_unified), not from
    // drive_seg_idx/left_seg_idx/right_seg_idx themselves -- see the longer comment
    // on ClockRib::map_relative_seg_indices.

    int seg_idx;

    seg_idx = get_parallel_seg_index(drive_seg_idx_unified, indices_map, e_parallel_axis::Y_AXIS);
    drive_seg_idx = (seg_idx >= 0) ? seg_idx : drive_seg_idx_unified;

    seg_idx = get_parallel_seg_index(left_seg_idx_unified, indices_map, e_parallel_axis::Y_AXIS);
    left_seg_idx = (seg_idx >= 0) ? seg_idx : left_seg_idx_unified;

    seg_idx = get_parallel_seg_index(right_seg_idx_unified, indices_map, e_parallel_axis::Y_AXIS);
    right_seg_idx = (seg_idx >= 0) ? seg_idx : right_seg_idx_unified;
}

/*********************************************************************************
 *********************************************************************************
 ******************** ClockSwitchGrid Function Implementations *******************
 *********************************************************************************
 *********************************************************************************/

/*
 * ClockSwitchGrid (getters)
 */

ClockType ClockSwitchGrid::get_network_type() const {
    return ClockType::SPINE; // TODO: give ClockSwitchGrid its own ClockType once other code depends on it
}

/*
 * ClockSwitchGrid (setters)
 */

void ClockSwitchGrid::set_metal_layer(float r_metal, float c_metal) {
    layer_.r_metal = r_metal;
    layer_.c_metal = c_metal;
}

void ClockSwitchGrid::set_metal_layer(MetalLayer metal_layer) {
    layer_ = metal_layer;
}

void ClockSwitchGrid::set_grid_start_location(int start_x, int start_y) {
    start_x_ = start_x;
    start_y_ = start_y;
}

void ClockSwitchGrid::set_wire_repeat(int repeat_x, int repeat_y) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinite loop when creating the switch grid
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock switch grid repeat (%d,%d) must be greater than zero\n",
                        repeat_x, repeat_y);
    }

    repeat_.x = repeat_x;
    repeat_.y = repeat_y;
}

void ClockSwitchGrid::set_chan_width(int chan_w) {
    if (chan_w <= 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock switch grid chan_w (%d) must be greater than zero\n", chan_w);
    }

    chan_w_ = chan_w;
}

void ClockSwitchGrid::set_internal_switch(int switch_idx) {
    internal_switch_idx_ = switch_idx;
}

void ClockSwitchGrid::set_switch_block_type(e_switch_block_type switch_block_type) {
    switch_block_type_ = switch_block_type;
}

void ClockSwitchGrid::set_length(int length_hops) {
    VTR_ASSERT(length_hops > 0);
    length_hops_ = length_hops;
}

void ClockSwitchGrid::set_directionality(e_directionality directionality) {
    directionality_ = directionality;
}

void ClockSwitchGrid::add_switch_point(std::string name, SwitchGridPointType type, int x, int y, int switch_idx) {
    SwitchGridPoint point;
    point.name = std::move(name);
    point.type = type;
    point.x = x;
    point.y = y;
    point.switch_idx = switch_idx;

    switch_points_.push_back(std::move(point));
}

/*
 * ClockSwitchGrid (member functions)
 */

void ClockSwitchGrid::create_segments(std::vector<t_segment_inf>& segment_inf) {
    // Segment length is recorded in tiles (like every other segment_inf entry),
    // even though length_hops_ is specified in the arch file in switch-box-hop
    // units: a hop wire physically spans length_hops_ * repeat tiles.
    segment_inf.emplace_back();
    x_seg_idx_ = segment_inf.size() - 1;
    populate_segment_values(x_seg_idx_, clock_name_ + "_x", length_hops_ * repeat_.x, layer_, segment_inf, e_parallel_axis::X_AXIS, directionality_);

    segment_inf.emplace_back();
    y_seg_idx_ = segment_inf.size() - 1;
    populate_segment_values(y_seg_idx_, clock_name_ + "_y", length_hops_ * repeat_.y, layer_, segment_inf, e_parallel_axis::Y_AXIS, directionality_);

    // Snapshot the unified-space indices for map_relative_seg_indices to remap from.
    x_seg_idx_unified_ = x_seg_idx_;
    y_seg_idx_unified_ = y_seg_idx_;
}

int ClockSwitchGrid::num_grid_locations(const DeviceGrid& grid) const {
    VTR_ASSERT(repeat_.x > 0);
    VTR_ASSERT(repeat_.y > 0);

    int x_max = int(grid.width()) - 2;
    int y_max = int(grid.height()) - 2;

    int num_x = (x_max >= start_x_) ? (x_max - start_x_) / repeat_.x + 1 : 0;
    int num_y = (y_max >= start_y_) ? (y_max - start_y_) / repeat_.y + 1 : 0;

    return num_x * num_y;
}

size_t ClockSwitchGrid::estimate_additional_nodes(const DeviceGrid& grid) {
    // Up to 2 hop wire nodes (east, north) per grid location, per track, plus 1 hub
    // node per track at each switch_point location. This over-estimates (locations at
    // the top/right edge have fewer hops), which is fine since this is only used to
    // reserve node storage.
    return size_t(chan_w_) * (size_t(num_grid_locations(grid)) * 2 + switch_points_.size());
}

void ClockSwitchGrid::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                          t_rr_graph_storage* rr_nodes,
                                                                          RRGraphBuilder& rr_graph_builder,
                                                                          t_rr_edge_info_set* rr_edges_to_create,
                                                                          int num_segments_x) {
    const auto& grid = clock_graph.grid();

    VTR_ASSERT(repeat_.x > 0);
    VTR_ASSERT(repeat_.y > 0);

    // TODO: This function is not adapted to the multi-layer grid
    VTR_ASSERT(g_vpr_ctx.device().grid.get_num_layers() == 1);
    int layer_num = 0;

    int x_max = int(grid.width()) - 2;
    int y_max = int(grid.height()) - 2;

    std::vector<bool> switch_point_registered(switch_points_.size(), false);

    // Each node gets its own dedicated ptc. Nodes that touch the same (x,y) location
    // must never share a ptc, since the rr_node_indices reverse lookup is keyed by
    // (x,y,type,ptc) and a collision silently drops one of the nodes.

    // Hop wire rr_node index of the wire leaving a switch box to the east/north,
    // indexed by [track][{x,y}]. The wire leaving (x,y) to the east is the same node
    // as the wire arriving at (x + repeat_.x, y) from the west, and similarly for north.
    std::vector<std::map<std::pair<int, int>, int>> east_wire(chan_w_);
    std::vector<std::map<std::pair<int, int>, int>> north_wire(chan_w_);

    // The Direction a given track's hop wires are built with. BI_DIRECTIONAL grids
    // always use BIDIR (one node per track, enterable/exitable from either end, same
    // as before this was configurable). UNI_DIRECTIONAL grids alternate INC/DEC by
    // track parity, matching rr_graph_chan_seg_details.cpp's general-routing
    // convention; set_directionality (via setup_clocks.cpp) already validated chan_w_
    // is even in that case, so tracks pair up evenly.
    auto track_direction = [&](int track) -> Direction {
        if (directionality_ == BI_DIRECTIONAL) {
            return Direction::BIDIR;
        }
        return (track % 2 == 0) ? Direction::INC : Direction::DEC;
    };

    // Pass 1: create the hop wires themselves (independent of switch-block pattern).
    // A wire spans length_hops_ switch-box pitches, not necessarily just one. Each
    // track's wires are staggered by (track % length_hops_) pitches -- mirroring how
    // alloc_and_load_seg_details staggers general routing segments -- so that across
    // the whole channel, every switch box is still the true endpoint of some track's
    // wire and can participate in wire-to-wire turns; only 1/length_hops_ of the
    // tracks turn at any single switch box.
    for (int track = 0; track < chan_w_; track++) {
        int stagger = track % length_hops_;

        for (int by = start_y_; by <= y_max; by += repeat_.y) {
            for (int k = stagger;; k += length_hops_) {
                int bx = start_x_ + k * repeat_.x;
                int east_x = start_x_ + (k + length_hops_) * repeat_.x;
                if (east_x > x_max) break;

                int wire_ptc = clock_graph.get_and_increment_chanx_ptc_num();
                int wire_idx = create_chanx_node(layer_num, bx, east_x, by, wire_ptc, track_direction(track), rr_nodes, rr_graph_builder);
                east_wire[track][{bx, by}] = wire_idx;
            }
        }

        for (int bx = start_x_; bx <= x_max; bx += repeat_.x) {
            for (int k = stagger;; k += length_hops_) {
                int by = start_y_ + k * repeat_.y;
                int north_y = start_y_ + (k + length_hops_) * repeat_.y;
                if (north_y > y_max) break;

                int wire_ptc = clock_graph.get_and_increment_chany_ptc_num();
                int wire_idx = create_chany_node(layer_num, by, north_y, bx, wire_ptc, track_direction(track), rr_nodes, rr_graph_builder, num_segments_x);
                north_wire[track][{bx, by}] = wire_idx;
            }
        }
    }

    // Returns the rr_node index of the hop wire touching switch box (bx,by) on the
    // given side, for the given track, or -1 if no such wire exists there. With
    // length_hops_ > 1 this only succeeds at a wire's true endpoints (grid boundary,
    // or a switch box that isn't a multiple of length_hops_ pitches from this track's
    // stagger, simply has no wire-to-wire turn available here for this track).
    auto stub_at = [&](int bx, int by, e_side side, int track) -> int {
        const std::map<std::pair<int, int>, int>* wires = nullptr;
        int key_x = bx;
        int key_y = by;
        switch (side) {
            case LEFT:
                key_x = bx - length_hops_ * repeat_.x;
                if (key_x < start_x_) return -1;
                wires = &east_wire[track];
                break;
            case RIGHT:
                wires = &east_wire[track];
                break;
            case BOTTOM:
                key_y = by - length_hops_ * repeat_.y;
                if (key_y < start_y_) return -1;
                wires = &north_wire[track];
                break;
            case TOP:
                wires = &north_wire[track];
                break;
            default:
                VTR_ASSERT_MSG(false, "Unexpected side for clock switch grid");
                return -1;
        }
        auto it = wires->find({key_x, key_y});
        return (it != wires->end()) ? it->second : -1;
    };

    // Returns the rr_node index of the given track's wire (along the given axis)
    // whose physical span covers (bx,by), even when (bx,by) is not a true endpoint
    // of that wire (i.e. stub_at would return -1 on both sides). This lets a
    // switch_point tap into a length_hops_ > 1 wire anywhere along its span, the
    // same way a connection block can tap a multi-tile general routing segment
    // mid-span, independent of where that wire makes switch-block turns.
    auto covering_wire_at = [&](int bx, int by, e_parallel_axis axis, int track) -> int {
        int stagger = track % length_hops_;
        if (axis == e_parallel_axis::X_AXIS) {
            int k = (bx - start_x_) / repeat_.x;
            if (k < stagger) return -1;
            int start_k = stagger + length_hops_ * ((k - stagger) / length_hops_);
            auto it = east_wire[track].find({start_x_ + start_k * repeat_.x, by});
            return (it != east_wire[track].end()) ? it->second : -1;
        } else {
            int k = (by - start_y_) / repeat_.y;
            if (k < stagger) return -1;
            int start_k = stagger + length_hops_ * ((k - stagger) / length_hops_);
            auto it = north_wire[track].find({bx, start_y_ + start_k * repeat_.y});
            return (it != north_wire[track].end()) ? it->second : -1;
        }
    };

    // A unidirectional wire's flow direction relative to the switch box it's being
    // queried from: true if the wire carries a signal away from this box (a valid
    // switch-block/hub "from"/fan-out target), false if it carries a signal into this
    // box (a valid "to"/fan-in source). A box touches a wire on RIGHT/TOP at the
    // wire's low-coordinate end (see east_wire/north_wire above: keyed by the box at
    // that end) and on LEFT/BOTTOM at its high-coordinate end. Only meaningful for
    // INC/DEC tracks -- never called for BI_DIRECTIONAL grids, where every wire is
    // both.
    auto is_outgoing = [](e_side side, Direction dir) -> bool {
        bool low_coord_end = (side == RIGHT || side == TOP);
        bool inc = (dir == Direction::INC);
        return low_coord_end == inc;
    };

    // Pass 2: connect the switch boxes.
    for (int by = start_y_; by <= y_max; by += repeat_.y) {
        for (int bx = start_x_; bx <= x_max; bx += repeat_.x) {
            // Switch points (drive/tap) get a dedicated hub node per track, shared by
            // every switch point at this location, with full access to every wire
            // incident to this switch box: drive/tap points model dedicated
            // clock-network access hardware, not the general switching fabric, so they
            // are intentionally exempt from the switch-block pattern.
            std::vector<size_t> points_here;
            for (size_t i = 0; i < switch_points_.size(); i++) {
                if (switch_points_[i].x == bx && switch_points_[i].y == by) {
                    points_here.push_back(i);
                }
            }

            if (!points_here.empty()) {
                for (int track = 0; track < chan_w_; track++) {
                    int hub_ptc = clock_graph.get_and_increment_chanx_ptc_num();
                    int hub_idx = create_chanx_node(layer_num, bx, bx, by, hub_ptc, Direction::BIDIR, rr_nodes, rr_graph_builder);

                    for (size_t i : points_here) {
                        clock_graph.add_switch_location(get_name(), switch_points_[i].name, bx, by, hub_idx);
                        switch_point_registered[i] = true;
                    }

                    // A shared hub can serve both DRIVE (fan-out onto outgoing wires)
                    // and TAP (fan-in from incoming wires) roles at once: for a
                    // BI_DIRECTIONAL grid every incident wire gets both edges, as
                    // before; for UNI_DIRECTIONAL, each incident wire only gets the
                    // one edge matching its actual flow direction at this box.
                    Direction track_dir = track_direction(track);
                    bool x_tapped = false;
                    bool y_tapped = false;
                    for (e_side side : TOTAL_2D_SIDES) {
                        int stub_idx = stub_at(bx, by, side, track);
                        if (stub_idx < 0) continue;

                        if (side == LEFT || side == RIGHT) {
                            x_tapped = true;
                        } else {
                            y_tapped = true;
                        }

                        if (directionality_ == BI_DIRECTIONAL) {
                            clock_graph.add_edge(rr_edges_to_create, RRNodeId(hub_idx), RRNodeId(stub_idx), internal_switch_idx_, false);
                            clock_graph.add_edge(rr_edges_to_create, RRNodeId(stub_idx), RRNodeId(hub_idx), internal_switch_idx_, false);
                        } else if (is_outgoing(side, track_dir)) {
                            clock_graph.add_edge(rr_edges_to_create, RRNodeId(hub_idx), RRNodeId(stub_idx), internal_switch_idx_, false);
                        } else {
                            clock_graph.add_edge(rr_edges_to_create, RRNodeId(stub_idx), RRNodeId(hub_idx), internal_switch_idx_, false);
                        }
                    }

                    // This switch box isn't a true endpoint of this track's horizontal
                    // and/or vertical wire (only possible once length_hops_ > 1). The
                    // switch_point still needs tap access here even though no
                    // wire-to-wire turn is available at this location for this track.
                    // For UNI_DIRECTIONAL grids this fallback only supports fan-in
                    // (covering wire -> hub): injecting into the middle of a wire's
                    // span would require splitting that wire's node, which isn't
                    // supported, so a DRIVE point that only ever reaches a track via
                    // this fallback silently has no fan-out for that track (the same
                    // partial-coverage tolerance this function already applies to
                    // length_hops_ > 1 stagger gaps elsewhere).
                    if (!x_tapped) {
                        int cov_idx = covering_wire_at(bx, by, e_parallel_axis::X_AXIS, track);
                        if (cov_idx >= 0) {
                            if (directionality_ == BI_DIRECTIONAL) {
                                clock_graph.add_edge(rr_edges_to_create, RRNodeId(hub_idx), RRNodeId(cov_idx), internal_switch_idx_, false);
                            }
                            clock_graph.add_edge(rr_edges_to_create, RRNodeId(cov_idx), RRNodeId(hub_idx), internal_switch_idx_, false);
                        }
                    }
                    if (!y_tapped) {
                        int cov_idx = covering_wire_at(bx, by, e_parallel_axis::Y_AXIS, track);
                        if (cov_idx >= 0) {
                            if (directionality_ == BI_DIRECTIONAL) {
                                clock_graph.add_edge(rr_edges_to_create, RRNodeId(hub_idx), RRNodeId(cov_idx), internal_switch_idx_, false);
                            }
                            clock_graph.add_edge(rr_edges_to_create, RRNodeId(cov_idx), RRNodeId(hub_idx), internal_switch_idx_, false);
                        }
                    }
                }
            }

            // Wire-to-wire connectivity through this switch box follows the
            // configured switch-block pattern.
            for (e_side from_side : TOTAL_2D_SIDES) {
                for (int from_track = 0; from_track < chan_w_; from_track++) {
                    int from_idx = stub_at(bx, by, from_side, from_track);
                    if (from_idx < 0) continue;

                    // A turn's source must be a wire carrying signal into this box.
                    if (directionality_ == UNI_DIRECTIONAL && is_outgoing(from_side, track_direction(from_track))) {
                        continue;
                    }

                    for (e_side to_side : TOTAL_2D_SIDES) {
                        if (to_side == from_side) continue;

                        if (switch_block_type_ == e_switch_block_type::FULL) {
                            // FULL connects every from_track to every to_track (there is
                            // no meaningful single to_track to permute to).
                            for (int to_track = 0; to_track < chan_w_; to_track++) {
                                int to_idx = stub_at(bx, by, to_side, to_track);
                                if (to_idx < 0) continue;
                                // A turn's destination must be a wire carrying signal
                                // away from this box.
                                if (directionality_ == UNI_DIRECTIONAL && !is_outgoing(to_side, track_direction(to_track))) {
                                    continue;
                                }
                                clock_graph.add_edge(rr_edges_to_create, RRNodeId(from_idx), RRNodeId(to_idx), internal_switch_idx_, false);
                            }
                        } else if (directionality_ == BI_DIRECTIONAL) {
                            int to_track = get_simple_switch_block_track(from_side, to_side, from_track,
                                                                         switch_block_type_, chan_w_, chan_w_);
                            if (to_track < 0 || to_track >= chan_w_) continue;

                            int to_idx = stub_at(bx, by, to_side, to_track);
                            if (to_idx >= 0) {
                                clock_graph.add_edge(rr_edges_to_create, RRNodeId(from_idx), RRNodeId(to_idx), internal_switch_idx_, false);
                            }
                        } else {
                            // SUBSET/WILTON/UNIVERSAL for a unidirectional grid: reuse
                            // get_simple_switch_block_track's existing turn geometry at
                            // the track-*pair* level (each INC/DEC pair sharing one
                            // physical lane, chan_w_/2 lanes total -- chan_w_ is
                            // guaranteed even, see the setup_clocks.cpp validation),
                            // then resolve to the specific track of the destination
                            // lane that actually flows away from this box on to_side.
                            int from_lane = from_track / 2;
                            int to_lane = get_simple_switch_block_track(from_side, to_side, from_lane,
                                                                        switch_block_type_, chan_w_ / 2, chan_w_ / 2);
                            if (to_lane < 0 || to_lane >= chan_w_ / 2) continue;

                            int to_track = -1;
                            if (is_outgoing(to_side, track_direction(2 * to_lane))) {
                                to_track = 2 * to_lane;
                            } else if (is_outgoing(to_side, track_direction(2 * to_lane + 1))) {
                                to_track = 2 * to_lane + 1;
                            }
                            if (to_track < 0) continue;

                            int to_idx = stub_at(bx, by, to_side, to_track);
                            if (to_idx >= 0) {
                                clock_graph.add_edge(rr_edges_to_create, RRNodeId(from_idx), RRNodeId(to_idx), internal_switch_idx_, false);
                            }
                        }
                    }
                }
            }
        }
    }

    // Pass 3: connect switch points that sit on a channel between switch boxes,
    // rather than at a switch box location itself. This models clock architectures
    // where a lower-level network (or a complex block's clock pin) branches directly
    // off the channel wire running between two switch boxes, instead of always
    // routing through a full switch box. A channel tap is identified by having
    // exactly one of its (x,y) coordinates aligned to the switch-box lattice (the
    // column/row the channel runs along) with the other coordinate falling within
    // the grid but off the lattice (i.e. strictly between two switch boxes on that
    // channel, or coincident with one -- either way not itself a switch box row/col).
    for (size_t i = 0; i < switch_points_.size(); i++) {
        if (switch_point_registered[i]) continue;

        int x = switch_points_[i].x;
        int y = switch_points_[i].y;
        bool x_on_lattice = (x >= start_x_ && x <= x_max && (x - start_x_) % repeat_.x == 0);
        bool y_on_lattice = (y >= start_y_ && y <= y_max && (y - start_y_) % repeat_.y == 0);
        bool x_in_range = (x >= start_x_ && x <= x_max);
        bool y_in_range = (y >= start_y_ && y <= y_max);

        e_parallel_axis axis;
        if (x_on_lattice && !y_on_lattice && y_in_range) {
            axis = e_parallel_axis::Y_AXIS; // tap into the vertical channel running through column x
        } else if (y_on_lattice && !x_on_lattice && x_in_range) {
            axis = e_parallel_axis::X_AXIS; // tap into the horizontal channel running through row y
        } else {
            continue; // not on any channel; falls through to the warning below
        }

        // A mid-span tap can only receive from the wire it lands on: injecting into
        // the middle of a unidirectional wire's span would require splitting that
        // wire's node, which isn't supported. DRIVE points must therefore sit exactly
        // on the switch-box lattice for a unidirectional grid.
        if (directionality_ == UNI_DIRECTIONAL && switch_points_[i].type == SwitchGridPointType::DRIVE) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Drive point '%s' of unidirectional clock network '%s' at (%d,%d) is not aligned "
                            "to a switch-box location. Unidirectional clock switch grid drive points must sit "
                            "exactly on the switch-box lattice (startx/starty/repeatx/repeaty).\n",
                            switch_points_[i].name.c_str(), clock_name_.c_str(), x, y);
        }

        for (int track = 0; track < chan_w_; track++) {
            int cov_idx = covering_wire_at(x, y, axis, track);
            if (cov_idx < 0) continue; // this track's wire (if staggered) doesn't reach here

            int hub_ptc = clock_graph.get_and_increment_chanx_ptc_num();
            int hub_idx = create_chanx_node(layer_num, x, x, y, hub_ptc, Direction::BIDIR, rr_nodes, rr_graph_builder);

            clock_graph.add_switch_location(get_name(), switch_points_[i].name, x, y, hub_idx);
            switch_point_registered[i] = true;

            if (directionality_ == BI_DIRECTIONAL) {
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(hub_idx), RRNodeId(cov_idx), internal_switch_idx_, false);
            }
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(cov_idx), RRNodeId(hub_idx), internal_switch_idx_, false);
        }
    }

    for (size_t i = 0; i < switch_points_.size(); i++) {
        if (!switch_point_registered[i]) {
            vtr::printf_warning(__FILE__, __LINE__,
                                "Switch point '%s' of clock network '%s' at location (%d,%d) does not"
                                " correspond to any switch box location produced by startx/starty/repeatx/repeaty,"
                                " and is not a valid channel tap (exactly one of its x/y coordinates must be on"
                                " that lattice, with the other coordinate inside the grid)."
                                " This can lead to an unroutable architecture.\n",
                                switch_points_[i].name.c_str(), clock_name_.c_str(),
                                switch_points_[i].x, switch_points_[i].y);
        }
    }
}

int ClockSwitchGrid::create_chanx_node(int layer,
                                       int x_start,
                                       int x_end,
                                       int y,
                                       int ptc_num,
                                       Direction direction,
                                       t_rr_graph_storage* rr_nodes,
                                       RRGraphBuilder& rr_graph_builder) {
    rr_nodes->emplace_back();
    size_t node_index = rr_nodes->size() - 1;
    RRNodeId chanx_node = RRNodeId(node_index);

    rr_graph_builder.set_node_type(chanx_node, e_rr_type::CHANX);
    rr_graph_builder.set_node_coordinates(chanx_node, x_start, y, x_end, y);
    rr_graph_builder.set_node_layer(chanx_node, layer, layer);
    rr_graph_builder.set_node_capacity(chanx_node, 1);
    rr_graph_builder.set_node_track_num(chanx_node, ptc_num);
    // Rmetal/Cmetal are resistance/capacitance PER UNIT LENGTH (per tile spanned,
    // see doc/src/arch/reference.rst), not a flat per-node total. Scale by the
    // node's actual tile span so that increasing repeatx (or length_hops_) genuinely
    // increases wire delay, instead of every hop wire silently getting the same R/C
    // regardless of how many tiles it physically spans. This also naturally gives
    // hub nodes (x_start == x_end) 0 R/C, which is correct since they represent
    // switch hardware rather than metal wire.
    int node_length = x_end - x_start;
    const NodeRCIndex rc_index = find_create_rr_rc_data(layer_.r_metal * node_length, layer_.c_metal * node_length, g_vpr_ctx.mutable_device().rr_rc_data);
    rr_graph_builder.set_node_rc_index(chanx_node, rc_index);
    rr_graph_builder.set_node_direction(chanx_node, direction);
    rr_graph_builder.set_node_cost_index(chanx_node, RRIndexedDataId(CHANX_COST_INDEX_START + x_seg_idx_));

    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    for (int ix = rr_graph.node_xlow(chanx_node); ix <= rr_graph.node_xhigh(chanx_node); ++ix) {
        for (int iy = rr_graph.node_ylow(chanx_node); iy <= rr_graph.node_yhigh(chanx_node); ++iy) {
            rr_graph_builder.node_lookup().add_node(chanx_node, layer, ix, iy, rr_graph.node_type(chanx_node), rr_graph.node_track_num(chanx_node));
        }
    }

    return node_index;
}

int ClockSwitchGrid::create_chany_node(int layer,
                                       int y_start,
                                       int y_end,
                                       int x,
                                       int ptc_num,
                                       Direction direction,
                                       t_rr_graph_storage* rr_nodes,
                                       RRGraphBuilder& rr_graph_builder,
                                       int num_segments_x) {
    rr_nodes->emplace_back();
    size_t node_index = rr_nodes->size() - 1;
    RRNodeId chany_node = RRNodeId(node_index);

    rr_graph_builder.set_node_type(chany_node, e_rr_type::CHANY);
    rr_graph_builder.set_node_coordinates(chany_node, x, y_start, x, y_end);
    rr_graph_builder.set_node_layer(chany_node, layer, layer);
    rr_graph_builder.set_node_capacity(chany_node, 1);
    rr_graph_builder.set_node_track_num(chany_node, ptc_num);
    // See the matching comment in create_chanx_node: Rmetal/Cmetal are per-unit-length,
    // so scale by the node's actual tile span rather than treating them as a flat
    // per-node total.
    int node_length = y_end - y_start;
    const NodeRCIndex rc_index = find_create_rr_rc_data(layer_.r_metal * node_length, layer_.c_metal * node_length, g_vpr_ctx.mutable_device().rr_rc_data);
    rr_graph_builder.set_node_rc_index(chany_node, rc_index);
    rr_graph_builder.set_node_direction(chany_node, direction);
    rr_graph_builder.set_node_cost_index(chany_node, RRIndexedDataId(CHANX_COST_INDEX_START + num_segments_x + y_seg_idx_));

    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    for (int ix = rr_graph.node_xlow(chany_node); ix <= rr_graph.node_xhigh(chany_node); ++ix) {
        for (int iy = rr_graph.node_ylow(chany_node); iy <= rr_graph.node_yhigh(chany_node); ++iy) {
            rr_graph_builder.node_lookup().add_node(chany_node, layer, ix, iy, rr_graph.node_type(chany_node), rr_graph.node_ptc_num(chany_node));
        }
    }

    return node_index;
}

void ClockSwitchGrid::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    // Always remap from the unified-space originals (*_seg_idx_unified_), not from
    // x_seg_idx_/y_seg_idx_ themselves -- see the longer comment on
    // ClockRib::map_relative_seg_indices.

    int seg_idx;

    seg_idx = get_parallel_seg_index(x_seg_idx_unified_, indices_map, e_parallel_axis::X_AXIS);
    x_seg_idx_ = (seg_idx >= 0) ? seg_idx : x_seg_idx_unified_;

    seg_idx = get_parallel_seg_index(y_seg_idx_unified_, indices_map, e_parallel_axis::Y_AXIS);
    y_seg_idx_ = (seg_idx >= 0) ? seg_idx : y_seg_idx_unified_;
}

/*********************************************************************************
 *********************************************************************************
 ********************** ClockHTree Function Implementations **********************
 *********************************************************************************
 *********************************************************************************/

/*
 * ClockHtree (member functions)
 */

//TODO: Implement clock Htree generation code
void ClockHTree::create_segments(std::vector<t_segment_inf>& segment_inf) {
    //Remove unused parameter warning
    (void)segment_inf;

    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "HTrees are not yet supported.\n");
}

size_t ClockHTree::estimate_additional_nodes(const DeviceGrid& /*grid*/) {
    return 0;
}

void ClockHTree::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                     t_rr_graph_storage* rr_nodes,
                                                                     RRGraphBuilder& rr_graph_builder,
                                                                     t_rr_edge_info_set* rr_edges_to_create,
                                                                     int num_segments) {
    //Remove unused parameter warning
    (void)clock_graph;
    (void)num_segments;
    (void)rr_nodes;
    (void)rr_graph_builder;
    (void)rr_edges_to_create;

    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "HTrees are not yet supported.\n");
}
