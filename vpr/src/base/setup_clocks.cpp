#include "setup_clocks.h"

#include "globals.h"
#include "vtr_expr_eval.h"

#include "vtr_assert.h"
#include "vpr_error.h"

#include "vpr_utils.h"
#include "vtr_token.h"
#include "vtr_util.h"

#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <utility>

using vtr::FormulaParser;
using vtr::t_formula_data;

// Describes a "TILE.<tile_name>[hi:lo].<port_name>[hi:lo]" clock tap spec, as parsed
// from the architecture file's "from" attribute of a <clock_routing><tap> entry.
// The [hi:lo] ranges are optional; {-1, -1} means "not specified" (use the full range),
// resolved once the tile type at the tap's location is known (see
// TileToClockConnection::create_switches).
struct t_tile_tap_spec {
    std::string tile_name;
    std::pair<int, int> subtile_range = {-1, -1};
    std::string port_name;
    std::pair<int, int> pin_range = {-1, -1};
};

static MetalLayer get_metal_layer_from_name(
    std::string metal_layer_name,
    std::unordered_map<std::string, t_metal_layer> clock_metal_layers,
    std::string clock_network_name);
static void setup_clock_network_wires(const t_arch& Arch, FormulaParser& p, std::vector<t_segment_inf>& segment_inf);
static void setup_clock_connections(const t_arch& Arch, FormulaParser& p);
static bool is_tile_tap_spec(const std::string& from);
static t_tile_tap_spec parse_tile_tap_spec(const std::string& spec);

void setup_clock_networks(const t_arch& Arch, std::vector<t_segment_inf>& segment_inf) {
    // This function may be called more than once if the device grid is
    // resized after the clock networks were first set up (clock network
    // geometry is computed from the grid width/height). Reset segment_inf to
    // the architecture's base segment list (discarding any clock segments
    // appended by a previous call) and clear any previously-created clock
    // network/connection objects so this function is safe to re-run.
    segment_inf = Arch.Segments;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.clock_networks.clear();
    device_ctx.clock_connections.clear();

    FormulaParser p;
    setup_clock_network_wires(Arch, p, segment_inf);
    setup_clock_connections(Arch, p);
}

/**
 * @brief Parses the clock architecture information and modifies
 *        the architecture segment information.
 */
