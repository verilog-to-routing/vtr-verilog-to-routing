#include "clock_network_builders.h"

#include "globals.h"

#include "get_parallel_segs.h"
#include "rr_graph_sbox.h"
#include "rr_rc_data.h"
#include "vtr_assert.h"
#include "vtr_log.h"

void static populate_segment_values(int seg_index,
                                    std::string name,
                                    int length,
                                    MetalLayer layer,
                                    std::vector<t_segment_inf>& segment_inf,
                                    e_parallel_axis parallel_axis);

void populate_segment_values(int seg_index,
                             std::string name,
                             int length,
                             MetalLayer layer,
                             std::vector<t_segment_inf>& segment_inf,
                             e_parallel_axis parallel_axis) {
    segment_inf[seg_index].name = name;
    segment_inf[seg_index].length = length;
    segment_inf[seg_index].frequency = 1;
    segment_inf[seg_index].Rmetal = layer.r_metal;
    segment_inf[seg_index].Cmetal = layer.c_metal;
    segment_inf[seg_index].directionality = UNI_DIRECTIONAL;
    segment_inf[seg_index].longline = false;
    segment_inf[seg_index].parallel_axis = parallel_axis;

    segment_inf[seg_index].seg_index = seg_index;

    // unused values tagged with -1 (only used RR graph creation)
    segment_inf[seg_index].arch_wire_switch = -1;
    segment_inf[seg_index].arch_opin_switch = -1;
    segment_inf[seg_index].frac_cb = -1;
    segment_inf[seg_index].frac_sb = -1;
    segment_inf[seg_index].res_type = SegResType::GCLK;
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

void ClockRib::set_wire_repeat(int repeat_x, int repeat_y) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinite loop when creating ribs
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
                        repeat_x, repeat_y);
    }

    repeat_.x = repeat_x;
    repeat_.y = repeat_y;
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

    populate_segment_values(index, name, length, x_chan_wire_.layer, segment_inf, e_parallel_axis::X_AXIS);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx_ = segment_inf.size() - 1;

    index = right_seg_idx_;
    name = clock_name_ + "_right";
    length = (x_chan_wire_.length - drive_.offset) - 1;

    populate_segment_values(index, name, length, x_chan_wire_.layer, segment_inf, e_parallel_axis::X_AXIS);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx_ = segment_inf.size() - 1;

    index = left_seg_idx_;
    name = clock_name_ + "_left";
    length = drive_.offset - 1;

    populate_segment_values(index, name, length, x_chan_wire_.layer, segment_inf, e_parallel_axis::X_AXIS);
}

