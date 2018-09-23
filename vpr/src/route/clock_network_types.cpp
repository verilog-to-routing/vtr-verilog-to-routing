#include "clock_network_types.h"

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_error.h"

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

void ClockNetwork::create_rr_nodes_for_clock_network_wires(ClockRRGraph& clock_graph) {
    for(int inst_num = 0; inst_num < get_num_inst(); inst_num++){
        create_rr_nodes_for_one_instance(inst_num, clock_graph);
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

void ClockRib::set_metal_layer(int r_metal, int c_metal) {
    x_chan_wire.layer.r_metal = r_metal;
    x_chan_wire.layer.c_metal = c_metal;
}

void ClockRib::set_initial_wire_location(int start_x, int end_x, int y) {
    x_chan_wire.start = start_x;
    x_chan_wire.end = end_x;
    x_chan_wire.position = y;
}

void ClockRib::set_wire_repeat(int repeat_x, int repeat_y) {
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

void ClockRib::create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph) {
 
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& grid = device_ctx.grid;

    int ptc_num = inst_num + 55; // used for drawing

    for(unsigned x_start = x_chan_wire.start, x_end = x_chan_wire.end;
        x_end < grid.width() - 1;
        x_start += repeat.x, x_end += repeat.x) {

        int drive_x = x_start + drive.offset;

        // Adjust for boundry conditions but get the dirve position first so as to not shift
        // switch locations
        if(x_start == 0) {
            x_start = 1;
        }

        for(unsigned y = x_chan_wire.position; y < grid.height() - 1; y += repeat.y) { 

            // create drive point (length zero wire)
            auto drive_node_idx = create_chanx_wire(drive_x, drive_x, y, ptc_num, rr_nodes);
            clock_graph.add_switch_location(get_name(), drive.name, drive_x, y, drive_node_idx);
            
            // create rib wire to the right and left of the drive point 
            auto left_node_idx = create_chanx_wire(x_start, drive_x, y, ptc_num, rr_nodes);
            auto right_node_idx = create_chanx_wire(drive_x, x_end, y, ptc_num, rr_nodes);

            record_tap_locations(x_start, x_end, y, left_node_idx, right_node_idx, clock_graph);
            
            // connect drive point to each half rib using a directed switch
            rr_nodes[drive_node_idx].add_edge(left_node_idx, drive.switch_idx);
            rr_nodes[drive_node_idx].add_edge(right_node_idx, drive.switch_idx);
        }
    }
}

int ClockRib::create_chanx_wire(
    int x_start,
    int x_end,
    int y,
    int ptc_num,
    std::vector<t_rr_node>& rr_nodes)
{
    rr_nodes.emplace_back();
    auto node_index = rr_nodes.size() - 1;

    rr_nodes[node_index].set_coordinates(x_start, y, x_end, y);
    rr_nodes[node_index].set_type(CHANX);
    rr_nodes[node_index].set_capacity(1);
    rr_nodes[node_index].set_ptc_num(ptc_num);

    return node_index;
}

void ClockRib::record_tap_locations(
        unsigned x_start,
        unsigned x_end,
        unsigned y,
        int left_rr_node_idx,
        int right_rr_node_idx,
        ClockRRGraph& clock_graph)
{
    for(unsigned x = x_start+tap.offset; x <= x_end; x+=tap.increment) {
        if(x < x_start + drive.offset) {
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

void ClockSpine::set_metal_layer(int r_metal, int c_metal) {
    y_chan_wire.layer.r_metal = r_metal;
    y_chan_wire.layer.c_metal = c_metal;
}

void ClockSpine::set_initial_wire_location(int start_y, int end_y, int x) {
    y_chan_wire.start = start_y;
    y_chan_wire.end = end_y;
    y_chan_wire.position = x;
}

void ClockSpine::set_wire_repeat(int repeat_x, int repeat_y) {
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

void ClockSpine::create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph) {

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& grid = device_ctx.grid;

    int ptc_num = inst_num + 55;

    for(unsigned y_start = y_chan_wire.start, y_end = y_chan_wire.end;
        y_end < grid.height() - 1;
        y_start += repeat.y, y_end += repeat.y) {

        int drive_y = y_start + drive.offset;

        // Adjust for boundry conditions but get the dirve position first so as to not shift
        // switch locations
        if(y_start == 0) {
            y_start = 1;
        }

        for(unsigned x = y_chan_wire.position; x < grid.width() - 1; x += repeat.x) {

            //create drive point (length zero wire)
            auto drive_node_idx = create_chany_wire(drive_y, drive_y, x, ptc_num, rr_nodes);
            clock_graph.add_switch_location(get_name(), drive.name, x, drive_y, drive_node_idx);

            // create spine wire to the right and left of the drive point
            auto left_node_idx = create_chany_wire(y_start, drive_y, x, ptc_num, rr_nodes);
            auto right_node_idx = create_chany_wire(drive_y, y_end, x, ptc_num, rr_nodes);

            record_tap_locations(y_start, y_end, x, left_node_idx, right_node_idx, clock_graph);

            // connect drive point to each half spine using a directed switch
            rr_nodes[drive_node_idx].add_edge(left_node_idx, drive.switch_idx);
            rr_nodes[drive_node_idx].add_edge(right_node_idx, drive.switch_idx);
        }
    }
}

int ClockSpine::create_chany_wire(
    int y_start,
    int y_end,
    int x,
    int ptc_num,
    std::vector<t_rr_node>& rr_nodes)
{
    rr_nodes.emplace_back();
    auto node_index = rr_nodes.size() - 1;

    rr_nodes[node_index].set_coordinates(x, y_start, x, y_end);
    rr_nodes[node_index].set_type(CHANY);
    rr_nodes[node_index].set_capacity(1);
    rr_nodes[node_index].set_ptc_num(ptc_num);

    return node_index;
}

void ClockSpine::record_tap_locations(
        unsigned y_start,
        unsigned y_end,
        unsigned x,
        int left_node_idx,
        int right_node_idx,
        ClockRRGraph& clock_graph)
{
    for (unsigned y = y_start+tap.offset; y <= y_end; y+=tap.increment) {
        if(y < y_start + drive.offset) {
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
void ClockHTree::create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph) {

    //Remove unused parameter warning
    (void)inst_num;
    (void)clock_graph; 

    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "HTrees are not yet supported.\n");
}

