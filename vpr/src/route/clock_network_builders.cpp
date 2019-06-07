#include "clock_network_builders.h"

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_error.h"

void static populate_segment_values(int seg_index,
                                    std::string name,
                                    int length,
                                    MetalLayer layer,
                                    std::vector<t_segment_inf>& segment_inf);

void populate_segment_values(int seg_index,
                             std::string name,
                             int length,
                             MetalLayer layer,
                             std::vector<t_segment_inf>& segment_inf) {
    segment_inf[seg_index].name = name;
    segment_inf[seg_index].length = length;
    segment_inf[seg_index].frequency = 1;
    segment_inf[seg_index].Rmetal = layer.r_metal;
    segment_inf[seg_index].Cmetal = layer.c_metal;
    segment_inf[seg_index].directionality = UNI_DIRECTIONAL;
    segment_inf[seg_index].longline = false;

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
                                                           int num_segments) {
    for (int inst_num = 0; inst_num < get_num_inst(); inst_num++) {
        create_rr_nodes_and_internal_edges_for_one_instance(clock_graph, num_segments);
    }
}

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
        VPR_THROW(VPR_ERROR_ROUTE,
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
        VPR_THROW(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
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

    populate_segment_values(index, name, length, x_chan_wire.layer, segment_inf);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx = segment_inf.size() - 1;

    index = right_seg_idx;
    name = clock_name_ + "_right";
    length = (x_chan_wire.length - drive.offset) - 1;

    populate_segment_values(index, name, length, x_chan_wire.layer, segment_inf);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx = segment_inf.size() - 1;

    index = left_seg_idx;
    name = clock_name_ + "_left";
    length = drive.offset - 1;

    populate_segment_values(index, name, length, x_chan_wire.layer, segment_inf);
}

void ClockRib::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                   int num_segments) {
    // Only chany wires need to know the number of segments inorder
    // to calculate the cost_index
    (void)num_segments;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& grid = device_ctx.grid;

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
                                                    BI_DIRECTION,
                                                    rr_nodes);
            clock_graph.add_switch_location(get_name(), drive.name, drive_x, y, drive_node_idx);

            // create rib wire to the right and left of the drive point
            auto left_node_idx = create_chanx_wire(x_start + x_offset,
                                                   drive_x - 1,
                                                   y,
                                                   ptc_num,
                                                   DEC_DIRECTION,
                                                   rr_nodes);
            auto right_node_idx = create_chanx_wire(drive_x + 1,
                                                    x_end,
                                                    y,
                                                    ptc_num,
                                                    INC_DIRECTION,
                                                    rr_nodes);
            record_tap_locations(x_start + x_offset,
                                 x_end,
                                 y,
                                 left_node_idx,
                                 right_node_idx,
                                 clock_graph);

            // connect drive point to each half rib using a directed switch
            rr_nodes[drive_node_idx].add_edge(left_node_idx, drive.switch_idx);
            rr_nodes[drive_node_idx].add_edge(right_node_idx, drive.switch_idx);
        }
    }
}

int ClockRib::create_chanx_wire(int x_start,
                                int x_end,
                                int y,
                                int ptc_num,
                                e_direction direction,
                                std::vector<t_rr_node>& rr_nodes) {
    rr_nodes.emplace_back();
    auto node_index = rr_nodes.size() - 1;

    rr_nodes[node_index].set_coordinates(x_start, y, x_end, y);
    rr_nodes[node_index].set_type(CHANX);
    rr_nodes[node_index].set_capacity(1);
    rr_nodes[node_index].set_track_num(ptc_num);
    auto rc_index = find_create_rr_rc_data(x_chan_wire.layer.r_metal, x_chan_wire.layer.c_metal);
    rr_nodes[node_index].set_rc_index(rc_index);
    rr_nodes[node_index].set_direction(direction);

    short seg_index = 0;
    switch (direction) {
        case BI_DIRECTION:
            seg_index = drive_seg_idx;
            break;
        case DEC_DIRECTION:
            seg_index = left_seg_idx;
            break;
        case INC_DIRECTION:
            seg_index = right_seg_idx;
            break;
        default:
            VTR_ASSERT_MSG(false, "Unidentified direction type for clock rib");
            break;
    }
    rr_nodes[node_index].set_cost_index(CHANX_COST_INDEX_START + seg_index); // Actual value set later

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
        VPR_THROW(VPR_ERROR_ROUTE,
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
        VPR_THROW(VPR_ERROR_ROUTE, "Clock Network wire repeat (%d,%d) must be greater than zero\n",
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

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf);

    // Segment to the right of the drive point
    segment_inf.emplace_back();
    right_seg_idx = segment_inf.size() - 1;

    index = right_seg_idx;
    name = clock_name_ + "_right";
    length = (y_chan_wire.length - drive.offset) - 1;

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf);

    // Segment to the left of the drive point
    segment_inf.emplace_back();
    left_seg_idx = segment_inf.size() - 1;

    index = left_seg_idx;
    name = clock_name_ + "_left";
    length = drive.offset - 1;

    populate_segment_values(index, name, length, y_chan_wire.layer, segment_inf);
}