void setup_clock_network_wires(const t_arch& Arch, FormulaParser& p, std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& clock_networks_device = device_ctx.clock_networks;
    auto& grid = device_ctx.grid;

    const std::vector<t_clock_network_arch>& clock_networks_arch = Arch.clock_arch.clock_networks_arch;
    const std::unordered_map<std::string, t_metal_layer>& clock_metal_layers = Arch.clock_arch.clock_metal_layers;

    // TODO: copied over from SetupGrid. Ensure consistency by only assigning in one place
    t_formula_data vars;
    vars.set_var_value("W", grid.width());
    vars.set_var_value("H", grid.height());

    for (const t_clock_network_arch& clock_network_arch : clock_networks_arch) {
        switch (clock_network_arch.type) {
            case e_clock_type::SPINE: {
                std::unique_ptr<ClockSpine> spine = std::make_unique<ClockSpine>();

                spine->set_clock_name(clock_network_arch.name);
                spine->set_num_instance(clock_network_arch.num_inst);
                spine->set_metal_layer(get_metal_layer_from_name(
                    clock_network_arch.metal_layer,
                    clock_metal_layers,
                    clock_network_arch.name));
                spine->set_initial_wire_location(
                    p.parse_formula(clock_network_arch.wire.start, vars),
                    p.parse_formula(clock_network_arch.wire.end, vars),
                    p.parse_formula(clock_network_arch.wire.position, vars));
                spine->set_wire_repeat(
                    p.parse_formula(clock_network_arch.repeat.x, vars),
                    p.parse_formula(clock_network_arch.repeat.y, vars),
                    p.parse_formula(clock_network_arch.repeat.end_x, vars));
                spine->set_drive_location(p.parse_formula(clock_network_arch.drive.offset, vars));
                spine->set_drive_switch(clock_network_arch.drive.arch_switch_idx);
                spine->set_drive_name(clock_network_arch.drive.name);
                spine->set_tap_locations(
                    p.parse_formula(clock_network_arch.tap.offset, vars),
                    p.parse_formula(clock_network_arch.tap.increment, vars));
                spine->set_tap_name(clock_network_arch.tap.name);

                spine->create_segments(segment_inf);

                clock_networks_device.push_back(std::move(spine));
                break;
            }
            case e_clock_type::RIB: {
                std::unique_ptr<ClockRib> rib = std::make_unique<ClockRib>();

                rib->set_clock_name(clock_network_arch.name);
                rib->set_num_instance(clock_network_arch.num_inst);
                rib->set_metal_layer(get_metal_layer_from_name(
                    clock_network_arch.metal_layer,
                    clock_metal_layers,
                    clock_network_arch.name));
                rib->set_initial_wire_location(
                    p.parse_formula(clock_network_arch.wire.start, vars),
                    p.parse_formula(clock_network_arch.wire.end, vars),
                    p.parse_formula(clock_network_arch.wire.position, vars));
                rib->set_wire_repeat(
                    p.parse_formula(clock_network_arch.repeat.x, vars),
                    p.parse_formula(clock_network_arch.repeat.y, vars),
                    p.parse_formula(clock_network_arch.repeat.end_y, vars));
                rib->set_drive_location(p.parse_formula(clock_network_arch.drive.offset, vars));
                rib->set_drive_switch(clock_network_arch.drive.arch_switch_idx);
                rib->set_drive_name(clock_network_arch.drive.name);
                rib->set_tap_locations(
                    p.parse_formula(clock_network_arch.tap.offset, vars),
                    p.parse_formula(clock_network_arch.tap.increment, vars));
                rib->set_tap_name(clock_network_arch.tap.name);

                rib->create_segments(segment_inf);
                clock_networks_device.push_back(std::move(rib));
                break;
            }
            case e_clock_type::H_TREE: {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER, "HTrees not yet supported.\n");
                break;
            }
            case e_clock_type::SWITCH_GRID: {
                std::unique_ptr<ClockSwitchGrid> switch_grid = std::make_unique<ClockSwitchGrid>();

                switch_grid->set_clock_name(clock_network_arch.name);
                switch_grid->set_num_instance(clock_network_arch.num_inst);
                switch_grid->set_metal_layer(get_metal_layer_from_name(
                    clock_network_arch.switch_grid.metal_layer,
                    clock_metal_layers,
                    clock_network_arch.name));
                int grid_startx = p.parse_formula(clock_network_arch.switch_grid.startx, vars);
                int grid_starty = p.parse_formula(clock_network_arch.switch_grid.starty, vars);
                switch_grid->set_grid_start_location(grid_startx, grid_starty);
                switch_grid->set_wire_repeat(
                    p.parse_formula(clock_network_arch.switch_grid.repeatx, vars),
                    p.parse_formula(clock_network_arch.switch_grid.repeaty, vars));
                int switch_grid_chan_w = p.parse_formula(clock_network_arch.switch_grid.chan_w, vars);
                if (clock_network_arch.switch_grid.directionality == UNI_DIRECTIONAL && switch_grid_chan_w % 2 != 0) {
                    VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                    "Clock switch grid channel width must be even for unidirectional wires "
                                    "(got %d) for clock network '%s'.\n",
                                    switch_grid_chan_w, clock_network_arch.name.c_str());
                }
                switch_grid->set_chan_width(switch_grid_chan_w);
                switch_grid->set_internal_switch(clock_network_arch.switch_grid.arch_switch_idx);
                switch_grid->set_switch_block_type(clock_network_arch.switch_grid.switch_block_type);
                switch_grid->set_length(p.parse_formula(clock_network_arch.switch_grid.length, vars));
                switch_grid->set_directionality(clock_network_arch.switch_grid.directionality);

                for (const t_clock_switch_grid_point& point : clock_network_arch.switch_grid.switch_points) {
                    SwitchGridPointType type = (point.type == e_clock_switch_grid_point_type::DRIVE)
                                                    ? SwitchGridPointType::DRIVE
                                                    : SwitchGridPointType::TAP;

                    // xoffset/yoffset are relative to the switch grid's own startx/starty,
                    // matching the rib/spine convention where drive/tap offsets are relative
                    // to that network's start point (see ClockRib::set_drive_location /
                    // ClockSpine::set_drive_location).
                    int base_x = grid_startx + p.parse_formula(point.xoffset, vars);
                    int base_y = grid_starty + p.parse_formula(point.yoffset, vars);
                    int xincr = p.parse_formula(point.xincr, vars);
                    int yincr = p.parse_formula(point.yincr, vars);

                    // xincr/yincr repeat a tap point across the grid every that-many
                    // switch boxes (mirroring rib/spine's tap xincr/yincr); "0" (the
                    // default, and always the case for drive points) means a single
                    // point at (base_x, base_y).
                    //
                    // The outermost ring of tiles (x/y == 0 or width/height - 1) is the
                    // device perimeter and never has a switch box -- general routing
                    // channels don't reach it either (see the "-2 for no perim channels"
                    // convention used throughout rr_graph2.cpp/rr_graph_chan_seg_details.cpp,
                    // and ClockSwitchGrid's own x_max/y_max in clock_network_builders.cpp).
                    // Clamp the auto-generated end here to match, so a plain xincr="1"/
                    // yincr="1" naturally covers every valid switch box location instead
                    // of overshooting onto the perimeter and warning about unregistered
                    // switch points.
                    int x_end = (xincr > 0) ? (int)grid.width() - 1 : base_x + 1;
                    int y_end = (yincr > 0) ? (int)grid.height() - 1 : base_y + 1;
                    int x_step = (xincr > 0) ? xincr : 1;
                    int y_step = (yincr > 0) ? yincr : 1;

                    for (int x = base_x; x < x_end; x += x_step) {
                        for (int y = base_y; y < y_end; y += y_step) {
                            switch_grid->add_switch_point(point.name, type, x, y, point.arch_switch_idx);
                        }
                    }
                }

                switch_grid->create_segments(segment_inf);

                clock_networks_device.push_back(std::move(switch_grid));
                break;
            }
            default: {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Found unsupported clock network type for '%s' clock network",
                                clock_network_arch.name.c_str());
            }
        }
    }
    clock_networks_device.shrink_to_fit();
}