size_t ClockRib::estimate_additional_nodes(const DeviceGrid& grid) {
    // Avoid an infinite loop
    VTR_ASSERT(repeat_.y > 0);
    VTR_ASSERT(repeat_.x > 0);

    size_t num_additional_nodes = 0;
    for (unsigned y = x_chan_wire_.position; y < grid.height() - 1; y += repeat_.y) {
        for (unsigned x_start = x_chan_wire_.start; x_start < grid.width() - 1; x_start += repeat_.x) {
            unsigned drive_x = x_start + drive_.offset;
            unsigned x_end = x_start + x_chan_wire_.length;

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

            // Dont create rib if drive point is not reachable
            if (drive_x > grid.width() - 2 || drive_x >= x_end || drive_x <= (x_start + x_offset)) {
                continue;
            }

            // Dont create rib if wire segment is too small
            if ((x_start + x_offset) >= x_end) {
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

    for (unsigned y = x_chan_wire_.position; y < grid.height() - 1; y += repeat_.y) {
        for (unsigned x_start = x_chan_wire_.start; x_start < grid.width() - 1; x_start += repeat_.x) {
            unsigned drive_x = x_start + drive_.offset;
            unsigned x_end = x_start + x_chan_wire_.length;

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

            // Dont create rib if drive point is not reachable
            if (drive_x > grid.width() - 2 || drive_x >= x_end || drive_x <= (x_start + x_offset)) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "A rib part of clock network '%s' was not"
                                    " created because the drive point is not reachable. "
                                    "This can lead to an unroutable architecture.\n",
                                    clock_name_.c_str());
                continue;
            }

            // Dont create rib if wire segment is too small
            if ((x_start + x_offset) >= x_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "Rib start '%d' and end '%d' values are "
                                    "not successive for clock network '%s' due to not meeting boundary conditions."
                                    " This can lead to an unroutable architecture.\n",
                                    (x_start + x_offset), x_end, clock_name_.c_str());
                continue;
            }

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

            // create rib wire to the right and left of the drive point
            auto left_node_idx = create_chanx_wire(layer_num,
                                                   x_start + x_offset,
                                                   drive_x - 1,
                                                   y,
                                                   ptc_num,
                                                   Direction::DEC,
                                                   rr_nodes,
                                                   rr_graph_builder);
            auto right_node_idx = create_chanx_wire(layer_num,
                                                    drive_x + 1,
                                                    x_end,
                                                    y,
                                                    ptc_num,
                                                    Direction::INC,
                                                    rr_nodes,
                                                    rr_graph_builder);
            record_tap_locations(x_start + x_offset,
                                 x_end,
                                 y,
                                 left_node_idx,
                                 right_node_idx,
                                 clock_graph);

            // connect drive point to each half rib using a directed switch
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(left_node_idx), drive_.switch_idx, false);
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(right_node_idx), drive_.switch_idx, false);
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
    const NodeRCIndex rc_index = find_create_rr_rc_data(x_chan_wire_.layer.r_metal, x_chan_wire_.layer.c_metal, g_vpr_ctx.mutable_device().rr_rc_data);
    rr_graph_builder.set_node_rc_index(chanx_node, rc_index);
    rr_graph_builder.set_node_direction(chanx_node, direction);

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
                                    unsigned y,
                                    int left_rr_node_idx,
                                    int right_rr_node_idx,
                                    ClockRRGraphBuilder& clock_graph) {
    for (unsigned x = x_start + tap_.offset; x <= x_end; x += tap_.increment) {
        if (x < (x_start + drive_.offset - 1)) {
            clock_graph.add_switch_location(get_name(), tap_.name, x, y, left_rr_node_idx);
        } else {
            clock_graph.add_switch_location(get_name(), tap_.name, x, y, right_rr_node_idx);
        }
    }
}

/* AA: Map drive_seg_idx, left_seg_idx, and right_seg_idx to equivalent index in segment_inf_x as produced in rr_graph.cpp:build_rr_graph */
void ClockRib::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    // We have horizontal segments in clock-ribs so we search for X_AXIS

    int seg_idx;

    seg_idx = get_parallel_seg_index(drive_seg_idx_, indices_map, e_parallel_axis::X_AXIS);
    drive_seg_idx_ = (seg_idx >= 0) ? seg_idx : drive_seg_idx_;

    seg_idx = get_parallel_seg_index(left_seg_idx_, indices_map, e_parallel_axis::X_AXIS);
    left_seg_idx_ = (seg_idx >= 0) ? seg_idx : left_seg_idx_;

    seg_idx = get_parallel_seg_index(right_seg_idx_, indices_map, e_parallel_axis::X_AXIS);
    right_seg_idx_ = (seg_idx >= 0) ? seg_idx : right_seg_idx_;
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

void ClockSpine::set_wire_repeat(int repeat_x, int repeat_y) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinite loop when creating spines
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
                        repeat_x, repeat_y);
    }

    repeat.x = repeat_x;
    repeat.y = repeat_y;
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
    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, e_parallel_axis::Y_AXIS);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx = segment_inf.size() - 1;

    index = right_seg_idx;
    name = clock_name_ + "_right";
    length = (y_chan_wire.length - drive.offset) - 1;

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, e_parallel_axis::Y_AXIS);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx = segment_inf.size() - 1;

    index = left_seg_idx;
    name = clock_name_ + "_left";
    length = drive.offset - 1;

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, e_parallel_axis::Y_AXIS);
}

