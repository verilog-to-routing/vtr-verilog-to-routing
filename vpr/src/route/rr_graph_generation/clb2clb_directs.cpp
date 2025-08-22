#include "clb2clb_directs.h"

#include "globals.h"
#include "vpr_utils.h"
#include "physical_types_util.h"

std::vector<t_clb_to_clb_directs> alloc_and_load_clb_to_clb_directs(const std::vector<t_direct_inf>& directs, int delayless_switch) {
    // TODO: The function that does this parsing in placement is poorly done because it lacks generality on heterogeniety, should replace with this one

    auto& device_ctx = g_vpr_ctx.device();

    const int num_directs = directs.size();
    std::vector<t_clb_to_clb_directs> clb_to_clb_directs(num_directs);

    for (int i = 0; i < num_directs; i++) {
        //clb_to_clb_directs[i].from_clb_type;
        clb_to_clb_directs[i].from_clb_pin_start_index = 0;
        clb_to_clb_directs[i].from_clb_pin_end_index = 0;
        //clb_to_clb_directs[i].to_clb_type;
        clb_to_clb_directs[i].to_clb_pin_start_index = 0;
        clb_to_clb_directs[i].to_clb_pin_end_index = 0;
        clb_to_clb_directs[i].switch_index = 0;

        // Load from pins
        // Parse out the pb_type name, port name, and pin range
        auto [start_pin_index, end_pin_index, tile_name, port_name] = parse_direct_pin_name(directs[i].from_pin, directs[i].line);

        // Figure out which type, port, and pin is used
        t_physical_tile_type_ptr physical_tile = find_tile_type_by_name(tile_name, device_ctx.physical_tile_types);
        if (physical_tile == nullptr) {
            VPR_THROW(VPR_ERROR_ARCH, "Unable to find block %s.\n", tile_name.c_str());
        }
        clb_to_clb_directs[i].from_clb_type = physical_tile;

        t_physical_tile_port tile_port = find_tile_port_by_name(physical_tile, port_name);

        if (start_pin_index == UNDEFINED) {
            VTR_ASSERT(start_pin_index == end_pin_index);
            start_pin_index = 0;
            end_pin_index = tile_port.num_pins - 1;
        }

        // Add clb directs start/end pin indices based on the absolute pin position
        // of the port defined in the direct connection. The CLB is the source one.
        clb_to_clb_directs[i].from_clb_pin_start_index = tile_port.absolute_first_pin_index + start_pin_index;
        clb_to_clb_directs[i].from_clb_pin_end_index = tile_port.absolute_first_pin_index + end_pin_index;

        // Load to pins
        // Parse out the pb_type name, port name, and pin range
        std::tie(start_pin_index, end_pin_index, tile_name, port_name) = parse_direct_pin_name(directs[i].to_pin, directs[i].line);

        // Figure out which type, port, and pin is used
        physical_tile = find_tile_type_by_name(tile_name, device_ctx.physical_tile_types);
        if (physical_tile == nullptr) {
            VPR_THROW(VPR_ERROR_ARCH, "Unable to find block %s.\n", tile_name.c_str());
        }
        clb_to_clb_directs[i].to_clb_type = physical_tile;

        tile_port = find_tile_port_by_name(physical_tile, port_name);

        if (start_pin_index == UNDEFINED) {
            VTR_ASSERT(start_pin_index == end_pin_index);
            start_pin_index = 0;
            end_pin_index = tile_port.num_pins - 1;
        }

        // Add clb directs start/end pin indices based on the absolute pin position
        // of the port defined in the direct connection. The CLB is the destination one.
        clb_to_clb_directs[i].to_clb_pin_start_index = tile_port.absolute_first_pin_index + start_pin_index;
        clb_to_clb_directs[i].to_clb_pin_end_index = tile_port.absolute_first_pin_index + end_pin_index;

        if (abs(clb_to_clb_directs[i].from_clb_pin_start_index - clb_to_clb_directs[i].from_clb_pin_end_index) != abs(clb_to_clb_directs[i].to_clb_pin_start_index - clb_to_clb_directs[i].to_clb_pin_end_index)) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line,
                      "Range mismatch from %s to %s.\n", directs[i].from_pin.c_str(), directs[i].to_pin.c_str());
        }

        //Set the switch index
        if (directs[i].switch_type > 0) {
            //Use the specified switch
            clb_to_clb_directs[i].switch_index = directs[i].switch_type;
        } else {
            //Use the delayless switch by default
            clb_to_clb_directs[i].switch_index = delayless_switch;
        }
    }

    return clb_to_clb_directs;
}