bool is_tile_tap_spec(const std::string& from) {
    return from.rfind("TILE.", 0) == 0;
}

// Parses an optional "[hi:lo]" (or "[idx]") range at tokens[token_index], advancing
// token_index past it. Returns {-1, -1} (leaving token_index unchanged) if there is
// no bracket at this position, since the range is optional in the tap grammar.
static std::pair<int, int> parse_optional_bracket_range(const Tokens& tokens, size_t& token_index, const std::string& spec) {
    if (tokens[token_index].type != e_token_type::OPEN_SQUARE_BRACKET) {
        return {-1, -1};
    }
    token_index++;

    if (tokens[token_index].type != e_token_type::INT) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected integer after '['.\n", spec.c_str());
    }
    int first = vtr::atoi(tokens[token_index].data);
    token_index++;

    int second = first;
    if (tokens[token_index].type == e_token_type::COLON) {
        token_index++;
        if (tokens[token_index].type != e_token_type::INT) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected integer after ':'.\n", spec.c_str());
        }
        second = vtr::atoi(tokens[token_index].data);
        token_index++;
    }

    if (tokens[token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': missing closing ']'.\n", spec.c_str());
    }
    token_index++;

    if (first > second) {
        std::swap(first, second);
    }
    return {first, second};
}