size_t ClockSpine::estimate_additional_nodes(const DeviceGrid& grid) {
    size_t num_additional_nodes = 0;

    // Avoid an infinite loop
    VTR_ASSERT(repeat.y > 0);
    VTR_ASSERT(repeat.x > 0);

    for (unsigned x = y_chan_wire.position; x < grid.width() - 1; x += repeat.x) {
        for (unsigned y_start = y_chan_wire.start; y_start < grid.height() - 1; y_start += repeat.y) {
            unsigned drive_y = y_start + drive.offset;
            unsigned y_end = y_start + y_chan_wire.length;

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

            // Dont create spine if drive point is not reachable
            if (drive_y > grid.width() - 2 || drive_y >= y_end || drive_y <= (y_start + y_offset)) {
                continue;
            }

            // Dont create spine if wire segment is too small
            if ((y_start + y_offset) >= y_end) {
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

    for (unsigned x = y_chan_wire.position; x < grid.width() - 1; x += repeat.x) {
        for (unsigned y_start = y_chan_wire.start; y_start < grid.height() - 1; y_start += repeat.y) {
            unsigned drive_y = y_start + drive.offset;
            unsigned y_end = y_start + y_chan_wire.length;

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

            // Dont create spine if drive point is not reachable
            if (drive_y > grid.width() - 2 || drive_y >= y_end || drive_y <= (y_start + y_offset)) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "A spine part of clock network '%s' was not"
                                    " created because the drive point is not reachable. "
                                    "This can lead to an unroutable architecture.\n",
                                    clock_name_.c_str());
                continue;
            }

            // Dont create spine if wire segment is too small
            if ((y_start + y_offset) >= y_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "Spine start '%d' and end '%d' values are "
                                    "not successive for clock network '%s' due to not meeting boundary conditions."
                                    " This can lead to an unroutable architecture.\n",
                                    (y_start + y_offset), y_end, clock_name_.c_str());
                continue;
            }

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

            // create spine wire above and below the drive point
            auto left_node_idx = create_chany_wire(layer_num,
                                                   y_start + y_offset,
                                                   drive_y - 1,
                                                   x,
                                                   ptc_num,
                                                   Direction::DEC,
                                                   rr_nodes,
                                                   rr_graph_builder,
                                                   num_segments_x);
            auto right_node_idx = create_chany_wire(layer_num,
                                                    drive_y + 1,
                                                    y_end,
                                                    x,
                                                    ptc_num,
                                                    Direction::INC,
                                                    rr_nodes,
                                                    rr_graph_builder,
                                                    num_segments_x);

            // Keep a record of the rr_node idx that we will use to connects switches to at
            // the tap point
            record_tap_locations(y_start + y_offset,
                                 y_end,
                                 x,
                                 left_node_idx,
                                 right_node_idx,
                                 clock_graph);

            // connect drive point to each half spine using a directed switch
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(left_node_idx), drive.switch_idx, false);
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(right_node_idx), drive.switch_idx, false);
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
    const NodeRCIndex rc_index = find_create_rr_rc_data(y_chan_wire.layer.r_metal, y_chan_wire.layer.c_metal, g_vpr_ctx.mutable_device().rr_rc_data);
    rr_graph_builder.set_node_rc_index(chany_node, rc_index);
    rr_graph_builder.set_node_direction(chany_node, direction);

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
                                      unsigned x,
                                      int left_node_idx,
                                      int right_node_idx,
                                      ClockRRGraphBuilder& clock_graph) {
    for (unsigned y = y_start + tap.offset; y <= y_end; y += tap.increment) {
        if (y < (y_start + drive.offset - 1)) {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, left_node_idx);
        } else {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, right_node_idx);
        }
    }
}

