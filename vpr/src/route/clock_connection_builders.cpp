#include "clock_connection_builders.h"

#include "globals.h"
#include "rr_graph2.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_error.h"

#include <random>
#include <math.h>

/*
 * RoutingToClockConnection (setters)
 */

void RoutingToClockConnection::set_clock_name_to_connect_to(std::string clock_name) {
    clock_to_connect_to = clock_name;
}

void RoutingToClockConnection::set_clock_switch_point_name(std::string clock_switch_point_name) {
    switch_point_name = clock_switch_point_name;
}

void RoutingToClockConnection::set_switch_location(int x, int y) {
    switch_location.x = x;
    switch_location.y = y;
}

void RoutingToClockConnection::set_switch(int rr_switch_index) {
    rr_switch_idx = rr_switch_index;
}

void RoutingToClockConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * RoutingToClockConnection (member functions)
 */

void RoutingToClockConnection::create_switches(const ClockRRGraphBuilder& clock_graph) {
    // Initialize random seed
    // Must be done durring every call inorder for restored rr_graphs after a binary
    // search to be consistant
    std::srand(seed);

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& rr_node_indices = device_ctx.rr_node_indices;

    // rr_node indices for x and y channel routing wires and clock wires to connect to
    auto x_wire_indices = get_rr_node_chan_wires_at_location(
        rr_node_indices, CHANX, switch_location.x, switch_location.y);
    auto y_wire_indices = get_rr_node_chan_wires_at_location(
        rr_node_indices, CHANY, switch_location.x, switch_location.y);
    auto clock_indices = clock_graph.get_rr_node_indices_at_switch_location(
        clock_to_connect_to, switch_point_name, switch_location.x, switch_location.y);

    for (auto clock_index : clock_indices) {
        // Select wires to connect to at random
        std::random_shuffle(x_wire_indices.begin(), x_wire_indices.end());
        std::random_shuffle(y_wire_indices.begin(), y_wire_indices.end());

        // Connect to x-channel wires
        unsigned num_wires_x = x_wire_indices.size() * fc;
        for (size_t i = 0; i < num_wires_x; i++) {
            rr_nodes[x_wire_indices[i]].add_edge(clock_index, rr_switch_idx);
        }

        // Connect to y-channel wires
        unsigned num_wires_y = y_wire_indices.size() * fc;
        for (size_t i = 0; i < num_wires_y; i++) {
            rr_nodes[y_wire_indices[i]].add_edge(clock_index, rr_switch_idx);
        }
    }
}

/*
 * ClockToClockConneciton (setters)
 */

void ClockToClockConneciton::set_from_clock_name(std::string clock_name) {
    from_clock = clock_name;
}

void ClockToClockConneciton::set_from_clock_switch_point_name(std::string switch_point_name) {
    from_switch = switch_point_name;
}

void ClockToClockConneciton::set_to_clock_name(std::string clock_name) {
    to_clock = clock_name;
}

void ClockToClockConneciton::set_to_clock_switch_point_name(std::string switch_point_name) {
    to_switch = switch_point_name;
}

void ClockToClockConneciton::set_switch(int rr_switch_index) {
    rr_switch_idx = rr_switch_index;
}

void ClockToClockConneciton::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * ClockToClockConneciton (member functions)
 */