// Parses a "TILE.<tile_name>[hi:lo].<port_name>[hi:lo]" clock tap spec (both [hi:lo]
// ranges optional). Only validates syntax; resolving the tile/port names and ranges
// against the actual architecture happens later, in TileToClockConnection::create_switches,
// once the tile type placed at the tap's location is known.
t_tile_tap_spec parse_tile_tap_spec(const std::string& spec) {
    Tokens tokens(spec);
    size_t token_index = 0;

    if (tokens[token_index].type != e_token_type::STRING || tokens[token_index].data != "TILE") {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected to start with 'TILE.'.\n", spec.c_str());
    }
    token_index++;

    if (tokens[token_index].type != e_token_type::DOT) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected '.' after 'TILE'.\n", spec.c_str());
    }
    token_index++;

    t_tile_tap_spec result;

    if (tokens[token_index].type != e_token_type::STRING) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected a tile name.\n", spec.c_str());
    }
    result.tile_name = tokens[token_index].data;
    token_index++;

    result.subtile_range = parse_optional_bracket_range(tokens, token_index, spec);

    if (tokens[token_index].type != e_token_type::DOT) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected '.' after tile name.\n", spec.c_str());
    }
    token_index++;

    if (tokens[token_index].type != e_token_type::STRING) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': expected a port name.\n", spec.c_str());
    }
    result.port_name = tokens[token_index].data;
    token_index++;

    result.pin_range = parse_optional_bracket_range(tokens, token_index, spec);

    if (token_index != tokens.size()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Invalid clock tap tile spec '%s': unexpected trailing characters.\n", spec.c_str());
    }

    return result;
}

