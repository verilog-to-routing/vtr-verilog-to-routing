#include "setup_clocks.h"

#include "globals.h"
#include "expr_eval.h"

#include "vtr_assert.h"
#include "vpr_error.h"

static MetalLayer get_metal_layer_from_name(
        std::string metal_layer_name,
        std::unordered_map<std::string, t_metal_layer> clock_metal_layers,
        std::string clock_network_name);

void setup_clock_networks(const t_arch& Arch) {

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& clock_networks_device = device_ctx.clock_networks;
    auto& grid = device_ctx.grid;

    auto& clock_networks_arch = Arch.clock_networks_arch;
    auto& clock_metal_layers = Arch.clock_metal_layers;

    // TODO: copied over from SetupGrid. Ensure consistency by only assiging in one place
    t_formula_data vars;
    vars.set_var_value("W", grid.width());
    vars.set_var_value("H", grid.height());

    for(auto clock_network_arch : clock_networks_arch) {
        switch(clock_network_arch.type) {
            case e_clock_type::SPINE: {
                clock_networks_device.emplace_back(new ClockSpine());
                ClockSpine* spine = dynamic_cast<ClockSpine*>(clock_networks_device.back().get());

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

                break;
            } case e_clock_type::RIB: {
                clock_networks_device.emplace_back(new ClockRib());
                ClockRib* rib = dynamic_cast<ClockRib*>(clock_networks_device.back().get());

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

                break;
            } case e_clock_type::H_TREE: {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "HTrees not yet supported.\n");
                break;
             } default: {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Found unsupported clock network type for '%s' clock network",
                    clock_network_arch.name.c_str());
             }
        }
    }
    clock_networks_device.shrink_to_fit();
}

MetalLayer get_metal_layer_from_name(
        std::string metal_layer_name,
        std::unordered_map<std::string, t_metal_layer> clock_metal_layers,
        std::string clock_network_name)
{
    auto itter = clock_metal_layers.find(metal_layer_name);

    if(itter == clock_metal_layers.end()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "Metal layer '%s' for clock network '%s' not found. Check to make sure that it is"
            "included in the clock architecture description",
            metal_layer_name,
            clock_network_name);
    }

    // Metal layer was found. Copy over from arch description to proper data type 
    auto arch_metal_layer = itter->second;
    MetalLayer metal_layer;
    metal_layer.r_metal = arch_metal_layer.r_metal;
    metal_layer.c_metal = arch_metal_layer.c_metal;

    return metal_layer;
}