/* AA: Map drive_seg_idx, left_seg_idx, and right_seg_idx to equivalent index in segment_inf_y as produced in rr_graph.cpp:build_rr_graph */
void ClockSpine::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    // We have vertical segments in clock-spines so we search for Y_AXIS

    int seg_idx;

    seg_idx = get_parallel_seg_index(drive_seg_idx, indices_map, e_parallel_axis::Y_AXIS);
    drive_seg_idx = (seg_idx >= 0) ? seg_idx : drive_seg_idx;

    seg_idx = get_parallel_seg_index(left_seg_idx, indices_map, e_parallel_axis::Y_AXIS);
    left_seg_idx = (seg_idx >= 0) ? seg_idx : left_seg_idx;

    seg_idx = get_parallel_seg_index(right_seg_idx, indices_map, e_parallel_axis::Y_AXIS);
    right_seg_idx = (seg_idx >= 0) ? seg_idx : right_seg_idx;
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
    segment_inf.emplace_back();
    x_seg_idx_ = segment_inf.size() - 1;
    populate_segment_values(x_seg_idx_, clock_name_ + "_x", repeat_.x, layer_, segment_inf, e_parallel_axis::X_AXIS);

    segment_inf.emplace_back();
    y_seg_idx_ = segment_inf.size() - 1;
    populate_segment_values(y_seg_idx_, clock_name_ + "_y", repeat_.y, layer_, segment_inf, e_parallel_axis::Y_AXIS);
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

    // Pass 1: create the hop wires themselves (independent of switch-block pattern).
    for (int track = 0; track < chan_w_; track++) {
        for (int by = start_y_; by <= y_max; by += repeat_.y) {
            for (int bx = start_x_; bx <= x_max; bx += repeat_.x) {
                int east_x = bx + repeat_.x;
                if (east_x <= x_max) {
                    int wire_ptc = clock_graph.get_and_increment_chanx_ptc_num();
                    int wire_idx = create_chanx_node(layer_num, bx, east_x, by, wire_ptc, Direction::BIDIR, rr_nodes, rr_graph_builder);
                    east_wire[track][{bx, by}] = wire_idx;
                }

                int north_y = by + repeat_.y;
                if (north_y <= y_max) {
                    int wire_ptc = clock_graph.get_and_increment_chany_ptc_num();
                    int wire_idx = create_chany_node(layer_num, by, north_y, bx, wire_ptc, Direction::BIDIR, rr_nodes, rr_graph_builder, num_segments_x);
                    north_wire[track][{bx, by}] = wire_idx;
                }
            }
        }
    }

    // Returns the rr_node index of the hop wire touching switch box (bx,by) on the
    // given side, for the given track, or -1 if no such wire exists (grid boundary).
    auto stub_at = [&](int bx, int by, e_side side, int track) -> int {
        const std::map<std::pair<int, int>, int>* wires = nullptr;
        int key_x = bx;
        int key_y = by;
        switch (side) {
            case LEFT:
                key_x = bx - repeat_.x;
                if (key_x < start_x_) return -1;
                wires = &east_wire[track];
                break;
            case RIGHT:
                wires = &east_wire[track];
                break;
            case BOTTOM:
                key_y = by - repeat_.y;
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

                    for (e_side side : TOTAL_2D_SIDES) {
                        int stub_idx = stub_at(bx, by, side, track);
                        if (stub_idx < 0) continue;

                        clock_graph.add_edge(rr_edges_to_create, RRNodeId(hub_idx), RRNodeId(stub_idx), internal_switch_idx_, false);
                        clock_graph.add_edge(rr_edges_to_create, RRNodeId(stub_idx), RRNodeId(hub_idx), internal_switch_idx_, false);
                    }
                }
            }

            // Wire-to-wire connectivity through this switch box follows the
            // configured switch-block pattern.
            for (e_side from_side : TOTAL_2D_SIDES) {
                for (int from_track = 0; from_track < chan_w_; from_track++) {
                    int from_idx = stub_at(bx, by, from_side, from_track);
                    if (from_idx < 0) continue;

                    for (e_side to_side : TOTAL_2D_SIDES) {
                        if (to_side == from_side) continue;

                        if (switch_block_type_ == e_switch_block_type::FULL) {
                            // FULL connects every from_track to every to_track (there is
                            // no meaningful single to_track to permute to).
                            for (int to_track = 0; to_track < chan_w_; to_track++) {
                                int to_idx = stub_at(bx, by, to_side, to_track);
                                if (to_idx >= 0) {
                                    clock_graph.add_edge(rr_edges_to_create, RRNodeId(from_idx), RRNodeId(to_idx), internal_switch_idx_, false);
                                }
                            }
                        } else {
                            int to_track = get_simple_switch_block_track(from_side, to_side, from_track,
                                                                         switch_block_type_, chan_w_, chan_w_);
                            if (to_track < 0 || to_track >= chan_w_) continue;

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

    for (size_t i = 0; i < switch_points_.size(); i++) {
        if (!switch_point_registered[i]) {
            vtr::printf_warning(__FILE__, __LINE__,
                                "Switch point '%s' of clock network '%s' at location (%d,%d) does not"
                                " correspond to any switch box location produced by startx/starty/repeatx/repeaty."
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
    const NodeRCIndex rc_index = find_create_rr_rc_data(layer_.r_metal, layer_.c_metal, g_vpr_ctx.mutable_device().rr_rc_data);
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
    const NodeRCIndex rc_index = find_create_rr_rc_data(layer_.r_metal, layer_.c_metal, g_vpr_ctx.mutable_device().rr_rc_data);
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
    int seg_idx;

    seg_idx = get_parallel_seg_index(x_seg_idx_, indices_map, e_parallel_axis::X_AXIS);
    x_seg_idx_ = (seg_idx >= 0) ? seg_idx : x_seg_idx_;

    seg_idx = get_parallel_seg_index(y_seg_idx_, indices_map, e_parallel_axis::Y_AXIS);
    y_seg_idx_ = (seg_idx >= 0) ? seg_idx : y_seg_idx_;
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
