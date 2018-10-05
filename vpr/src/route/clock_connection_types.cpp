#include "clock_connection_types.h"

#include "globals.h"
#include "rr_graph2.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_error.h"

#include <random>
#include <math.h>

/*
 * RoutingToClockConnection (getters)
 */

ClockConnectionType RoutingToClockConnection::get_connection_type() const {
    return ClockConnectionType::ROUTING_TO_CLOCK;
}

/*
 * RoutingToClockConnection (setters)
 */

void RoutingToClockConnection::set_clock_name_to_connect_to(std::string clock_name) {
    clock_to_connect_to = clock_name;
}

void RoutingToClockConnection::set_clock_switch_name(std::string clock_switch_name) {
    switch_name = clock_switch_name;
}

void RoutingToClockConnection::set_switch_location(int x, int y) {
    switch_location.x = x;
    switch_location.y = y;
}

void RoutingToClockConnection::set_switch(int switch_index) {
    switch_idx = switch_index;
}

void RoutingToClockConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * RoutingToClockConnection (member functions)
 */

void RoutingToClockConnection::create_switches(const ClockRRGraph& clock_graph) {

    // Initialize random seed
    // Must be done durring every call inorder for restored rr_graphs after a binary
    // search to be consistant
    std::srand(seed);

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;

    // rr_node indices for x and y channel routing wires and clock wires to connect to
    auto x_wire_indices = get_chan_wire_indices_at_switch_location(CHANX);
    auto y_wire_indices = get_chan_wire_indices_at_switch_location(CHANY);
    auto clock_indices = get_clock_indices_at_switch_location(clock_graph);

    for(auto clock_index : clock_indices) {

        // Select wires to connect to at random
        std::random_shuffle(x_wire_indices.begin(), x_wire_indices.end());
        std::random_shuffle(y_wire_indices.begin(), y_wire_indices.end());

        // Connect to x-channel wires
        for(size_t i = 0; i < x_wire_indices.size()*fc; i++) {
            rr_nodes[x_wire_indices[i]].add_edge(clock_index, switch_idx);
        }

        // Connect to y-channel wires
        for(size_t i = 0; i < y_wire_indices.size()*fc; i++) {
            rr_nodes[y_wire_indices[i]].add_edge(clock_index, switch_idx);
        }
    }

}

std::vector<int> RoutingToClockConnection::get_chan_wire_indices_at_switch_location(
    t_rr_type rr_type)
{
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_node_indices =  device_ctx.rr_node_indices;

    return get_rr_node_chan_wires_at_location(
        rr_node_indices,
        rr_type,
        switch_location.x,
        switch_location.y);
}

std::vector<int> RoutingToClockConnection::get_clock_indices_at_switch_location(
    const ClockRRGraph& clock_graph)
{

    return clock_graph.get_rr_node_indices_at_switch_location(
        clock_to_connect_to,
        switch_name,
        switch_location.x,
        switch_location.y);
}

/*
 * ClockToClockConneciton (getters)
 */

ClockConnectionType ClockToClockConneciton::get_connection_type() const {
    return ClockConnectionType::CLOCK_TO_CLOCK;
}

/*
 * ClockToClockConneciton (setters)
 */

void ClockToClockConneciton::set_from_clock_name(std::string clock_name) {
    from_clock = clock_name;
}

void ClockToClockConneciton::set_from_clock_switch_name(std::string switch_name) {
    from_switch = switch_name;
}

void ClockToClockConneciton::set_to_clock_name(std::string clock_name) {
    to_clock = clock_name;
}

void ClockToClockConneciton::set_to_clock_switch_name(std::string switch_name) {
    to_switch = switch_name;
}

void ClockToClockConneciton::set_switch(int switch_index) {
    switch_idx = switch_index;
}

void ClockToClockConneciton::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * ClockToClockConneciton (member functions)
 */