void setup_clock_connections(const t_arch& Arch, FormulaParser& p) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& clock_connections_device = device_ctx.clock_connections;
    auto& grid = device_ctx.grid;

    const std::vector<t_clock_connection_arch>& clock_connections_arch = Arch.clock_arch.clock_connections_arch;

    // TODO: copied over from SetupGrid. Ensure consistency by only assigning in one place
    t_formula_data vars;
    vars.set_var_value("W", grid.width());
    vars.set_var_value("H", grid.height());

    // A clock network's drive points -- whether from general routing (ROUTING) or a tile
    // port/pin (TILE.*) -- all share one virtual sink node in the RR graph (see
    // get_or_create_virtual_clock_network_root in clock_connection_builders.cpp), so pick
    // a single representative location for that shared node up front: the centroid of all
    // of this network's drive point locations, rather than arbitrarily using whichever
    // drive point happens to be processed first.
    std::unordered_map<std::string, std::pair<int, int>> drive_location_sum;
    std::unordered_map<std::string, int> drive_location_count;
    for (const t_clock_connection_arch& clock_connection_arch : clock_connections_arch) {
        if (clock_connection_arch.from == "ROUTING" || is_tile_tap_spec(clock_connection_arch.from)) {
            std::string clock_name = vtr::StringToken(clock_connection_arch.to).split(".")[0];
            drive_location_sum[clock_name].first += p.parse_formula(clock_connection_arch.locationx, vars);
            drive_location_sum[clock_name].second += p.parse_formula(clock_connection_arch.locationy, vars);
            drive_location_count[clock_name]++;
        }
    }

    for (const t_clock_connection_arch& clock_connection_arch : clock_connections_arch) {
        if (clock_connection_arch.from == "ROUTING") {
            clock_connections_device.emplace_back(new RoutingToClockConnection);
            if (RoutingToClockConnection* routing_to_clock = dynamic_cast<RoutingToClockConnection*>(clock_connections_device.back().get())) {
                //TODO: Add error check to check that clock name and tap name exist and that only
                //      two names are returned by the below function
                std::vector<std::string> names = vtr::StringToken(clock_connection_arch.to).split(".");
                VTR_ASSERT_MSG(names.size() == 2, "Invalid clock name.\n");
                routing_to_clock->set_clock_name_to_connect_to(names[0]);
                routing_to_clock->set_clock_switch_point_name(names[1]);

                routing_to_clock->set_switch_location(
                    p.parse_formula(clock_connection_arch.locationx, vars),
                    p.parse_formula(clock_connection_arch.locationy, vars));
                routing_to_clock->set_switch(clock_connection_arch.arch_switch_idx);
                routing_to_clock->set_fc_val(clock_connection_arch.fc);

                int count = drive_location_count[names[0]];
                routing_to_clock->set_virtual_sink_location(
                    drive_location_sum[names[0]].first / count,
                    drive_location_sum[names[0]].second / count);
            }

        } else if (is_tile_tap_spec(clock_connection_arch.from)) {
            clock_connections_device.emplace_back(new TileToClockConnection);
            if (TileToClockConnection* tile_to_clock = dynamic_cast<TileToClockConnection*>(clock_connections_device.back().get())) {
                t_tile_tap_spec tile_spec = parse_tile_tap_spec(clock_connection_arch.from);

                std::vector<std::string> names = vtr::StringToken(clock_connection_arch.to).split(".");
                VTR_ASSERT_MSG(names.size() == 2, "Invalid clock name.\n");
                tile_to_clock->set_clock_name_to_connect_to(names[0]);
                tile_to_clock->set_clock_switch_point_name(names[1]);

                tile_to_clock->set_location(
                    p.parse_formula(clock_connection_arch.locationx, vars),
                    p.parse_formula(clock_connection_arch.locationy, vars));
                tile_to_clock->set_tile_name(tile_spec.tile_name);
                tile_to_clock->set_subtile_range(tile_spec.subtile_range.first, tile_spec.subtile_range.second);
                tile_to_clock->set_port_name(tile_spec.port_name);
                tile_to_clock->set_pin_range(tile_spec.pin_range.first, tile_spec.pin_range.second);
                tile_to_clock->set_switch(clock_connection_arch.arch_switch_idx);
                tile_to_clock->set_fc_val(clock_connection_arch.fc);

                int count = drive_location_count[names[0]];
                tile_to_clock->set_virtual_sink_location(
                    drive_location_sum[names[0]].first / count,
                    drive_location_sum[names[0]].second / count);
            }

        } else if (clock_connection_arch.to == "CLOCK") {
            clock_connections_device.emplace_back(new ClockToPinsConnection);
            if (ClockToPinsConnection* clock_to_pins = dynamic_cast<ClockToPinsConnection*>(clock_connections_device.back().get())) {
                //TODO: Add error check to check that clock name and tap name exist and that only
                //      two names are returned by the below function
                std::vector<std::string> names = vtr::StringToken(clock_connection_arch.from).split(".");
                VTR_ASSERT_MSG(names.size() == 2, "Invalid clock name.\n");
                clock_to_pins->set_clock_name_to_connect_from(names[0]);
                clock_to_pins->set_clock_switch_point_name(names[1]);

                clock_to_pins->set_switch(clock_connection_arch.arch_switch_idx);
                clock_to_pins->set_fc_val(clock_connection_arch.fc);
            }
        } else {
            clock_connections_device.emplace_back(new ClockToClockConneciton);
            if (ClockToClockConneciton* clock_to_clock = dynamic_cast<ClockToClockConneciton*>(clock_connections_device.back().get())) {
                //TODO: Add error check to check that clock name and tap name exist and that only
                //      two names are returned by the below function
                std::vector<std::string> to_names = vtr::StringToken(clock_connection_arch.to).split(".");
                std::vector<std::string> from_names = vtr::StringToken(clock_connection_arch.from).split(".");
                VTR_ASSERT_MSG(to_names.size() == 2, "Invalid clock name.\n");
                clock_to_clock->set_to_clock_name(to_names[0]);
                clock_to_clock->set_to_clock_switch_point_name(to_names[1]);
                clock_to_clock->set_from_clock_name(from_names[0]);
                clock_to_clock->set_from_clock_switch_point_name(from_names[1]);

                clock_to_clock->set_switch(clock_connection_arch.arch_switch_idx);
                clock_to_clock->set_fc_val(clock_connection_arch.fc);
            }
        }
    }
}

MetalLayer get_metal_layer_from_name(
    std::string metal_layer_name,
    std::unordered_map<std::string, t_metal_layer> clock_metal_layers,
    std::string clock_network_name) {
    auto itter = clock_metal_layers.find(metal_layer_name);

    if (itter == clock_metal_layers.end()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Metal layer '%s' for clock network '%s' not found. Check to make sure that it is"
                        "included in the clock architecture description",
                        metal_layer_name.c_str(),
                        clock_network_name.c_str());
    }

    // Metal layer was found. Copy over from arch description to proper data type
    t_metal_layer arch_metal_layer = itter->second;
    MetalLayer metal_layer;
    metal_layer.r_metal = arch_metal_layer.r_metal;
    metal_layer.c_metal = arch_metal_layer.c_metal;

    return metal_layer;
}
