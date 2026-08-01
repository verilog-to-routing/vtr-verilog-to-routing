#include "clock_connection_builders.h"

#include "globals.h"
#include "arch_util.h"
#include "rr_rc_data.h"
#include "vpr_utils.h"
#include "physical_types_util.h"
#include "vpr_error.h"

#include <random>
#include <cmath>
#include <utility>

/*
 * RoutingToClockConnection (setters)
 */

void RoutingToClockConnection::set_clock_name_to_connect_to(std::string clock_name) {
    clock_to_connect_to = std::move(clock_name);
}

void RoutingToClockConnection::set_clock_switch_point_name(std::string clock_switch_point_name) {
    switch_point_name = std::move(clock_switch_point_name);
}

void RoutingToClockConnection::set_switch_location(int x, int y, int layer /* =0 */) {
    switch_location.x = x;
    switch_location.y = y;
    switch_location.layer_num = layer;
}

void RoutingToClockConnection::set_virtual_sink_location(int x, int y, int layer /* =0 */) {
    virtual_sink_location.x = x;
    virtual_sink_location.y = y;
    virtual_sink_location.layer_num = layer;
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
    // Up to 1 rr node is added as the virtual clock sink (shared across all drive
    // points of a clock network, so most calls add 0; this is just a capacity
    // estimate, so overcounting here is harmless).
    return 1;
}

static RRNodeId create_virtual_clock_network_sink_node(int layer, int x, int y) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_graph = device_ctx.rr_graph;
    auto& rr_graph_builder = device_ctx.rr_graph_builder;
    auto& node_lookup = device_ctx.rr_graph_builder.node_lookup();
    auto& rr_rc_data = device_ctx.rr_rc_data;
    auto& arch = device_ctx.arch;
    rr_graph_builder.emplace_back();
    RRNodeId node_index = RRNodeId(rr_graph.num_nodes() - 1);

    //Determine a valid PTC
    std::vector<RRNodeId> nodes_at_loc = node_lookup.find_grid_nodes_at_all_sides(layer, x, y, e_rr_type::SINK);

    int max_ptc = 0;
    for (RRNodeId inode : nodes_at_loc) {
        max_ptc = std::max<int>(max_ptc, rr_graph.node_class_num(inode));
    }
    int ptc = max_ptc + 1;

    rr_graph_builder.set_node_type(node_index, e_rr_type::SINK);
    rr_graph_builder.set_node_name(node_index, arch->default_clock_network_name);
    rr_graph_builder.set_node_class_num(node_index, ptc);
    rr_graph_builder.set_node_coordinates(node_index, x, y, x, y);
    rr_graph_builder.set_node_layer(node_index, layer, layer);
    rr_graph_builder.set_node_capacity(node_index, 1);
    rr_graph_builder.set_node_cost_index(node_index, RRIndexedDataId(SINK_COST_INDEX));

    const NodeRCIndex rc_index = find_create_rr_rc_data(0, 0, rr_rc_data);
    rr_graph_builder.set_node_rc_index(node_index, rc_index);

    // Use a generic way when adding nodes to lookup.
    // However, since the SINK node has the same xhigh/xlow as well as yhigh/ylow, we can probably use a shortcut
    for (int ix = rr_graph.node_xlow(node_index); ix <= rr_graph.node_xhigh(node_index); ++ix) {
        for (int iy = rr_graph.node_ylow(node_index); iy <= rr_graph.node_yhigh(node_index); ++iy) {
            node_lookup.add_node(node_index, layer, ix, iy, rr_graph.node_type(node_index), rr_graph.node_class_num(node_index));
        }
    }

    return node_index;
}