void ClockToClockConneciton::create_switches(const ClockRRGraph& clock_graph) {

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& grid = device_ctx.grid;
    auto& rr_nodes = device_ctx.rr_nodes;

    auto to_locations = clock_graph.get_switch_locations(to_clock, to_switch);

    for(auto location : to_locations) {

        auto x = location.first;
        auto y = location.second;

        auto to_indices = clock_graph.get_rr_node_indices_at_switch_location(
            to_clock,
            to_switch,
            x,
            y);

        // boundry conditions:
        // y at gird height and height -1 connections share the same drive point
        if(y == int(grid.height()-2)) {
            y = y-1;
        }
        // y at 0 and y at 1 share the same drive point
        if(y == 0) {
            y = 1;
        }

        auto from_indices = clock_graph.get_rr_node_indices_at_switch_location(
            from_clock,
            from_switch,
            x,
            y);

        auto from_itter = from_indices.begin();
        size_t num_connections = ceil(from_indices.size()*fc);

        for(auto to_index : to_indices) {
            for(size_t i = 0; i < num_connections; i++) {
                if(from_itter == from_indices.end()){
                    from_itter = from_indices.begin();
                }
                rr_nodes[*from_itter].add_edge(to_index, switch_idx);
                from_itter++;
            }
        }
    }
}




/*
 * ClockToPinsConnection (getters)
 */

ClockConnectionType ClockToPinsConnection::get_connection_type() const {
    return ClockConnectionType::CLOCK_TO_PINS;
}

/*
 * ClockToPinsConnection (setters)
 */

void ClockToPinsConnection::set_clock_name_to_connect_from(std::string clock_name) {
    clock_to_connect_from = clock_name;
}

void ClockToPinsConnection::set_clock_switch_name(std::string connection_switch_name) {
    switch_name = connection_switch_name;
}

void ClockToPinsConnection::set_switch(int switch_index) {
    switch_idx = switch_index;
}

void ClockToPinsConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * ClockToPinsConnection (member functions)
 */

void ClockToPinsConnection::create_switches(const ClockRRGraph& clock_graph) {

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& rr_node_indices = device_ctx.rr_node_indices;
    auto& grid = device_ctx.grid;

    for (size_t x = 0; x < grid.width(); x++) {
        for(size_t y = 0; y < grid.height(); y++) {

            //Avoid boundry
            if((y == 0 && x == 0) || (x == grid.width()-1 && y == grid.height()-1)) {
                continue;
            }

            auto type = grid[x][y].type;
            auto width_offset = grid[x][y].width_offset;
            auto height_offset = grid[x][y].height_offset;

            for(e_side side : SIDES) {

                //Don't connect pins which are not adjacent to channels around the perimeter
                if ((x == 0 && side != RIGHT) ||
                    (x == grid.width()-1 && side != LEFT) ||
                    (y == 0 && side != TOP) ||
                    (y == grid.height()-1 && side != BOTTOM))
                {
                    continue;
                }

                for(auto clock_pin_idx : type->get_clock_pins_indices()) {

                    //Can't do anything if pin isn't at this location
                    if (0 == type->pinloc[width_offset][height_offset][side][clock_pin_idx]) {
                        continue;
                    }

                    //Adjust boundry connections (TODO: revisist if chany connections)
                    int clock_x_offset = 0;
                    int clock_y_offset = 0;
                    if(x == 0) {
                        clock_x_offset = 1;  // chanx clock always starts at 1 offset
                        clock_y_offset = -1; // pick the chanx below the block
                    } else if (x == grid.width()-1) {
                        clock_x_offset = -1; // chanx clock always ends at 1 offset
                        clock_y_offset = -1; // pick the chanx below the block
                    } else if (y == 0) { // pick chanx above the block, no offset needed
                    } else if (y == grid.height()-1) {
                        clock_y_offset = -1; // pick the chanx below the block
                    } else {
                        clock_y_offset = -1;
                    }

                    auto clock_pin_node_idx = get_rr_node_index(
                        rr_node_indices,
                        x,
                        y,
                        IPIN,
                        clock_pin_idx,
                        side);
                    
                    auto clock_indices = clock_graph.get_rr_node_indices_at_switch_location(
                        clock_to_connect_from,
                        switch_name,
                        x + clock_x_offset,
                        y + clock_y_offset);

                    //Create edges depending on Fc
                    for(size_t i = 0; i < clock_indices.size()*fc; i++) {
                        rr_nodes[clock_indices[i]].add_edge(clock_pin_node_idx, switch_idx);
                    }
                }
            }
        }
    }
}

