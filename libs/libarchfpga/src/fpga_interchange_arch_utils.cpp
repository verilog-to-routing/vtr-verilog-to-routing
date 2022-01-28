#include "fpga_interchange_arch_utils.h"

/****************** Utility functions ******************/

/**
 * @brief The FPGA interchange timing model includes three different corners (min, typ and max) for each of the two
 * speed_models (slow and fast).
 *
 * Timing data can be found on PIPs, nodes, site pins and bel pins.
 * This function retrieves the timing value based on the wanted speed model and the wanted corner.
 *
 * Corner model is considered valid if at least one configuration is set.
 * In that case this value shall be returned.
 *
 * More information on the FPGA Interchange timing model can be found here:
 *   - https://github.com/chipsalliance/fpga-interchange-schema/blob/main/interchange/DeviceResources.capnp
 */

float get_corner_value(DeviceResources::Device::CornerModel::Reader model, const char* speed_model, const char* value) {
    bool slow_model = std::string(speed_model) == std::string("slow");
    bool fast_model = std::string(speed_model) == std::string("fast");

    bool min_corner = std::string(value) == std::string("min");
    bool typ_corner = std::string(value) == std::string("typ");
    bool max_corner = std::string(value) == std::string("max");

    if (!slow_model && !fast_model) {
        archfpga_throw("", __LINE__, "Wrong speed model `%s`. Expected `slow` or `fast`\n", speed_model);
    }

    if (!min_corner && !typ_corner && !max_corner) {
        archfpga_throw("", __LINE__, "Wrong corner model `%s`. Expected `min`, `typ` or `max`\n", value);
    }

    bool has_fast = model.getFast().hasFast();
    bool has_slow = model.getSlow().hasSlow();

    if ((slow_model || (fast_model && !has_fast)) && has_slow) {
        auto half = model.getSlow().getSlow();
        if (min_corner && half.getMin().isMin()) {
            return half.getMin().getMin();
        } else if (typ_corner && half.getTyp().isTyp()) {
            return half.getTyp().getTyp();
        } else if (max_corner && half.getMax().isMax()) {
            return half.getMax().getMax();
        } else {
            if (half.getMin().isMin()) {
                return half.getMin().getMin();
            } else if (half.getTyp().isTyp()) {
                return half.getTyp().getTyp();
            } else if (half.getMax().isMax()) {
                return half.getMax().getMax();
            } else {
                archfpga_throw("", __LINE__, "Invalid speed model %s. No value found!\n", speed_model);
            }
        }
    } else if ((fast_model || slow_model) && has_fast) {
        auto half = model.getFast().getFast();
        if (min_corner && half.getMin().isMin()) {
            return half.getMin().getMin();
        } else if (typ_corner && half.getTyp().isTyp()) {
            return half.getTyp().getTyp();
        } else if (max_corner && half.getMax().isMax()) {
            return half.getMax().getMax();
        } else {
            if (half.getMin().isMin()) {
                return half.getMin().getMin();
            } else if (half.getTyp().isTyp()) {
                return half.getTyp().getTyp();
            } else if (half.getMax().isMax()) {
                return half.getMax().getMax();
            } else {
                archfpga_throw("", __LINE__, "Invalid speed model %s. No value found!\n", speed_model);
            }
        }
    }
    return 0.;
}

char* int_to_string(char* buff, int value) {
    if (value < 10) {
        return &(*buff = '0' + value) + 1;
    } else {
        return &(*int_to_string(buff, value / 10) = '0' + value % 10) + 1;
    }
}

void pip_types(std::set<std::tuple<int, bool>>& seen,
               DeviceResources::Device::Reader ar_) {
    for (const auto& tile : ar_.getTileTypeList()) {
        for (const auto& pip : tile.getPips()) {
            /*
             * FIXME
             * right now we allow for pseudo pips even tho we don't check if they are avaiable
             * if (pip.isPseudoCells())
             *     continue;
             */
            seen.emplace(pip.getTiming(), pip.getBuffered21());
            if (!pip.getDirectional()) {
                seen.emplace(pip.getTiming(), pip.getBuffered20());
            }
        }
    }
}

void fill_switch(t_arch_switch_inf* switch_,
                 float R,
                 float Cin,
                 float Cout,
                 float Cinternal,
                 float Tdel,
                 float mux_trans_size,
                 float buf_size,
                 char* name,
                 SwitchType type) {
    switch_->R = R;
    switch_->Cin = Cin;
    switch_->Cout = Cout;
    switch_->Cinternal = Cinternal;
    switch_->set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, Tdel);
    switch_->mux_trans_size = mux_trans_size;
    switch_->buf_size = buf_size;
    switch_->name = name;
    switch_->set_type(type);
}

void process_cell_bel_mappings(DeviceResources::Device::Reader ar_,
                               std::unordered_map<uint32_t, std::set<t_bel_cell_mapping>>& bel_cell_mappings_,
                               std::function<std::string(size_t)> str) {
    auto primLib = ar_.getPrimLibs();
    auto portList = primLib.getPortList();

    for (auto cell_mapping : ar_.getCellBelMap()) {
        size_t cell_name = cell_mapping.getCell();

        int found_valid_prim = false;
        for (auto primitive : primLib.getCellDecls()) {
            bool is_prim = str(primitive.getLib()) == std::string("primitives");
            bool is_cell = cell_name == primitive.getName();

            bool has_inout = false;
            for (auto port_idx : primitive.getPorts()) {
                auto port = portList[port_idx];

                if (port.getDir() == INOUT) {
                    has_inout = true;
                    break;
                }
            }

            if (is_prim && is_cell && !has_inout) {
                found_valid_prim = true;
                break;
            }
        }

        if (!found_valid_prim)
            continue;

        for (auto common_pins : cell_mapping.getCommonPins()) {
            std::vector<std::pair<size_t, size_t>> pins;

            for (auto pin_map : common_pins.getPins())
                pins.emplace_back(pin_map.getCellPin(), pin_map.getBelPin());

            for (auto site_type_entry : common_pins.getSiteTypes()) {
                size_t site_type = site_type_entry.getSiteType();

                for (auto bel : site_type_entry.getBels()) {
                    t_bel_cell_mapping mapping;

                    mapping.cell = cell_name;
                    mapping.site = site_type;
                    mapping.pins = pins;

                    std::set<t_bel_cell_mapping> maps{mapping};
                    auto res = bel_cell_mappings_.emplace(bel, maps);
                    if (!res.second) {
                        res.first->second.insert(mapping);
                    }
                }
            }
        }
    }
}
