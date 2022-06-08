#include "clock_network_builders.h"

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_error.h"

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
    x_chan_wire.layer.r_metal = r_metal;
    x_chan_wire.layer.c_metal = c_metal;
}

void ClockRib::set_metal_layer(MetalLayer metal_layer) {
    x_chan_wire.layer = metal_layer;
}

void ClockRib::set_initial_wire_location(int start_x, int end_x, int y) {
    if (end_x <= start_x) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Clock Network wire cannot have negtive or zero length. "
                        "Wire end: %d < wire start: %d\n",
                        end_x, start_x);
    }

    x_chan_wire.start = start_x;
    x_chan_wire.length = end_x - start_x;
    x_chan_wire.position = y;
}

void ClockRib::set_wire_repeat(int repeat_x, int repeat_y) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinte loop when creating ribs
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
                        repeat_x, repeat_y);
    }

    repeat.x = repeat_x;
    repeat.y = repeat_y;
}

void ClockRib::set_drive_location(int offset_x) {
    drive.offset = offset_x;
}

void ClockRib::set_drive_switch(int switch_idx) {
    drive.switch_idx = switch_idx;
}

void ClockRib::set_drive_name(std::string name) {
    drive.name = name;
}

void ClockRib::set_tap_locations(int offset_x, int increment_x) {
    tap.offset = offset_x;
    tap.increment = increment_x;
}

void ClockRib::set_tap_name(std::string name) {
    tap.name = name;
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
    drive_seg_idx = segment_inf.size() - 1;

    index = drive_seg_idx;
    name = clock_name_ + "_drive";
    length = 1; // Since drive segment has one length, the left and right segments have length - 1

    /*AA: ClockRibs are assumed to be horizontal currently. */

    populate_segment_values(index, name, length, x_chan_wire.layer, segment_inf, X_AXIS);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx = segment_inf.size() - 1;

    index = right_seg_idx;
    name = clock_name_ + "_right";
    length = (x_chan_wire.length - drive.offset) - 1;

    populate_segment_values(index, name, length, x_chan_wire.layer, segment_inf, X_AXIS);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx = segment_inf.size() - 1;

    index = left_seg_idx;
    name = clock_name_ + "_left";
    length = drive.offset - 1;

    populate_segment_values(index, name, length, x_chan_wire.layer, segment_inf, X_AXIS);
}

size_t ClockRib::estimate_additional_nodes(const DeviceGrid& grid) {
    // Avoid an infinite loop
    VTR_ASSERT(repeat.y > 0);
    VTR_ASSERT(repeat.x > 0);

    size_t num_additional_nodes = 0;
    for (unsigned y = x_chan_wire.position; y < grid.height() - 1; y += repeat.y) {
        for (unsigned x_start = x_chan_wire.start; x_start < grid.width() - 1; x_start += repeat.x) {
            unsigned drive_x = x_start + drive.offset;
            unsigned x_end = x_start + x_chan_wire.length;

            // Adjust for boundry conditions
            int x_offset = 0;
            if ((x_start == 0) ||              // CHANX wires left boundry
                (x_start + repeat.x == x_end)) // Avoid overlap
            {
                x_offset = 1;
            }
            if (x_end > grid.width() - 2) {
                x_end = grid.width() - 2; // CHANX wires right boundry
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
    VTR_ASSERT(repeat.y > 0);
    VTR_ASSERT(repeat.x > 0);

    for (unsigned y = x_chan_wire.position; y < grid.height() - 1; y += repeat.y) {
        for (unsigned x_start = x_chan_wire.start; x_start < grid.width() - 1; x_start += repeat.x) {
            unsigned drive_x = x_start + drive.offset;
            unsigned x_end = x_start + x_chan_wire.length;

            // Adjust for boundry conditions
            int x_offset = 0;
            if ((x_start == 0) ||              // CHANX wires left boundry
                (x_start + repeat.x == x_end)) // Avoid overlap
            {
                x_offset = 1;
            }
            if (x_end > grid.width() - 2) {
                x_end = grid.width() - 2; // CHANX wires right boundry
            }

            // Dont create rib if drive point is not reachable
            if (drive_x > grid.width() - 2 || drive_x >= x_end || drive_x <= (x_start + x_offset)) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "A rib part of clock network '%s' was not"
                                    " created becuase the drive point is not reachable. "
                                    "This can lead to an unroutable architecture.\n",
                                    clock_name_.c_str());
                continue;
            }

            // Dont create rib if wire segment is too small
            if ((x_start + x_offset) >= x_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "Rib start '%d' and end '%d' values are "
                                    "not sucessive for clock network '%s' due to not meeting boundry conditions."
                                    " This can lead to an unroutable architecture.\n",
                                    (x_start + x_offset), x_end, clock_name_.c_str());
                continue;
            }

            // create drive point (length zero wire)
            auto drive_node_idx = create_chanx_wire(drive_x,
                                                    drive_x,
                                                    y,
                                                    ptc_num,
                                                    Direction::BIDIR,
                                                    rr_nodes,
                                                    rr_graph_builder);
            clock_graph.add_switch_location(get_name(), drive.name, drive_x, y, drive_node_idx);

            // create rib wire to the right and left of the drive point
            auto left_node_idx = create_chanx_wire(x_start + x_offset,
                                                   drive_x - 1,
                                                   y,
                                                   ptc_num,
                                                   Direction::DEC,
                                                   rr_nodes,
                                                   rr_graph_builder);
            auto right_node_idx = create_chanx_wire(drive_x + 1,
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
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(left_node_idx), drive.switch_idx);
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(right_node_idx), drive.switch_idx);
        }
    }
}

