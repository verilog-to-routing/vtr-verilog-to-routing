#include "clock_connection_builders.h"

#include "globals.h"
#include "arch_util.h"
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

void RoutingToClockConnection::set_switch(int arch_switch_index) {
    arch_switch_idx = arch_switch_index;
}

void RoutingToClockConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * RoutingToClockConnection (member functions)
 */

size_t RoutingToClockConnection::estimate_additional_nodes() {
    // 1 rr node is being added as the virtual clock sink.
    return 1;
}

void RoutingToClockConnection::create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) {
    // Initialize random seed
    // Must be done during every call in order for restored rr_graphs after a binary
    // search to be consistent
    std::srand(seed);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& node_lookup = device_ctx.rr_graph.node_lookup();

    RRNodeId virtual_clock_network_root_idx = create_virtual_clock_network_sink_node(switch_location.x, switch_location.y);
    {
        auto& mut_device_ctx = g_vpr_ctx.mutable_device();
        mut_device_ctx.virtual_clock_network_root_idx = size_t(virtual_clock_network_root_idx);
    }

    // rr_node indices for x and y channel routing wires and clock wires to connect to
    auto x_wire_indices = node_lookup.find_channel_nodes(switch_location.x, switch_location.y, CHANX);
    auto y_wire_indices = node_lookup.find_channel_nodes(switch_location.x, switch_location.y, CHANY);
    auto clock_indices = clock_graph.get_rr_node_indices_at_switch_location(
        clock_to_connect_to, switch_point_name, switch_location.x, switch_location.y);

    for (auto clock_index : clock_indices) {
        // Select wires to connect to at random
        std::random_shuffle(x_wire_indices.begin(), x_wire_indices.end());
        std::random_shuffle(y_wire_indices.begin(), y_wire_indices.end());

        // Connect to x-channel wires
        unsigned num_wires_x = x_wire_indices.size() * fc;
        for (size_t i = 0; i < num_wires_x; i++) {
            clock_graph.add_edge(rr_edges_to_create, x_wire_indices[i], RRNodeId(clock_index), arch_switch_idx);
        }

        // Connect to y-channel wires
        unsigned num_wires_y = y_wire_indices.size() * fc;
        for (size_t i = 0; i < num_wires_y; i++) {
            clock_graph.add_edge(rr_edges_to_create, y_wire_indices[i], RRNodeId(clock_index), arch_switch_idx);
        }

        // Connect to virtual clock sink node
        // used by the two stage router
        clock_graph.add_edge(rr_edges_to_create, RRNodeId(clock_index), virtual_clock_network_root_idx, arch_switch_idx);
    }
}

RRNodeId RoutingToClockConnection::create_virtual_clock_network_sink_node(int x, int y) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_graph = device_ctx.rr_graph;
    auto& rr_graph_builder = device_ctx.rr_graph_builder;
    auto& node_lookup = device_ctx.rr_graph_builder.node_lookup();
    rr_graph_builder.emplace_back();
    RRNodeId node_index = RRNodeId(rr_graph.num_nodes() - 1);

    //Determine the a valid PTC
    std::vector<RRNodeId> nodes_at_loc = node_lookup.find_grid_nodes_at_all_sides(x, y, SINK);

    int max_ptc = 0;
    for (RRNodeId inode : nodes_at_loc) {
        max_ptc = std::max<int>(max_ptc, rr_graph.node_class_num(inode));
    }
    int ptc = max_ptc + 1;

    rr_graph_builder.set_node_type(node_index, SINK);
    rr_graph_builder.set_node_class_num(node_index, ptc);
    rr_graph_builder.set_node_coordinates(node_index, x, y, x, y);
    rr_graph_builder.set_node_capacity(node_index, 1);
    rr_graph_builder.set_node_cost_index(node_index, RRIndexedDataId(SINK_COST_INDEX));

    float R = 0.;
    float C = 0.;
    rr_graph_builder.set_node_rc_index(node_index, NodeRCIndex(find_create_rr_rc_data(R, C)));

    // Use a generic way when adding nodes to lookup.
    // However, since the SINK node has the same xhigh/xlow as well as yhigh/ylow, we can probably use a shortcut
    for (int ix = rr_graph.node_xlow(node_index); ix <= rr_graph.node_xhigh(node_index); ++ix) {
        for (int iy = rr_graph.node_ylow(node_index); iy <= rr_graph.node_yhigh(node_index); ++iy) {
            node_lookup.add_node(node_index, ix, iy, rr_graph.node_type(node_index), rr_graph.node_class_num(node_index));
        }
    }

    return node_index;
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

void ClockToClockConneciton::set_switch(int arch_switch_index) {
    arch_switch_idx = arch_switch_index;
}

void ClockToClockConneciton::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * ClockToClockConneciton (member functions)
 */

size_t ClockToClockConneciton::estimate_additional_nodes() {
    return 0;
}

void ClockToClockConneciton::create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) {
    auto& grid = clock_graph.grid();

    auto to_locations = clock_graph.get_switch_locations(to_clock, to_switch);

    for (auto location : to_locations) {
        auto x = location.first;
        auto y = location.second;

        auto to_rr_node_indices = clock_graph.get_rr_node_indices_at_switch_location(
            to_clock,
            to_switch,
            x,
            y);

        // boundary conditions:
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
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(*from_itter), RRNodeId(to_index), arch_switch_idx);
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

void ClockToPinsConnection::set_switch(int arch_switch_index) {
    arch_switch_idx = arch_switch_index;
}

void ClockToPinsConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * ClockToPinsConnection (member functions)
 */

size_t ClockToPinsConnection::estimate_additional_nodes() {
    return 0;
}

void ClockToPinsConnection::create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& node_lookup = device_ctx.rr_graph.node_lookup();
    auto& grid = clock_graph.grid();

    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            //Avoid boundary
            if ((y == 0 && x == 0) || (x == grid.width() - 1 && y == grid.height() - 1)) {
                continue;
            }

            auto type = grid[x][y].type;

            // Skip EMPTY type
            if (is_empty_type(type)) {
                continue;
            }

            auto width_offset = grid[x][y].width_offset;
            auto height_offset = grid[x][y].height_offset;

            // Ignore grid locations that do not have blocks
            bool has_pb_type = false;
            auto equivalent_sites = get_equivalent_sites_set(type);
            for (auto logical_block : equivalent_sites) {
                if (logical_block->pb_type) {
                    has_pb_type = true;
                    break;
                }
            }

            if (!has_pb_type) {
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

                    //Adjust boundary connections (TODO: revisit if chany connections)
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

                    auto clock_pin_node_idx = node_lookup.find_node(x,
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
                        clock_graph.add_edge(rr_edges_to_create, RRNodeId(clock_network_indices[i]), RRNodeId(clock_pin_node_idx), arch_switch_idx);
                    }
                }
            }
        }
    }
}