// A clock network can have multiple drive points feeding it (e.g. one ROUTING tap per
// device side, and/or one or more TILE taps -- see TileToClockConnection). They all
// share a single virtual sink node in the RR graph (see is_virtual_clock_network_root's
// doc comment), so that stage-1 routing of a global net (pre_route_to_clock_root) can
// reach the clock network through whichever drive point turns out to be cheapest,
// instead of only the one processed first, and so --two_stage_clock_routing works
// regardless of how a given clock network is driven. Looks up whether that shared node
// already exists before creating a new one; if this is the first drive point processed
// for the network, creates it now, at the given location (the centroid of all of the
// network's drive points -- see the drive_location_sum/drive_location_count computation
// in setup_clock_connections) rather than any single drive point's own location.
//
// FIXME: the router lookahead estimates remaining cost using only the shared sink
// node's fixed (x,y) location, not the true distance to whichever drive point is
// actually closest to a given expansion node. For drive points away from the centroid
// this makes the heuristic inadmissible (it can overestimate the true remaining cost),
// biasing stage-1 routing away from the closest drive point instead of toward it.
// Routing stays correct -- Dijkstra still finds a valid path using exact edge costs --
// but the search may waste effort and/or fail to minimize insertion delay across drive
// points. Fixing this properly requires teaching the lookahead about multiple targets
// per sink (e.g. the minimum over each drive point's distance), not just picking a
// better fixed coordinate here.
static RRNodeId get_or_create_virtual_clock_network_root(int layer, int x, int y) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_graph_builder = device_ctx.rr_graph_builder;

    RRNodeId virtual_clock_network_root_idx = device_ctx.rr_graph.virtual_clock_network_root_idx(device_ctx.arch->default_clock_network_name.c_str());
    if (virtual_clock_network_root_idx == RRNodeId::INVALID()) {
        virtual_clock_network_root_idx = create_virtual_clock_network_sink_node(layer, x, y);
        rr_graph_builder.set_virtual_clock_network_root_idx(virtual_clock_network_root_idx);
    }
    return virtual_clock_network_root_idx;
}

void RoutingToClockConnection::create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) {
    // Initialize random seed
    // Must be done during every call in order for restored rr_graphs after a binary
    // search to be consistent
    std::mt19937 rand_generator;
    rand_generator.seed(seed);

    auto& device_ctx = g_vpr_ctx.mutable_device();
    const auto& node_lookup = device_ctx.rr_graph.node_lookup();

    RRNodeId virtual_clock_network_root_idx = get_or_create_virtual_clock_network_root(
        virtual_sink_location.layer_num, virtual_sink_location.x, virtual_sink_location.y);

    // rr_node indices for x and y channel routing wires and clock wires to connect to
    auto x_wire_indices = node_lookup.find_channel_nodes(switch_location.layer_num, switch_location.x, switch_location.y, e_rr_type::CHANX);
    auto y_wire_indices = node_lookup.find_channel_nodes(switch_location.layer_num, switch_location.x, switch_location.y, e_rr_type::CHANY);
    auto clock_indices = clock_graph.get_rr_node_indices_at_switch_location(
        clock_to_connect_to, switch_point_name, switch_location.x, switch_location.y);

    for (int clock_index : clock_indices) {
        // Select wires to connect to at random
        std::shuffle(x_wire_indices.begin(), x_wire_indices.end(), rand_generator);
        std::shuffle(y_wire_indices.begin(), y_wire_indices.end(), rand_generator);

        // Connect to x-channel wires
        unsigned num_wires_x = x_wire_indices.size() * fc;
        for (size_t i = 0; i < num_wires_x; i++) {
            clock_graph.add_edge(rr_edges_to_create, x_wire_indices[i], RRNodeId(clock_index), arch_switch_idx, false);
        }

        // Connect to y-channel wires
        unsigned num_wires_y = y_wire_indices.size() * fc;
        for (size_t i = 0; i < num_wires_y; i++) {
            clock_graph.add_edge(rr_edges_to_create, y_wire_indices[i], RRNodeId(clock_index), arch_switch_idx, false);
        }

        // Connect to virtual clock sink node
        // used by the two stage router
        clock_graph.add_edge(rr_edges_to_create, RRNodeId(clock_index), virtual_clock_network_root_idx, arch_switch_idx, false);
    }
}

/*
 * TileToClockConnection (setters)
 */

void TileToClockConnection::set_clock_name_to_connect_to(std::string clock_name) {
    clock_to_connect_to = std::move(clock_name);
}

void TileToClockConnection::set_clock_switch_point_name(std::string clock_switch_point_name) {
    switch_point_name = std::move(clock_switch_point_name);
}

void TileToClockConnection::set_location(int x, int y, int layer /* =0 */) {
    location.x = x;
    location.y = y;
    location.layer_num = layer;
}

void TileToClockConnection::set_virtual_sink_location(int x, int y, int layer /* =0 */) {
    virtual_sink_location.x = x;
    virtual_sink_location.y = y;
    virtual_sink_location.layer_num = layer;
}

void TileToClockConnection::set_tile_name(std::string name) {
    tile_name = std::move(name);
}

void TileToClockConnection::set_subtile_range(int low, int high) {
    subtile_range = {low, high};
}

void TileToClockConnection::set_port_name(std::string name) {
    port_name = std::move(name);
}

void TileToClockConnection::set_pin_range(int low, int high) {
    pin_range = {low, high};
}

void TileToClockConnection::set_switch(int arch_switch_index) {
    arch_switch_idx = arch_switch_index;
}

void TileToClockConnection::set_fc_val(float fc_val) {
    fc = fc_val;
}

/*
 * TileToClockConnection (member functions)
 */