void ClockSpine::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                     int num_segments) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& grid = device_ctx.grid;

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
                                                    BI_DIRECTION,
                                                    rr_nodes,
                                                    num_segments);
            clock_graph.add_switch_location(get_name(), drive.name, x, drive_y, drive_node_idx);

            // create spine wire above and below the drive point
            auto left_node_idx = create_chany_wire(y_start + y_offset,
                                                   drive_y - 1,
                                                   x,
                                                   ptc_num,
                                                   DEC_DIRECTION,
                                                   rr_nodes,
                                                   num_segments);
            auto right_node_idx = create_chany_wire(drive_y + 1,
                                                    y_end,
                                                    x,
                                                    ptc_num,
                                                    INC_DIRECTION,
                                                    rr_nodes,
                                                    num_segments);

            // Keep a record of the rr_node idx that we will use to connects switches to at
            // the tap point
            record_tap_locations(y_start + y_offset,
                                 y_end,
                                 x,
                                 left_node_idx,
                                 right_node_idx,
                                 clock_graph);

            // connect drive point to each half spine using a directed switch
            rr_nodes[drive_node_idx].add_edge(left_node_idx, drive.switch_idx);
            rr_nodes[drive_node_idx].add_edge(right_node_idx, drive.switch_idx);
        }
    }
}

int ClockSpine::create_chany_wire(int y_start,
                                  int y_end,
                                  int x,
                                  int ptc_num,
                                  e_direction direction,
                                  std::vector<t_rr_node>& rr_nodes,
                                  int num_segments) {
    rr_nodes.emplace_back();
    auto node_index = rr_nodes.size() - 1;

    rr_nodes[node_index].set_coordinates(x, y_start, x, y_end);
    rr_nodes[node_index].set_type(CHANY);
    rr_nodes[node_index].set_capacity(1);
    rr_nodes[node_index].set_track_num(ptc_num);
    auto rc_index = find_create_rr_rc_data(y_chan_wire.layer.r_metal, y_chan_wire.layer.c_metal);
    rr_nodes[node_index].set_rc_index(rc_index);
    rr_nodes[node_index].set_direction(direction);

    short seg_index = 0;
    switch (direction) {
        case BI_DIRECTION:
            seg_index = drive_seg_idx;
            break;
        case DEC_DIRECTION:
            seg_index = left_seg_idx;
            break;
        case INC_DIRECTION:
            seg_index = right_seg_idx;
            break;
        default:
            VTR_ASSERT_MSG(false, "Unidentified direction type for clock rib");
            break;
    }
    rr_nodes[node_index].set_cost_index(CHANX_COST_INDEX_START + num_segments + seg_index);

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

/*
 * ClockHtree (member funtions)
 */

//TODO: Implement clock Htree generation code
void ClockHTree::create_segments(std::vector<t_segment_inf>& segment_inf) {
    //Remove unused parameter warning
    (void)segment_inf;

    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "HTrees are not yet supported.\n");
}
void ClockHTree::create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                     int num_segments) {
    //Remove unused parameter warning
    (void)clock_graph;
    (void)num_segments;

    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "HTrees are not yet supported.\n");
}