int ClockRib::create_chanx_wire(int x_start,
                                int x_end,
                                int y,
                                int ptc_num,
                                Direction direction,
                                t_rr_graph_storage* rr_nodes,
                                RRGraphBuilder& rr_graph_builder) {
    rr_nodes->emplace_back();
    auto node_index = rr_nodes->size() - 1;
    RRNodeId chanx_node = RRNodeId(node_index);

    rr_graph_builder.set_node_type(chanx_node, CHANX);
    rr_graph_builder.set_node_coordinates(chanx_node, x_start, y, x_end, y);
    rr_graph_builder.set_node_capacity(chanx_node, 1);
    rr_graph_builder.set_node_track_num(chanx_node, ptc_num);
    rr_graph_builder.set_node_rc_index(chanx_node, NodeRCIndex(find_create_rr_rc_data(
                                                       x_chan_wire.layer.r_metal, x_chan_wire.layer.c_metal)));
    rr_graph_builder.set_node_direction(chanx_node, direction);

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
    rr_graph_builder.set_node_cost_index(chanx_node, RRIndexedDataId(CHANX_COST_INDEX_START + seg_index)); // Actual value set later

    /* Add the node to spatial lookup */
    //auto& rr_graph = (*rr_nodes);
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    /* TODO: Will replace these codes with an API add_node_to_all_locs() of RRGraphBuilder */
    for (int ix = rr_graph.node_xlow(chanx_node); ix <= rr_graph.node_xhigh(chanx_node); ++ix) {
        for (int iy = rr_graph.node_ylow(chanx_node); iy <= rr_graph.node_yhigh(chanx_node); ++iy) {
            //TODO: CHANX uses odd swapped x/y indices here. Will rework once rr_node_indices is shadowed
            rr_graph_builder.node_lookup().add_node(chanx_node, iy, ix, rr_graph.node_type(chanx_node), rr_graph.node_track_num(chanx_node));
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
    for (unsigned x = x_start + tap.offset; x <= x_end; x += tap.increment) {
        if (x < (x_start + drive.offset - 1)) {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, left_rr_node_idx);
        } else {
            clock_graph.add_switch_location(get_name(), tap.name, x, y, right_rr_node_idx);
        }
    }
}

static void get_parallel_seg_index(int& unified_seg_index, enum e_parallel_axis axis, const t_unified_to_parallel_seg_index& indices_map) {
    auto itr_pair = indices_map.equal_range(unified_seg_index);

    for (auto itr = itr_pair.first; itr != itr_pair.second; ++itr) {
        if (itr->second.second == axis) {
            unified_seg_index = itr->second.first;
            return;
        }
    }
}

/* AA: Map drive_seg_idx, left_seg_idx, and right_seg_idx to equivalent index in segment_inf_x as produced in rr_graph.cpp:build_rr_graph */
void ClockRib::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    /*We have horizontal segments in clock-ribs so we search for X_AXIS*/

    get_parallel_seg_index(drive_seg_idx, X_AXIS, indices_map);

    get_parallel_seg_index(left_seg_idx, X_AXIS, indices_map);

    get_parallel_seg_index(right_seg_idx, X_AXIS, indices_map);
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
                        "Clock Network wire cannot have negtive or zero length. "
                        "Wire end: %d < wire start: %d\n",
                        end_y, start_y);
    }

    y_chan_wire.start = start_y;
    y_chan_wire.length = end_y - start_y;
    y_chan_wire.position = x;
}