size_t TileToClockConnection::estimate_additional_nodes() {
    // Up to 1 rr node is added as the virtual clock sink (shared across all drive
    // points of a clock network, so most calls add 0; this is just a capacity
    // estimate, so overcounting here is harmless).
    return 1;
}

void TileToClockConnection::create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) {
    std::mt19937 rand_generator;
    rand_generator.seed(seed);

    auto& device_ctx = g_vpr_ctx.mutable_device();
    const auto& node_lookup = device_ctx.rr_graph.node_lookup();
    auto& grid = device_ctx.grid;

    t_physical_tile_type_ptr type = grid.get_physical_type({location.x, location.y, location.layer_num});
    if (is_empty_type(type)) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Clock connection tap 'TILE.%s.%s' at location (%d, %d) has no tile placed there.\n",
                        tile_name.c_str(), port_name.c_str(), location.x, location.y);
    }
    if (type->name != tile_name) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Clock connection tap 'TILE.%s.%s' at location (%d, %d) does not match the tile placed there ('%s').\n",
                        tile_name.c_str(), port_name.c_str(), location.x, location.y, type->name.c_str());
    }

    // Find the sub tile that owns the requested port.
    const t_sub_tile* owning_sub_tile = nullptr;
    const t_physical_tile_port* port = nullptr;
    for (const t_sub_tile& sub_tile : type->sub_tiles) {
        for (const t_physical_tile_port& candidate : sub_tile.ports) {
            if (port_name == candidate.name) {
                owning_sub_tile = &sub_tile;
                port = &candidate;
                break;
            }
        }
        if (port != nullptr) {
            break;
        }
    }
    if (port == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Clock connection tap references port '%s' which does not exist on tile '%s'.\n",
                        port_name.c_str(), tile_name.c_str());
    }

    int subtile_low = (subtile_range.first >= 0) ? subtile_range.first : owning_sub_tile->capacity.low;
    int subtile_high = (subtile_range.second >= 0) ? subtile_range.second : owning_sub_tile->capacity.high;
    if (subtile_low < owning_sub_tile->capacity.low || subtile_high > owning_sub_tile->capacity.high || subtile_low > subtile_high) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Clock connection tap subtile range [%d:%d] is out of bounds for tile '%s' port '%s' (valid range [%d:%d]).\n",
                        subtile_low, subtile_high, tile_name.c_str(), port_name.c_str(),
                        owning_sub_tile->capacity.low, owning_sub_tile->capacity.high);
    }

    int pin_low = (pin_range.first >= 0) ? pin_range.first : 0;
    int pin_high = (pin_range.second >= 0) ? pin_range.second : port->num_pins - 1;
    if (pin_low < 0 || pin_high >= port->num_pins || pin_low > pin_high) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Clock connection tap pin range [%d:%d] is out of bounds for tile '%s' port '%s' (%d pins).\n",
                        pin_low, pin_high, tile_name.c_str(), port_name.c_str(), port->num_pins);
    }

    // Gather the OPIN rr nodes for every (sub tile instance, pin) pair in range.
    std::vector<RRNodeId> pin_nodes;
    std::vector<e_side> all_sides(TOTAL_2D_SIDES.begin(), TOTAL_2D_SIDES.end());
    for (int capacity = subtile_low; capacity <= subtile_high; capacity++) {
        for (int pin_in_port = pin_low; pin_in_port <= pin_high; pin_in_port++) {
            int relative_pin = port->absolute_first_pin_index + pin_in_port;
            int physical_pin = get_physical_pin_from_capacity_location(type, relative_pin, capacity);

            auto [x_offsets, y_offsets, sides] = get_pin_coordinates(type, physical_pin, all_sides);
            for (size_t i = 0; i < sides.size(); i++) {
                RRNodeId node = node_lookup.find_node(location.layer_num,
                                                      location.x + x_offsets[i],
                                                      location.y + y_offsets[i],
                                                      e_rr_type::OPIN,
                                                      physical_pin,
                                                      sides[i]);
                if (node != RRNodeId::INVALID()) {
                    pin_nodes.push_back(node);
                }
            }
        }
    }

    if (pin_nodes.empty()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Clock connection tap 'TILE.%s.%s' at location (%d, %d) did not resolve to any routable pins.\n",
                        tile_name.c_str(), port_name.c_str(), location.x, location.y);
    }

    auto clock_indices = clock_graph.get_rr_node_indices_at_switch_location(
        clock_to_connect_to, switch_point_name, location.x, location.y);

    // See get_or_create_virtual_clock_network_root's doc comment: registering this drive
    // point's switch point node against the shared virtual sink is what lets
    // --two_stage_clock_routing pre-route a global net to this network regardless of
    // whether the network is entered from general routing, a tile pin, or both.
    RRNodeId virtual_clock_network_root_idx = get_or_create_virtual_clock_network_root(
        virtual_sink_location.layer_num, virtual_sink_location.x, virtual_sink_location.y);

    for (int clock_index : clock_indices) {
        std::shuffle(pin_nodes.begin(), pin_nodes.end(), rand_generator);

        unsigned num_pins_to_connect = pin_nodes.size() * fc;
        for (unsigned i = 0; i < num_pins_to_connect; i++) {
            clock_graph.add_edge(rr_edges_to_create, pin_nodes[i], RRNodeId(clock_index), arch_switch_idx, false);
        }

        clock_graph.add_edge(rr_edges_to_create, RRNodeId(clock_index), virtual_clock_network_root_idx, arch_switch_idx, false);
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

    std::set<std::pair<int, int>> to_locations = clock_graph.get_switch_locations(to_clock, to_switch);

    for (auto [x, y] : to_locations) {

        std::vector<int> to_rr_node_indices = clock_graph.get_rr_node_indices_at_switch_location(
            to_clock,
            to_switch,
            x,
            y);

        // boundary conditions:
        // y at grid height and height -1 connections share the same drive point
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
        for (int to_index : to_rr_node_indices) {
            for (size_t i = 0; i < num_connections; i++) {
                if (from_itter == from_rr_node_indices.end()) {
                    from_itter = from_rr_node_indices.begin();
                }
                clock_graph.add_edge(rr_edges_to_create, RRNodeId(*from_itter), RRNodeId(to_index), arch_switch_idx, false);
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
    int layer_num = 0; //Function *FOR NOW* assumes that layer_num is always 0

    for (int x = 0; x < (int)grid.width(); x++) {
        for (int y = 0; y < (int)grid.height(); y++) {
            //Avoid boundary
            if ((y == 0 && x == 0) || (x == (int)grid.width() - 1 && y == (int)grid.height() - 1)) {
                continue;
            }

            t_physical_tile_type_ptr type = grid.get_physical_type({x, y, layer_num});

            // Skip EMPTY type
            if (is_empty_type(type)) {
                continue;
            }

            int width_offset = grid.get_width_offset({x, y, layer_num});
            int height_offset = grid.get_height_offset({x, y, layer_num});

            // Ignore grid locations that do not have blocks
            bool has_pb_type = false;
            std::unordered_set<t_logical_block_type_ptr> equivalent_sites = get_equivalent_sites_set(type);
            for (t_logical_block_type_ptr logical_block : equivalent_sites) {
                if (logical_block->pb_type) {
                    has_pb_type = true;
                    break;
                }
            }

            if (!has_pb_type) {
                continue;
            }

            for (e_side side : TOTAL_2D_SIDES) {
                //Don't connect pins which are not adjacent to channels around the perimeter
                if ((x == 0 && side != RIGHT) || (x == (int)grid.width() - 1 && side != LEFT) || (y == 0 && side != TOP) || (y == (int)grid.height() - 1 && side != BOTTOM)) {
                    continue;
                }

                for (int clock_pin_idx : type->get_clock_pins_indices()) {
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
                    } else if (x == (int)grid.width() - 1) {
                        clock_x_offset = -1; // chanx clock always ends at 1 offset
                        clock_y_offset = -1; // pick the chanx below the block
                    } else if (y == 0) {
                        clock_y_offset = 0; // pick chanx above the block, no offset needed
                    } else {
                        clock_y_offset = -1; // pick the chanx below the block
                    }

                    RRNodeId clock_pin_node_idx = node_lookup.find_node(layer_num,
                                                                        x,
                                                                        y,
                                                                        e_rr_type::IPIN,
                                                                        clock_pin_idx,
                                                                        side);

                    // required=false: this loop probes every tile in the device, and a
                    // quadrant/range-scoped clock network is only expected to reach the
                    // tiles within its own scope -- a miss here just means "not reached
                    // from this network," not an arch mistake.
                    std::vector<int> clock_network_indices = clock_graph.get_rr_node_indices_at_switch_location(
                        clock_to_connect_from,
                        switch_point_name,
                        x + clock_x_offset,
                        y + clock_y_offset,
                        /*required=*/false);

                    //Create edges depending on Fc
                    for (size_t i = 0; i < clock_network_indices.size() * fc; i++) {
                        clock_graph.add_edge(rr_edges_to_create, RRNodeId(clock_network_indices[i]), RRNodeId(clock_pin_node_idx), arch_switch_idx, false);
                    }
                }
            }
        }
    }
}