void ClockToClockConneciton::create_switches(const ClockRRGraphBuilder& clock_graph) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& grid = device_ctx.grid;
    auto& rr_nodes = device_ctx.rr_nodes;

    auto to_locations = clock_graph.get_switch_locations(to_clock, to_switch);

    for (auto location : to_locations) {
        auto x = location.first;
        auto y = location.second;

        auto to_rr_node_indices = clock_graph.get_rr_node_indices_at_switch_location(
            to_clock,
            to_switch,
            x,
            y);

        // boundry conditions:
        // y at gird height and height -1 connections share the same drive point
        if (y == int(grid.height() - 2)) {
            y = y - 1;
        }
        // y at 0 and y at 1 share the same drive point
        if (y == 0) {
            y = 1;
        }

        auto from_rr_node_indices = clock_graph.get_rr_node_indices_at_switch_location(
            from_clock,
            from_switch,
            x,
            y);

        auto from_itter = from_rr_node_indices.begin();
        size_t num_connections = ceil(from_rr_node_indices.size() * fc);

        // Create a one to one connection from each chanx wire to the chany wire
        // or vice versa. If there are more chanx wire than chany wire or vice versa
        // then wrap around and start a one to one connection starting with the first node.
        // This ensures that each wire gets a connection.
        for (auto to_index : to_rr_node_indices) {
            for (size_t i = 0; i < num_connections; i++) {
                if (from_itter == from_rr_node_indices.end()) {
                    from_itter = from_rr_node_indices.begin();
                }
                rr_nodes[*from_itter].add_edge(to_index, rr_switch_idx);
                from_itter++;
            }
        }
    }
}

/*
 * ClockToPinsConnection (setters)
 */

void ClockToPinsConnection::set_clock_name_to_connect_from(std::string clock_name) {
    clock_to_connect_from = clock_name;
}

void ClockToPinsConnection::set_clock_switch_point_name(
    std::string connection_switch_point_name) {
    switch_point_name = connection_switch_point_name;
}

void ClockToPinsConnection::set_switch(int rr_switch_index) {
    rr_switch_idx = rr_switch_index;
}

void ClockToPinsConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * ClockToPinsConnection (member functions)
 */

void ClockToPinsConnection::create_switches(const ClockRRGraphBuilder& clock_graph) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& rr_node_indices = device_ctx.rr_node_indices;
    auto& grid = device_ctx.grid;

    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            //Avoid boundry
            if ((y == 0 && x == 0) || (x == grid.width() - 1 && y == grid.height() - 1)) {
                continue;
            }

            auto type = grid[x][y].type;
            auto width_offset = grid[x][y].width_offset;
            auto height_offset = grid[x][y].height_offset;

            // Ignore gird locations that do not have blocks
            if (!type->pb_type) {
                continue;
            }

            for (e_side side : SIDES) {
                //Don't connect pins which are not adjacent to channels around the perimeter
                if ((x == 0 && side != RIGHT) || (x == grid.width() - 1 && side != LEFT) || (y == 0 && side != TOP) || (y == grid.height() - 1 && side != BOTTOM)) {
                    continue;
                }

                for (auto clock_pin_idx : type->get_clock_pins_indices()) {
                    //Can't do anything if pin isn't at this location
                    if (0 == type->pinloc[width_offset][height_offset][side][clock_pin_idx]) {
                        continue;
                    }

                    //Adjust boundry connections (TODO: revisist if chany connections)
                    int clock_x_offset = 0;
                    int clock_y_offset = 0;
                    if (x == 0) {
                        clock_x_offset = 1;  // chanx clock always starts at 1 offset
                        clock_y_offset = -1; // pick the chanx below the block
                    } else if (x == grid.width() - 1) {
                        clock_x_offset = -1; // chanx clock always ends at 1 offset
                        clock_y_offset = -1; // pick the chanx below the block
                    } else if (y == 0) {
                        clock_y_offset = 0; // pick chanx above the block, no offset needed
                    } else {
                        clock_y_offset = -1; // pick the chanx below the block
                    }

                    auto clock_pin_node_idx = get_rr_node_index(
                        rr_node_indices,
                        x,
                        y,
                        IPIN,
                        clock_pin_idx,
                        side);

                    auto clock_network_indices = clock_graph.get_rr_node_indices_at_switch_location(
                        clock_to_connect_from,
                        switch_point_name,
                        x + clock_x_offset,
                        y + clock_y_offset);

                    //Create edges depending on Fc
                    for (size_t i = 0; i < clock_network_indices.size() * fc; i++) {
                        rr_nodes[clock_network_indices[i]].add_edge(clock_pin_node_idx, rr_switch_idx);
                    }
                }
            }
        }
    }
}
