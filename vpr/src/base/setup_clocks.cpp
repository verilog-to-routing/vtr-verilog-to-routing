#include "setup_clocks.h"

#include "globals.h"
#include "expr_eval.h"

#include "vtr_assert.h"
#include "vpr_error.h"

#include "vpr_utils.h"

#include <string>
#include <iostream>
#include <sstream>

static MetalLayer get_metal_layer_from_name(
    std::string metal_layer_name,
    std::unordered_map<std::string, t_metal_layer> clock_metal_layers,
    std::string clock_network_name);
static void setup_clock_network_wires(const t_arch& Arch, std::vector<t_segment_inf>& segment_inf);
static void setup_clock_connections(const t_arch& Arch);

void setup_clock_networks(const t_arch& Arch, std::vector<t_segment_inf>& segment_inf) {
    setup_clock_network_wires(Arch, segment_inf);
    setup_clock_connections(Arch);
}

/**
 * Parses the clock architecture information and modifies the architecture segment
 * information.
 */
void setup_clock_network_wires(const t_arch& Arch, std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& clock_networks_device = device_ctx.clock_networks;
    auto& grid = device_ctx.grid;

    auto& clock_networks_arch = Arch.clock_arch.clock_networks_arch;
    auto& clock_metal_layers = Arch.clock_arch.clock_metal_layers;

    // TODO: copied over from SetupGrid. Ensure consistency by only assiging in one place
    t_formula_data vars;
    vars.set_var_value("W", grid.width());
    vars.set_var_value("H", grid.height());

    for (auto clock_network_arch : clock_networks_arch) {
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
                    parse_formula(clock_network_arch.wire.start, vars),
                    parse_formula(clock_network_arch.wire.end, vars),
                    parse_formula(clock_network_arch.wire.position, vars));
                spine->set_wire_repeat(
                    parse_formula(clock_network_arch.repeat.x, vars),
                    parse_formula(clock_network_arch.repeat.y, vars));
                spine->set_drive_location(parse_formula(clock_network_arch.drive.offset, vars));
                spine->set_drive_switch(clock_network_arch.drive.arch_switch_idx);
                spine->set_drive_name(clock_network_arch.drive.name);
                spine->set_tap_locations(
                    parse_formula(clock_network_arch.tap.offset, vars),
                    parse_formula(clock_network_arch.tap.increment, vars));
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
                    parse_formula(clock_network_arch.wire.start, vars),
                    parse_formula(clock_network_arch.wire.end, vars),
                    parse_formula(clock_network_arch.wire.position, vars));
                rib->set_wire_repeat(
                    parse_formula(clock_network_arch.repeat.x, vars),
                    parse_formula(clock_network_arch.repeat.y, vars));
                rib->set_drive_location(parse_formula(clock_network_arch.drive.offset, vars));
                rib->set_drive_switch(clock_network_arch.drive.arch_switch_idx);
                rib->set_drive_name(clock_network_arch.drive.name);
                rib->set_tap_locations(
                    parse_formula(clock_network_arch.tap.offset, vars),
                    parse_formula(clock_network_arch.tap.increment, vars));
                rib->set_tap_name(clock_network_arch.tap.name);

                rib->create_segments(segment_inf);
                clock_networks_device.push_back(std::move(rib));
                break;
            }
            case e_clock_type::H_TREE: {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "HTrees not yet supported.\n");
                break;
            }
            default: {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                          "Found unsupported clock network type for '%s' clock network",
                          clock_network_arch.name.c_str());
            }
        }
    }
    clock_networks_device.shrink_to_fit();
}

void setup_clock_connections(const t_arch& Arch) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& clock_connections_device = device_ctx.clock_connections;
    auto& grid = device_ctx.grid;

    auto& clock_connections_arch = Arch.clock_arch.clock_connections_arch;

    // TODO: copied over from SetupGrid. Ensure consistency by only assiging in one place
    t_formula_data vars;
    vars.set_var_value("W", grid.width());
    vars.set_var_value("H", grid.height());

    for (auto clock_connection_arch : clock_connections_arch) {
        if (clock_connection_arch.from == "ROUTING") {
            clock_connections_device.emplace_back(new RoutingToClockConnection);
            RoutingToClockConnection* routing_to_clock = dynamic_cast<RoutingToClockConnection*>(clock_connections_device.back().get());

            //TODO: Add error check to check that clock name and tap name exist and that only
            //      two names are returned by the below function
            auto names = vtr::split(clock_connection_arch.to, ".");
            VTR_ASSERT_MSG(names.size() == 2, "Invalid clock name.\n");
            routing_to_clock->set_clock_name_to_connect_to(names[0]);
            routing_to_clock->set_clock_switch_point_name(names[1]);

            routing_to_clock->set_switch_location(
                parse_formula(clock_connection_arch.locationx, vars),
                parse_formula(clock_connection_arch.locationy, vars));
            routing_to_clock->set_switch(clock_connection_arch.arch_switch_idx);
            routing_to_clock->set_fc_val(clock_connection_arch.fc);

        } else if (clock_connection_arch.to == "CLOCK") {
            clock_connections_device.emplace_back(new ClockToPinsConnection);
            ClockToPinsConnection* clock_to_pins = dynamic_cast<ClockToPinsConnection*>(clock_connections_device.back().get());

            //TODO: Add error check to check that clock name and tap name exist and that only
            //      two names are returned by the below function
            auto names = vtr::split(clock_connection_arch.from, ".");
            VTR_ASSERT_MSG(names.size() == 2, "Invalid clock name.\n");
            clock_to_pins->set_clock_name_to_connect_from(names[0]);
            clock_to_pins->set_clock_switch_point_name(names[1]);

            clock_to_pins->set_switch(clock_connection_arch.arch_switch_idx);
            clock_to_pins->set_fc_val(clock_connection_arch.fc);
        } else {
            clock_connections_device.emplace_back(new ClockToClockConneciton);
            ClockToClockConneciton* clock_to_clock = dynamic_cast<ClockToClockConneciton*>(clock_connections_device.back().get());

            //TODO: Add error check to check that clock name and tap name exist and that only
            //      two names are returned by the below function
            auto to_names = vtr::split(clock_connection_arch.to, ".");
            auto from_names = vtr::split(clock_connection_arch.from, ".");
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

MetalLayer get_metal_layer_from_name(
    std::string metal_layer_name,
    std::unordered_map<std::string, t_metal_layer> clock_metal_layers,
    std::string clock_network_name) {
    auto itter = clock_metal_layers.find(metal_layer_name);

    if (itter == clock_metal_layers.end()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                  "Metal layer '%s' for clock network '%s' not found. Check to make sure that it is"
                  "included in the clock architecture description",
                  metal_layer_name.c_str(),
                  clock_network_name.c_str());
    }

    // Metal layer was found. Copy over from arch description to proper data type
    auto arch_metal_layer = itter->second;
    MetalLayer metal_layer;
    metal_layer.r_metal = arch_metal_layer.r_metal;
    metal_layer.c_metal = arch_metal_layer.c_metal;

    return metal_layer;
}