void ClockSpine::set_wire_repeat(int repeat_x, int repeat_y) {
    if (repeat_x <= 0 || repeat_y <= 0) {
        // Avoid an infinte loop when creating spines
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
    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, Y_AXIS);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx = segment_inf.size() - 1;

    index = right_seg_idx;
    name = clock_name_ + "_right";
    length = (y_chan_wire.length - drive.offset) - 1;

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, Y_AXIS);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx = segment_inf.size() - 1;

    index = left_seg_idx;
    name = clock_name_ + "_left";
    length = drive.offset - 1;

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf, Y_AXIS);
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

            // Adjust for boundry conditions
            unsigned y_offset = 0;
            if ((y_start == 0) ||              // CHANY wires bottom boundry, start above the LB
                (y_start + repeat.y == y_end)) // Avoid overlap
            {
                y_offset = 1;
            }
            if (y_end > grid.height() - 2) {
                y_end = grid.height() - 2; // CHANY wires top boundry, dont go above the LB
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

    for (unsigned x = y_chan_wire.position; x < grid.width() - 1; x += repeat.x) {
        for (unsigned y_start = y_chan_wire.start; y_start < grid.height() - 1; y_start += repeat.y) {
            unsigned drive_y = y_start + drive.offset;
            unsigned y_end = y_start + y_chan_wire.length;

            // Adjust for boundry conditions
            unsigned y_offset = 0;
            if ((y_start == 0) ||              // CHANY wires bottom boundry, start above the LB
                (y_start + repeat.y == y_end)) // Avoid overlap
            {
                y_offset = 1;
            }
            if (y_end > grid.height() - 2) {
                y_end = grid.height() - 2; // CHANY wires top boundry, dont go above the LB
            }

            // Dont create spine if drive point is not reachable
            if (drive_y > grid.width() - 2 || drive_y >= y_end || drive_y <= (y_start + y_offset)) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "A spine part of clock network '%s' was not"
                                    " created becuase the drive point is not reachable. "
                                    "This can lead to an unroutable architecture.\n",
                                    clock_name_.c_str());
                continue;
            }

            // Dont create spine if wire segment is too small
            if ((y_start + y_offset) >= y_end) {
                vtr::printf_warning(__FILE__, __LINE__,
                                    "Spine start '%d' and end '%d' values are "
                                    "not sucessive for clock network '%s' due to not meeting boundry conditions."
                                    " This can lead to an unroutable architecture.\n",
                                    (y_start + y_offset), y_end, clock_name_.c_str());
                continue;
            }

            //create drive point (length zero wire)
            auto drive_node_idx = create_chany_wire(drive_y,
                                                    drive_y,
                                                    x,
                                                    ptc_num,
                                                    Direction::BIDIR,
                                                    rr_nodes,
                                                    rr_graph_builder,
                                                    num_segments_x);
            clock_graph.add_switch_location(get_name(), drive.name, x, drive_y, drive_node_idx);

            // create spine wire above and below the drive point
            auto left_node_idx = create_chany_wire(y_start + y_offset,
                                                   drive_y - 1,
                                                   x,
                                                   ptc_num,
                                                   Direction::DEC,
                                                   rr_nodes,
                                                   rr_graph_builder,
                                                   num_segments_x);
            auto right_node_idx = create_chany_wire(drive_y + 1,
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
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(left_node_idx), drive.switch_idx);
            clock_graph.add_edge(rr_edges_to_create, RRNodeId(drive_node_idx), RRNodeId(right_node_idx), drive.switch_idx);
        }
    }
}

int ClockSpine::create_chany_wire(int y_start,
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

    rr_graph_builder.set_node_type(chany_node, CHANY);
    rr_graph_builder.set_node_coordinates(chany_node, x, y_start, x, y_end);
    rr_graph_builder.set_node_capacity(chany_node, 1);
    rr_graph_builder.set_node_track_num(chany_node, ptc_num);
    rr_graph_builder.set_node_rc_index(chany_node, NodeRCIndex(find_create_rr_rc_data(
                                                       y_chan_wire.layer.r_metal, y_chan_wire.layer.c_metal)));
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
            rr_graph_builder.node_lookup().add_node(chany_node, ix, iy, rr_graph.node_type(chany_node), rr_graph.node_ptc_num(chany_node));
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
    /*We have vertical segments in clock-spines so we search for Y_AXIS*/

    get_parallel_seg_index(drive_seg_idx, Y_AXIS, indices_map);

    get_parallel_seg_index(left_seg_idx, Y_AXIS, indices_map);

    get_parallel_seg_index(right_seg_idx, Y_AXIS, indices_map);
}

/*********************************************************************************
 *********************************************************************************
 ********************** ClockHTree Function Implementations **********************
 *********************************************************************************
 *********************************************************************************/

/*
 * ClockHtree (member funtions)
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
