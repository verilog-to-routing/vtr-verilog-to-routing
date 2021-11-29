#include <algorithm>
#include <kj/std/iostream.h>
#include <limits>
#include <map>
#include <set>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <zlib.h>

#include "vtr_assert.h"
#include "vtr_digest.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "arch_check.h"
#include "arch_error.h"
#include "arch_util.h"
#include "arch_types.h"

#include "read_fpga_interchange_arch.h"

/*
 * FPGA Interchange Device frontend
 *
 * This file contains functions to read and parse a Cap'n'proto FPGA interchange device description
 * and populate the various VTR architecture's internal data structures.
 *
 * The Device data is, by default, GZipped, hence the requirement of the ZLIB library to allow
 * for in-memory decompression of the input file.
 */

using namespace DeviceResources;
using namespace LogicalNetlist;
using namespace capnp;

/****************** Utility functions ******************/

/**
 * @brief The FPGA interchange timing model includes three different corners (min, typ and max) for each of the two
 * speed_models (slow and fast).
 *
 * Timing data can be found on PIPs, nodes, site pins and bel pins.
 * This function retrieves the timing value based on the wanted speed model and the wanted corner.
 *
 * More information on the FPGA Interchange timing model can be found here:
 *   - https://github.com/chipsalliance/fpga-interchange-schema/blob/main/interchange/DeviceResources.capnp
 */
static float get_corner_value(Device::CornerModel::Reader model, const char* speed_model, const char* value) {
    bool slow_model = std::string(speed_model) == std::string("slow");
    bool fast_model = std::string(speed_model) == std::string("fast");

    bool min_corner = std::string(value) == std::string("min");
    bool typ_corner = std::string(value) == std::string("typ");
    bool max_corner = std::string(value) == std::string("max");

    if (!slow_model && !fast_model) {
        archfpga_throw("", __LINE__,
                       "Wrong speed model `%s`. Expected `slow` or `fast`\n", speed_model);
    }

    if (!min_corner && !typ_corner && !max_corner) {
        archfpga_throw("", __LINE__,
                       "Wrong corner model `%s`. Expected `min`, `typ` or `max`\n", value);
    }

    bool has_fast = model.getFast().hasFast();
    bool has_slow = model.getSlow().hasSlow();

    if (slow_model && has_slow) {
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
                archfpga_throw("", __LINE__,
                               "Invalid speed model %s. No value found!\n", speed_model);
            }
        }
    } else if (fast_model && has_fast) {
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
                archfpga_throw("", __LINE__,
                               "Invalid speed model %s. No value found!\n", speed_model);
            }
        }
    }

    return 0.;
}

static t_model_ports* get_model_port(t_arch* arch, std::string model, std::string port) {
    for (t_model* m : {arch->models, arch->model_library}) {
        for (; m != nullptr; m = m->next) {
            if (std::string(m->name) != model)
                continue;

            for (t_model_ports* p : {m->inputs, m->outputs})
                for (; p != nullptr; p = p->next)
                    if (std::string(p->name) == port)
                        return p;
        }
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Could not find model port: %s (%s)\n", port.c_str(), model.c_str());
}

static t_model* get_model(t_arch* arch, std::string model) {
    for (t_model* m : {arch->models, arch->model_library})
        for (; m != nullptr; m = m->next)
            if (std::string(m->name) == model)
                return m;

    archfpga_throw(__FILE__, __LINE__,
                   "Could not find model: %s\n", model.c_str());
}

template<typename T>
static T* get_type_by_name(const char* type_name, std::vector<T>& types) {
    for (auto& type : types) {
        if (0 == strcmp(type.name, type_name)) {
            return &type;
        }
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Could not find type: %s\n", type_name);
}

static t_port get_generic_port(t_arch* arch,
                               t_pb_type* pb_type,
                               PORTS dir,
                               std::string name,
                               std::string model = "",
                               int num_pins = 1) {
    t_port port;
    port.parent_pb_type = pb_type;
    port.name = vtr::strdup(name.c_str());
    port.num_pins = num_pins;
    port.index = 0;
    port.absolute_first_pin_index = 0;
    port.port_index_by_type = 0;
    port.equivalent = PortEquivalence::NONE;
    port.type = dir;
    port.is_clock = false;
    port.model_port = nullptr;
    port.port_class = vtr::strdup(nullptr);
    port.port_power = (t_port_power*)vtr::calloc(1, sizeof(t_port_power));

    if (!model.empty())
        port.model_port = get_model_port(arch, model, name);

    return port;
}

/****************** End Utility functions ******************/

struct ArchReader {
  public:
    ArchReader(t_arch* arch,
               Device::Reader& arch_reader,
               const char* arch_file,
               std::vector<t_physical_tile_type>& phys_types,
               std::vector<t_logical_block_type>& logical_types)
        : arch_(arch)
        , arch_file_(arch_file)
        , ar_(arch_reader)
        , ptypes_(phys_types)
        , ltypes_(logical_types) {
        set_arch_file_name(arch_file);

        for (auto cell_bel : ar_.getCellBelMap()) {
            auto name = str(cell_bel.getCell());
            for (auto site_bels : cell_bel.getCommonPins()[0].getSiteTypes()) {
                auto site_type = str(site_bels.getSiteType());
                for (auto bel : site_bels.getBels()) {
                    auto bel_name = str(bel);
                    std::pair<std::string, std::string> key(site_type, bel_name);
                    bel_cell_mapping_[key].push_back(name);
                }
            }
        }
    }

    void read_arch() {
        // Preprocess arch information
        process_luts();
        process_package_pins();

        process_models();
        process_device();

        process_blocks();
        process_tiles();
        link_physical_logical_types(ptypes_, ltypes_);

        SyncModelsPbTypes(arch_, ltypes_);
        check_models(arch_);

        process_layout();
        process_switches();
        process_segments();
    }

  private:
    t_arch* arch_;
    const char* arch_file_;
    Device::Reader& ar_;
    std::vector<t_physical_tile_type>& ptypes_;
    std::vector<t_logical_block_type>& ltypes_;

    t_default_fc_spec default_fc_;

    //                siteTypeName, belName     , list of cell names
    std::map<std::pair<std::string, std::string>, std::vector<std::string>> bel_cell_mapping_;

    // Utils
    std::string str(int idx) {
        return std::string(ar_.getStrList()[idx].cStr());
    }

    int get_bel_type_count(Device::SiteType::Reader& site, Device::BELCategory category) {
        int count = 0;
        for (auto bel : site.getBels())
            if (bel.getCategory() == category)
                count++;

        return count;
    }

    Device::BEL::Reader get_bel_reader(Device::SiteType::Reader& site, std::string bel_name) {
        for (auto bel : site.getBels()) {
            if (str(bel.getName()) == bel_name)
                return bel;
        }
        VTR_ASSERT(0);
    }

    std::string get_ic_prefix(Device::SiteType::Reader& site, Device::BEL::Reader& bel) {
        return bel.getCategory() == Device::BELCategory::SITE_PORT ? str(site.getName()) : str(bel.getName());
    }

    std::unordered_map<std::string, std::tuple<std::string, std::string, e_interconnect, bool>> get_ics(Device::SiteType::Reader& site) {
        // dictionary:
        //   - key: interconnect name
        //   - value: (inputs string, outputs string, interconnect type)
        std::unordered_map<std::string, std::tuple<std::string, std::string, e_interconnect, bool>> ics;
        for (auto wire : site.getSiteWires()) {
            std::string wire_name = str(wire.getName());

            // pin name, bel name
            std::tuple<unsigned int, std::string, std::string> out_pin;
            bool is_mux = false;
            for (auto pin : wire.getPins()) {
                auto bel_pin = site.getBelPins()[pin];
                auto bel = get_bel_reader(site, str(bel_pin.getBel()));
                auto bel_name = get_ic_prefix(site, bel);

                bool is_output = bel_pin.getDir() == LogicalNetlist::Netlist::Direction::OUTPUT;
                if (is_output) {
                    VTR_ASSERT(std::get<1>(out_pin).empty());
                    out_pin = std::make_tuple(pin, str(bel_pin.getName()), bel_name);
                    is_mux = bel.getCategory() == Device::BELCategory::ROUTING;
                }
            }
            VTR_ASSERT(!std::get<1>(out_pin).empty());

            // Stores all output BELs connected to the same out_pin
            std::string pad_bel_name;
            std::string pad_bel_pin_name;
            bool is_pad = false;
            for (auto pin : wire.getPins()) {
                if (pin == std::get<0>(out_pin))
                    continue;

                auto bel_pin = site.getBelPins()[pin];
                std::string out_bel_pin_name = str(bel_pin.getName());

                auto bel = get_bel_reader(site, str(bel_pin.getBel()));
                auto out_bel_name = get_ic_prefix(site, bel);

                for (auto pad_bel : arch_->pad_bels) {
                    is_pad = pad_bel.bel_name == out_bel_name || is_pad;
                    pad_bel_name = pad_bel.bel_name == out_bel_name ? out_bel_name : pad_bel_name;
                    pad_bel_pin_name = pad_bel.bel_name == out_bel_name ? out_bel_pin_name : pad_bel_pin_name;
                }
            }

            for (auto pin : wire.getPins()) {
                if (pin == std::get<0>(out_pin))
                    continue;

                auto bel_pin = site.getBelPins()[pin];
                std::string out_bel_pin_name = str(bel_pin.getName());

                auto bel = get_bel_reader(site, str(bel_pin.getBel()));
                auto out_bel_name = get_ic_prefix(site, bel);

                auto in_bel_name = std::get<2>(out_pin);
                auto in_bel_pin_name = std::get<1>(out_pin);

                std::string ostr = out_bel_name + "." + out_bel_pin_name;
                std::string istr = in_bel_name + "." + in_bel_pin_name;

                std::string inputs;
                std::string outputs;
                e_interconnect ic_type;
                if (is_mux) {
                    auto ic_name = in_bel_name;
                    auto res = ics.emplace(ic_name, std::make_tuple(std::string(), ostr, MUX_INTERC, false));

                    if (!res.second) {
                        std::tie(inputs, outputs, ic_type, std::ignore) = res.first->second;
                        outputs += " " + ostr;
                        res.first->second = std::make_tuple(inputs, outputs, ic_type, false);
                    }
                } else if (bel.getCategory() == Device::BELCategory::ROUTING) {
                    auto ic_name = str(bel.getName());
                    auto res = ics.emplace(ic_name, std::make_tuple(istr, std::string(), MUX_INTERC, false));

                    if (!res.second) {
                        std::tie(inputs, outputs, ic_type, std::ignore) = res.first->second;
                        inputs += " " + istr;
                        res.first->second = std::make_tuple(inputs, outputs, ic_type, false);
                    }
                } else {
                    auto ic_name = wire_name + "_" + out_bel_pin_name;
                    if (is_pad && bel.getCategory() == Device::BELCategory::LOGIC) {
                        if (out_bel_name == pad_bel_name)
                            ostr += "_in";
                        else { // Create new wire to connect PAD output to the BELs input
                            ic_name = wire_name + "_" + pad_bel_pin_name + "_out";
                            istr = pad_bel_name + "." + pad_bel_pin_name + "_out";
                        }
                    }

                    auto res = ics.emplace(ic_name, std::make_tuple(istr, ostr, DIRECT_INTERC, is_pad));

                    if (!res.second) {
                        std::tie(inputs, outputs, ic_type, std::ignore) = res.first->second;
                        if (inputs.empty())
                            inputs = istr;
                        else
                            inputs += " " + istr;

                        if (outputs.empty())
                            outputs = ostr;
                        else
                            outputs += " " + ostr;

                        res.first->second = std::make_tuple(inputs, outputs, ic_type, is_pad);
                    }
                }
            }
        }

        return ics;
    }

    // Model processing
    void process_models() {
        // Populate the common library, namely .inputs, .outputs, .names, .latches
        CreateModelLibrary(arch_);

        t_model* temp = nullptr;
        std::map<std::string, int> model_name_map;
        std::pair<std::map<std::string, int>::iterator, bool> ret_map_name;

        int model_index = NUM_MODELS_IN_LIBRARY;
        arch_->models = nullptr;

        auto primLib = ar_.getPrimLibs();
        for (auto primitive : primLib.getCellDecls()) {
            if (str(primitive.getLib()) == std::string("primitives")) {
                std::string prim_name = str(primitive.getName());

                bool is_lut = false;
                for (auto lut_cell : arch_->lut_cells)
                    is_lut = lut_cell.name == prim_name || is_lut;

                if (is_lut)
                    continue;

                try {
                    temp = new t_model;
                    temp->index = model_index++;

                    temp->never_prune = true;
                    temp->name = vtr::strdup(str(primitive.getName()).c_str());

                    ret_map_name = model_name_map.insert(std::pair<std::string, int>(temp->name, 0));
                    if (!ret_map_name.second) {
                        archfpga_throw(arch_file_, __LINE__,
                                       "Duplicate model name: '%s'.\n", temp->name);
                    }

                    process_model_ports(temp, primitive);

                    check_model_clocks(temp, arch_file_, __LINE__);
                    check_model_combinational_sinks(temp, arch_file_, __LINE__);
                    warn_model_missing_timing(temp, arch_file_, __LINE__);

                } catch (ArchFpgaError& e) {
                    free_arch_model(temp);
                    throw;
                }
                temp->next = arch_->models;
                arch_->models = temp;
            }
        }
    }

    void process_model_ports(t_model* model, Netlist::CellDeclaration::Reader primitive) {
        auto primLib = ar_.getPrimLibs();
        auto portList = primLib.getPortList();

        std::set<std::pair<std::string, enum PORTS>> port_names;

        for (auto port_idx : primitive.getPorts()) {
            auto port = portList[port_idx];
            enum PORTS dir = ERR_PORT;
            switch (port.getDir()) {
                case LogicalNetlist::Netlist::Direction::INPUT:
                    dir = IN_PORT;
                    break;
                case LogicalNetlist::Netlist::Direction::OUTPUT:
                    dir = OUT_PORT;
                    break;
                case LogicalNetlist::Netlist::Direction::INOUT:
                    dir = INOUT_PORT;
                    break;
                default:
                    break;
            }
            t_model_ports* model_port = new t_model_ports;
            model_port->dir = dir;
            model_port->name = vtr::strdup(str(port.getName()).c_str());

            // TODO: add parsing of clock port types when the interchange schema allows for it:
            //       https://github.com/chipsalliance/fpga-interchange-schema/issues/66

            //Sanity checks
            if (model_port->is_clock == true && model_port->is_non_clock_global == true) {
                archfpga_throw(arch_file_, __LINE__,
                               "Model port '%s' cannot be both a clock and a non-clock signal simultaneously", model_port->name);
            }
            if (model_port->name == nullptr) {
                archfpga_throw(arch_file_, __LINE__,
                               "Model port is missing a name");
            }
            if (port_names.count(std::pair<std::string, enum PORTS>(model_port->name, dir)) && dir != INOUT_PORT) {
                archfpga_throw(arch_file_, __LINE__,
                               "Duplicate model port named '%s'", model_port->name);
            }
            if (dir == OUT_PORT && !model_port->combinational_sink_ports.empty()) {
                archfpga_throw(arch_file_, __LINE__,
                               "Model output ports can not have combinational sink ports");
            }

            model_port->min_size = 1;
            model_port->size = 1;
            if (port.isBus()) {
                int s = port.getBus().getBusStart();
                int e = port.getBus().getBusEnd();
                model_port->size = std::abs(e - s) + 1;
            }

            port_names.insert(std::pair<std::string, enum PORTS>(model_port->name, dir));
            //Add the port
            if (dir == IN_PORT) {
                model_port->next = model->inputs;
                model->inputs = model_port;

            } else if (dir == OUT_PORT) {
                model_port->next = model->outputs;
                model->outputs = model_port;
            }
        }
    }

    // Complex Blocks
    void process_blocks() {
        auto siteTypeList = ar_.getSiteTypeList();

        int index = 0;
        auto EMPTY = get_empty_logical_type(std::string("NULL"));
        EMPTY.index = index;
        ltypes_.push_back(EMPTY);

        for (auto site : siteTypeList) {
            t_logical_block_type ltype;

            std::string name = str(site.getName());

            // Check for duplicates
            auto is_duplicate = [name](t_logical_block_type l) { return std::string(l.name) == name; };
            VTR_ASSERT(std::find_if(ltypes_.begin(), ltypes_.end(), is_duplicate) == ltypes_.end());

            ltype.name = vtr::strdup(name.c_str());
            ltype.index = ++index;

            auto pb_type = new t_pb_type;
            ltype.pb_type = pb_type;

            pb_type->name = vtr::strdup(name.c_str());
            pb_type->num_pb = 1;
            process_block_ports(pb_type, name, site);

            // Process modes (for simplicity, only the default mode is allowed for the time being)
            pb_type->num_modes = 1;
            pb_type->modes = new t_mode[pb_type->num_modes];

            auto bels = site.getBels();
            auto mode = &pb_type->modes[0];
            mode->parent_pb_type = pb_type;
            mode->index = 0;
            mode->name = vtr::strdup("default");
            mode->disable_packing = false;

            int bel_count = get_bel_type_count(site, Device::BELCategory::LOGIC);
            mode->num_pb_type_children = bel_count;
            mode->pb_type_children = new t_pb_type[bel_count];

            int count = 0;
            for (auto bel : bels) {
                if (bel.getCategory() != Device::BELCategory::LOGIC)
                    continue;

                auto bel_name = str(bel.getName());
                std::pair<std::string, std::string> key(name, bel_name);

                auto cell_name = bel_name;

                if (bel_cell_mapping_.find(key) != bel_cell_mapping_.end()) {
                    VTR_ASSERT(bel_cell_mapping_[key].size() == 1);
                    cell_name = bel_cell_mapping_[key][0];
                }

                auto leaf_pb_type = new t_pb_type;
                leaf_pb_type->name = vtr::strdup(bel_name.c_str());
                leaf_pb_type->num_pb = 1;
                leaf_pb_type->parent_mode = mode;

                // TODO: fix this to make it dynamic. This will need the usage of CellBelMapping

                auto find_lut = [cell_name](t_lut_cell l) { return l.name == cell_name; };
                bool is_lut = std::find_if(arch_->lut_cells.begin(), arch_->lut_cells.end(), find_lut) != arch_->lut_cells.end();

                auto find_pad = [bel_name](t_package_pin p) { return p.bel_name == bel_name; };
                bool is_pad = std::find_if(arch_->pad_bels.begin(), arch_->pad_bels.end(), find_pad) != arch_->pad_bels.end();

                if (!is_pad)
                    process_block_ports(leaf_pb_type, cell_name, site, false, is_lut);

                if (is_lut) {
                    leaf_pb_type->blif_model = nullptr;
                    process_lut_block(leaf_pb_type);
                } else if (is_pad) {
                    leaf_pb_type->blif_model = nullptr;
                    process_pad_block(leaf_pb_type, bel, site);
                } else {
                    leaf_pb_type->blif_model = vtr::strdup((std::string(".subckt ") + cell_name).c_str());
                    leaf_pb_type->model = get_model(arch_, cell_name);
                }

                mode->pb_type_children[count++] = *leaf_pb_type;
            }

            process_interconnects(mode, site);
            ltypes_.push_back(ltype);
        }
    }

    void process_lut_block(t_pb_type* lut) {
        lut->num_modes = 1;
        lut->modes = new t_mode[1];

        // Check for duplicates
        std::string lut_name = lut->name;
        auto find_lut = [lut_name](t_lut_bel l) { return l.name == lut_name; };
        auto res = std::find_if(arch_->lut_bels.begin(), arch_->lut_bels.end(), find_lut);
        VTR_ASSERT(res != arch_->lut_bels.end());
        auto lut_bel = *res;

        auto mode = &lut->modes[0];
        mode->name = vtr::strdup("lut");
        mode->parent_pb_type = lut;
        mode->index = 0;
        mode->num_pb_type_children = 1;
        mode->pb_type_children = new t_pb_type[1];

        auto new_leaf = new t_pb_type;
        new_leaf->name = vtr::strdup("lut_child");
        new_leaf->num_pb = 1;
        new_leaf->parent_mode = mode;

        int num_ports = 2;
        new_leaf->num_ports = num_ports;
        new_leaf->ports = (t_port*)vtr::calloc(num_ports, sizeof(t_port));
        new_leaf->blif_model = vtr::strdup(MODEL_NAMES);
        new_leaf->model = get_model(arch_, std::string(MODEL_NAMES));

        auto in_size = lut_bel.input_pins.size();
        new_leaf->ports[0] = get_generic_port(arch_, new_leaf, IN_PORT, "in", MODEL_NAMES, in_size);
        new_leaf->ports[1] = get_generic_port(arch_, new_leaf, OUT_PORT, "out", MODEL_NAMES);

        mode->pb_type_children[0] = *new_leaf;

        // Num inputs + 1 (output pin)
        int num_pins = in_size + 1;

        mode->num_interconnect = num_pins;
        mode->interconnect = new t_interconnect[num_pins];

        for (int i = 0; i < num_pins; i++) {
            auto ic = new t_interconnect;

            std::stringstream istr;
            std::stringstream ostr;
            std::string input_string;
            std::string output_string;

            if (i < num_pins - 1) {
                istr << lut_bel.input_pins[i];
                ostr << "in[" << i << "]";
                input_string = std::string(lut->name) + std::string(".") + istr.str();
                output_string = std::string(new_leaf->name) + std::string(".") + ostr.str();
            } else {
                istr << "out";
                ostr << lut_bel.output_pin;
                input_string = std::string(new_leaf->name) + std::string(".") + istr.str();
                output_string = std::string(lut->name) + std::string(".") + ostr.str();
            }
            std::string name = istr.str() + std::string("_") + ostr.str();
            ic->name = vtr::strdup(name.c_str());
            ic->type = DIRECT_INTERC;
            ic->parent_mode_index = 0;
            ic->parent_mode = mode;
            ic->input_string = vtr::strdup(input_string.c_str());
            ic->output_string = vtr::strdup(output_string.c_str());

            mode->interconnect[i] = *ic;
        }
    }

    void process_pad_block(t_pb_type* pad, Device::BEL::Reader& bel, Device::SiteType::Reader& site) {
        // For now, hard-code two modes for pads, so that PADs can either be IPADs or OPADs
        pad->num_modes = 2;
        pad->modes = new t_mode[2];

        // Add PAD pb_type ports
        VTR_ASSERT(bel.getPins().size() == 1);
        std::string pin = str(site.getBelPins()[bel.getPins()[0]].getName());
        std::string ipin = pin + "_in";
        std::string opin = pin + "_out";

        auto num_ports = 2;
        auto ports = new t_port[num_ports];
        pad->ports = ports;
        pad->num_ports = pad->num_pins = num_ports;
        pad->num_input_pins = 1;
        pad->num_output_pins = 1;

        int pin_abs = 0;
        int pin_count = 0;
        for (auto dir : {IN_PORT, OUT_PORT}) {
            int pins_dir_count = 0;
            t_port* port = &ports[pin_count];

            port->parent_pb_type = pad;
            port->index = pin_count++;
            port->port_index_by_type = pins_dir_count++;
            port->absolute_first_pin_index = pin_abs++;

            port->equivalent = PortEquivalence::NONE;
            port->num_pins = 1;
            port->type = dir;
            port->is_clock = false;

            bool is_input = dir == IN_PORT;
            port->name = is_input ? vtr::strdup(ipin.c_str()) : vtr::strdup(opin.c_str());
            port->model_port = nullptr;
            port->port_class = vtr::strdup(nullptr);
            port->port_power = (t_port_power*)vtr::calloc(1, sizeof(t_port_power));
        }

        // OPAD mode
        auto omode = &pad->modes[0];
        omode->name = vtr::strdup("opad");
        omode->parent_pb_type = pad;
        omode->index = 0;
        omode->num_pb_type_children = 1;
        omode->pb_type_children = new t_pb_type[1];

        auto opad = new t_pb_type;
        opad->name = vtr::strdup("opad");
        opad->num_pb = 1;
        opad->parent_mode = omode;

        num_ports = 1;
        opad->num_ports = num_ports;
        opad->ports = (t_port*)vtr::calloc(num_ports, sizeof(t_port));
        opad->blif_model = vtr::strdup(MODEL_OUTPUT);
        opad->model = get_model(arch_, std::string(MODEL_OUTPUT));

        opad->ports[0] = get_generic_port(arch_, opad, IN_PORT, "outpad", MODEL_OUTPUT);
        omode->pb_type_children[0] = *opad;

        // IPAD mode
        auto imode = &pad->modes[1];
        imode->name = vtr::strdup("ipad");
        imode->parent_pb_type = pad;
        imode->index = 1;
        imode->num_pb_type_children = 1;
        imode->pb_type_children = new t_pb_type[1];

        auto ipad = new t_pb_type;
        ipad->name = vtr::strdup("ipad");
        ipad->num_pb = 1;
        ipad->parent_mode = imode;

        num_ports = 1;
        ipad->num_ports = num_ports;
        ipad->ports = (t_port*)vtr::calloc(num_ports, sizeof(t_port));
        ipad->blif_model = vtr::strdup(MODEL_INPUT);
        ipad->model = get_model(arch_, std::string(MODEL_INPUT));

        ipad->ports[0] = get_generic_port(arch_, ipad, OUT_PORT, "inpad", MODEL_INPUT);
        imode->pb_type_children[0] = *ipad;

        // Handle interconnects
        int num_pins = 1;

        omode->num_interconnect = num_pins;
        omode->interconnect = new t_interconnect[num_pins];

        imode->num_interconnect = num_pins;
        imode->interconnect = new t_interconnect[num_pins];

        std::string opad_istr = std::string(pad->name) + std::string(".") + ipin;
        std::string opad_ostr = std::string(opad->name) + std::string(".outpad");
        std::string o_ic_name = std::string(pad->name) + std::string("_") + std::string(opad->name);

        std::string ipad_istr = std::string(ipad->name) + std::string(".inpad");
        std::string ipad_ostr = std::string(pad->name) + std::string(".") + opin;
        std::string i_ic_name = std::string(ipad->name) + std::string("_") + std::string(pad->name);

        auto o_ic = new t_interconnect[num_pins];
        auto i_ic = new t_interconnect[num_pins];

        o_ic->name = vtr::strdup(o_ic_name.c_str());
        o_ic->type = DIRECT_INTERC;
        o_ic->parent_mode_index = 0;
        o_ic->parent_mode = omode;
        o_ic->input_string = vtr::strdup(opad_istr.c_str());
        o_ic->output_string = vtr::strdup(opad_ostr.c_str());

        i_ic->name = vtr::strdup(i_ic_name.c_str());
        i_ic->type = DIRECT_INTERC;
        i_ic->parent_mode_index = 0;
        i_ic->parent_mode = imode;
        i_ic->input_string = vtr::strdup(ipad_istr.c_str());
        i_ic->output_string = vtr::strdup(ipad_ostr.c_str());

        omode->interconnect[0] = *o_ic;
        imode->interconnect[0] = *i_ic;
    }

    void process_block_ports(t_pb_type* pb_type, std::string cell_name, Device::SiteType::Reader& site, bool is_root = true, bool is_model_library = false) {
        std::unordered_set<std::string> names;

        // Prepare data based on pb_type level
        std::unordered_map<std::string, PORTS> pins;
        if (is_root) {
            for (auto pin : site.getPins()) {
                auto dir = pin.getDir() == LogicalNetlist::Netlist::Direction::INPUT ? IN_PORT : OUT_PORT;
                pins.emplace(str(pin.getName()), dir);
            }
        } else {
            for (auto bel : site.getBels()) {
                if (bel.getCategory() != Device::BELCategory::LOGIC)
                    continue;

                if (std::string(pb_type->name) != str(bel.getName()))
                    continue;

                for (auto bel_pin : bel.getPins()) {
                    auto pin = site.getBelPins()[bel_pin];
                    auto dir = pin.getDir() == LogicalNetlist::Netlist::Direction::INPUT ? IN_PORT : OUT_PORT;
                    pins.emplace(str(pin.getName()), dir);
                }
            }
        }

        auto num_ports = pins.size();
        auto ports = new t_port[num_ports];
        pb_type->ports = ports;
        pb_type->num_ports = pb_type->num_pins = num_ports;
        pb_type->num_input_pins = 0;
        pb_type->num_output_pins = 0;

        int pin_abs = 0;
        int pin_count = 0;
        for (auto dir : {IN_PORT, OUT_PORT}) {
            int pins_dir_count = 0;
            for (auto pin_pair : pins) {
                auto pin_name = pin_pair.first;
                auto pin_dir = pin_pair.second;

                if (pin_dir != dir)
                    continue;

                VTR_ASSERT(names.insert(pin_name).second);

                bool is_input = dir == IN_PORT;
                pb_type->num_input_pins += is_input ? 1 : 0;
                pb_type->num_output_pins += is_input ? 0 : 1;

                auto port = get_generic_port(arch_, pb_type, dir, pin_name);
                ports[pin_count] = port;
                port.index = pin_count++;
                port.port_index_by_type = pins_dir_count++;
                port.absolute_first_pin_index = pin_abs++;

                if (!is_root && !is_model_library)
                    port.model_port = get_model_port(arch_, cell_name, pin_name);
            }
        }
    }

    void process_interconnects(t_mode* mode, Device::SiteType::Reader& site) {
        auto ics = get_ics(site);
        auto num_ic = ics.size();

        mode->num_interconnect = num_ic;
        mode->interconnect = new t_interconnect[num_ic];

        int curr_ic = 0;
        std::unordered_set<std::string> names;

        // Handle site wires, namely direct interconnects
        for (auto ic_pair : ics) {
            std::string ic_name = ic_pair.first;

            std::string inputs;
            std::string outputs;
            e_interconnect ic_type;
            bool add_pack_pattern;

            std::tie(inputs, outputs, ic_type, add_pack_pattern) = ic_pair.second;

            t_interconnect* ic = &mode->interconnect[curr_ic++];

            if (add_pack_pattern) {
                ic->num_annotations = 1;
                // pack pattern
                auto pp = new t_pin_to_pin_annotation;

                pp->prop = (int*)vtr::calloc(1, sizeof(int));
                pp->value = (char**)vtr::calloc(1, sizeof(char*));

                pp->type = E_ANNOT_PIN_TO_PIN_PACK_PATTERN;
                pp->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
                pp->prop[0] = (int)E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME;
                pp->value[0] = vtr::strdup(ic_name.c_str());
                pp->input_pins = vtr::strdup(inputs.c_str());
                pp->output_pins = vtr::strdup(outputs.c_str());
                pp->num_value_prop_pairs = 1;
                pp->clock = nullptr;
                ic->annotations = pp;
            }

            // No line num for interconnects, as line num is XML specific
            // TODO: probably line_num should be deprecated as it is dependent
            //       on the input architecture format.
            ic->line_num = 0;
            ic->type = ic_type;
            ic->parent_mode_index = mode->index;
            ic->parent_mode = mode;

            VTR_ASSERT(names.insert(ic_name).second);
            ic->name = vtr::strdup(ic_name.c_str());
            ic->input_string = vtr::strdup(inputs.c_str());
            ic->output_string = vtr::strdup(outputs.c_str());
        }
    }

    // Physical Tiles
    void process_tiles() {
        auto EMPTY = get_empty_physical_type(std::string("NULL"));
        int index = 0;
        EMPTY.index = index;
        ptypes_.push_back(EMPTY);

        auto tileTypeList = ar_.getTileTypeList();

        for (auto tile : tileTypeList) {
            t_physical_tile_type ptype;
            auto name = str(tile.getName());

            if (name == std::string("NULL"))
                continue;

            ptype.name = vtr::strdup(name.c_str());
            ptype.index = ++index;
            ptype.width = ptype.height = ptype.area = 1;
            ptype.capacity = 1;

            process_sub_tiles(ptype, tile);

            setup_pin_classes(&ptype);

            bool is_pad = false;
            for (auto site : tile.getSiteTypes()) {
                auto site_type = ar_.getSiteTypeList()[site.getPrimaryType()];

                for (auto bel : site_type.getBels()) {
                    auto bel_name = str(bel.getName());
                    auto is_pad_func = [bel_name](t_package_pin p) { return p.bel_name == bel_name; };
                    auto res = std::find_if(arch_->pad_bels.begin(), arch_->pad_bels.end(), is_pad_func);

                    is_pad = res != arch_->pad_bels.end() || is_pad;
                }
            }

            ptype.is_input_type = ptype.is_output_type = is_pad;

            ptypes_.push_back(ptype);
        }
    }

    void process_sub_tiles(t_physical_tile_type& type, Device::TileType::Reader& tile) {
        // TODO: only one subtile at the moment
        auto siteTypeList = ar_.getSiteTypeList();
        for (auto site_in_tile : tile.getSiteTypes()) {
            t_sub_tile sub_tile;

            auto site = siteTypeList[site_in_tile.getPrimaryType()];

            sub_tile.index = 0;
            sub_tile.name = vtr::strdup(str(site.getName()).c_str());
            sub_tile.capacity.set(0, 0);

            int port_idx = 0;
            int abs_first_pin_idx = 0;
            int icount = 0;
            int ocount = 0;
            for (auto dir : {LogicalNetlist::Netlist::Direction::INPUT, LogicalNetlist::Netlist::Direction::OUTPUT}) {
                int port_idx_by_type = 0;
                for (auto pin : site.getPins()) {
                    if (pin.getDir() != dir)
                        continue;

                    t_physical_tile_port port;

                    port.name = vtr::strdup(str(pin.getName()).c_str());
                    port.equivalent = PortEquivalence::NONE;
                    port.num_pins = 1;

                    sub_tile.sub_tile_to_tile_pin_indices.push_back(port_idx);
                    port.index = port_idx++;

                    port.absolute_first_pin_index = abs_first_pin_idx++;
                    port.port_index_by_type = port_idx_by_type++;

                    if (dir == LogicalNetlist::Netlist::Direction::INPUT) {
                        port.type = IN_PORT;
                        icount++;
                    } else {
                        port.type = OUT_PORT;
                        ocount++;
                    }

                    sub_tile.ports.push_back(port);
                }
            }

            auto pins_size = site.getPins().size();
            sub_tile.num_phy_pins += pins_size * type.capacity;
            type.num_pins += pins_size * type.capacity;
            type.num_inst_pins += pins_size;

            type.num_input_pins += icount;
            type.num_output_pins += ocount;
            type.num_receivers += icount * type.capacity;
            type.num_drivers += ocount * type.capacity;

            type.pin_width_offset.resize(type.num_pins, 0);
            type.pin_height_offset.resize(type.num_pins, 0);

            type.pinloc.resize({1, 1, 4}, std::vector<bool>(type.num_pins, false));
            for (e_side side : {TOP, RIGHT, BOTTOM, LEFT}) {
                for (int pin = 0; pin < type.num_pins; pin++) {
                    type.pinloc[0][0][side][pin] = true;
                    type.pin_width_offset[pin] = 0;
                    type.pin_height_offset[pin] = 0;
                }
            }

            auto ltype = get_type_by_name<t_logical_block_type>(sub_tile.name, ltypes_);
            vtr::bimap<t_logical_pin, t_physical_pin> directs_map;

            for (int npin = 0; npin < type.num_pins; npin++) {
                t_physical_pin physical_pin(npin);
                t_logical_pin logical_pin(npin);

                directs_map.insert(logical_pin, physical_pin);
            }

            sub_tile.equivalent_sites.push_back(ltype);

            type.tile_block_pin_directs_map[ltype->index][sub_tile.index] = directs_map;

            // Assign FC specs
            int iblk_pin = 0;
            for (const auto& port : sub_tile.ports) {
                t_fc_specification fc_spec;

                fc_spec.seg_index = 0;

                //Apply type and defaults
                if (port.type == IN_PORT) {
                    fc_spec.fc_type = e_fc_type::IN;
                    fc_spec.fc_value_type = default_fc_.in_value_type;
                    fc_spec.fc_value = default_fc_.in_value;
                } else {
                    VTR_ASSERT(port.type == OUT_PORT);
                    fc_spec.fc_type = e_fc_type::OUT;
                    fc_spec.fc_value_type = default_fc_.out_value_type;
                    fc_spec.fc_value = default_fc_.out_value;
                }

                //Add all the pins from this port
                for (int iport_pin = 0; iport_pin < port.num_pins; ++iport_pin) {
                    int true_physical_blk_pin = sub_tile.sub_tile_to_tile_pin_indices[iblk_pin++];
                    fc_spec.pins.push_back(true_physical_blk_pin);
                }

                type.fc_specs.push_back(fc_spec);
            }

            type.sub_tiles.push_back(sub_tile);
        }
    }

    void process_luts() {
        // Add LUT Cell definitions
        // This is helpful to understand which cells are LUTs
        auto lut_def = ar_.getLutDefinitions();

        for (auto lut_cell : lut_def.getLutCells()) {
            t_lut_cell cell;
            cell.name = lut_cell.getCell().cStr();
            for (auto input : lut_cell.getInputPins())
                cell.inputs.push_back(input.cStr());

            auto equation = lut_cell.getEquation();
            if (equation.isInitParam())
                cell.init_param = equation.getInitParam().cStr();

            arch_->lut_cells.push_back(cell);
        }

        for (auto lut_elem : lut_def.getLutElements()) {
            for (auto lut : lut_elem.getLuts()) {
                for (auto bel : lut.getBels()) {
                    t_lut_bel lut_bel;

                    std::string name = bel.getName().cStr();
                    lut_bel.name = name;

                    // Check for duplicates
                    auto is_duplicate = [name](t_lut_bel l) { return l.name == name; };
                    auto res = std::find_if(arch_->lut_bels.begin(), arch_->lut_bels.end(), is_duplicate);
                    if (res != arch_->lut_bels.end())
                        continue;

                    std::vector<std::string> ipins;
                    for (auto pin : bel.getInputPins())
                        ipins.push_back(pin.cStr());

                    lut_bel.input_pins = ipins;
                    lut_bel.output_pin = bel.getOutputPin().cStr();

                    arch_->lut_bels.push_back(lut_bel);
                }
            }
        }
    }

    void process_package_pins() {
        for (auto package : ar_.getPackages()) {
            for (auto pin : package.getPackagePins()) {
                t_package_pin pckg_pin;
                pckg_pin.name = str(pin.getPackagePin());

                if (pin.getBel().isBel())
                    pckg_pin.bel_name = str(pin.getBel().getBel());

                if (pin.getSite().isSite())
                    pckg_pin.site_name = str(pin.getSite().getSite());

                arch_->pad_bels.push_back(pckg_pin);
            }
        }
    }

    // Layout Processing
    void process_layout() {
        auto tileList = ar_.getTileList();
        auto tileTypeList = ar_.getTileTypeList();

        std::vector<std::string> packages;
        for (auto package : ar_.getPackages())
            packages.push_back(str(package.getName()));

        for (auto name : packages) {
            t_grid_def grid_def;
            grid_def.width = grid_def.height = 0;
            for (auto tile : tileList) {
                grid_def.width = std::max(grid_def.width, tile.getCol() + 1);
                grid_def.height = std::max(grid_def.height, tile.getRow() + 1);
            }

            grid_def.grid_type = GridDefType::FIXED;

            if (name == "auto") {
                // At the moment, the interchange specifies fixed-layout only architectures,
                // and allowing for auto-sizing could potentially be implemented later on
                // to allow for experimentation on new architectures.
                // For the time being the layout is restricted to be only fixed.
                archfpga_throw(arch_file_, __LINE__,
                               "The name auto is reserved for auto-size layouts; please choose another name");
            }
            grid_def.name = name;
            for (auto tile : tileList) {
                t_metadata_dict data;
                std::string tile_prefix = str(tile.getName());
                auto tileType = tileTypeList[tile.getType()];
                std::string tile_type = str(tileType.getName());

                size_t pos = tile_prefix.find(tile_type);
                if (pos != std::string::npos && pos == 0)
                    tile_prefix.erase(pos, tile_type.length() + 1);
                data.add(arch_->strings.intern_string(vtr::string_view("fasm_prefix")),
                         arch_->strings.intern_string(vtr::string_view(tile_prefix.c_str())));
                t_grid_loc_def single(tile_type, 1);
                single.x.start_expr = std::to_string(tile.getCol());
                single.y.start_expr = std::to_string(tile.getRow());

                single.x.end_expr = single.x.start_expr + " + w - 1";
                single.y.end_expr = single.y.start_expr + " + h - 1";

                single.owned_meta = std::make_unique<t_metadata_dict>(data);
                single.meta = single.owned_meta.get();
                grid_def.loc_defs.emplace_back(std::move(single));
            }

            arch_->grid_layouts.emplace_back(std::move(grid_def));
        }
    }

    void process_device() {
        /*
         * The generic architecture data is not currently available in the interchange format
         * therefore, for a very initial implementation, the values are taken from the ones
         * used primarly in the Xilinx series7 devices, generated using SymbiFlow.
         *
         * As the interchange format develops further, with possibly more details, this function can
         * become dynamic, allowing for different parameters for the different architectures.
         *
         * FIXME: This will require to be dynamically assigned, and a suitable representation added
         *        to the FPGA interchange device schema.
         */
        arch_->R_minW_nmos = 6065.520020;
        arch_->R_minW_pmos = 18138.500000;
        arch_->grid_logic_tile_area = 14813.392;
        arch_->Chans.chan_x_dist.type = UNIFORM;
        arch_->Chans.chan_x_dist.peak = 1;
        arch_->Chans.chan_x_dist.width = 0;
        arch_->Chans.chan_x_dist.xpeak = 0;
        arch_->Chans.chan_x_dist.dc = 0;
        arch_->Chans.chan_y_dist.type = UNIFORM;
        arch_->Chans.chan_y_dist.peak = 1;
        arch_->Chans.chan_y_dist.width = 0;
        arch_->Chans.chan_y_dist.xpeak = 0;
        arch_->Chans.chan_y_dist.dc = 0;
        arch_->ipin_cblock_switch_name = std::string("generic");
        arch_->SBType = WILTON;
        arch_->Fs = 3;
        default_fc_.specified = true;
        default_fc_.in_value_type = e_fc_value_type::FRACTIONAL;
        default_fc_.in_value = 1.0;
        default_fc_.out_value_type = e_fc_value_type::FRACTIONAL;
        default_fc_.out_value = 1.0;
    }

    void process_switches() {
        std::set<std::pair<bool, uint32_t>> pip_timing_models;
        for (auto tile_type : ar_.getTileTypeList()) {
            for (auto pip : tile_type.getPips()) {
                pip_timing_models.insert(std::pair<bool, uint32_t>(pip.getBuffered21(), pip.getTiming()));
                if (!pip.getDirectional())
                    pip_timing_models.insert(std::pair<bool, uint32_t>(pip.getBuffered20(), pip.getTiming()));
            }
        }

        auto timing_data = ar_.getPipTimings();

        std::vector<std::pair<bool, uint32_t>> pip_timing_models_list;
        pip_timing_models_list.reserve(pip_timing_models.size());

        for (auto entry : pip_timing_models) {
            pip_timing_models_list.push_back(entry);
        }

        auto num_switches = pip_timing_models.size() + 2;
        std::string switch_name;

        arch_->num_switches = num_switches;

        if (num_switches > 0) {
            arch_->Switches = new t_arch_switch_inf[num_switches];
        }

        float R, Cin, Cint, Cout, Tdel;
        for (int i = 0; i < (int)num_switches; ++i) {
            t_arch_switch_inf* as = &arch_->Switches[i];

            R = Cin = Cint = Cout = Tdel = 0.0;
            SwitchType type;

            if (i == 0) {
                switch_name = "short";
                type = SwitchType::SHORT;
                R = 0.0;
            } else if (i == 1) {
                switch_name = "generic";
                type = SwitchType::MUX;
                R = 0.0;
            } else {
                auto entry = pip_timing_models_list[i - 2];
                auto model = timing_data[entry.second];
                std::stringstream name;
                std::string mux_type_string = entry.first ? "mux_" : "passGate_";
                name << mux_type_string;

                // FIXME: allow to dynamically choose different speed models and corners
                R = get_corner_value(model.getOutputResistance(), "slow", "min");
                name << "R" << std::scientific << R;

                Cin = get_corner_value(model.getInputCapacitance(), "slow", "min");
                name << "Cin" << std::scientific << Cin;

                Cout = get_corner_value(model.getOutputCapacitance(), "slow", "min");
                name << "Cout" << std::scientific << Cout;

                if (entry.first) {
                    Cint = get_corner_value(model.getInternalCapacitance(), "slow", "min");
                    name << "Cinternal" << std::scientific << Cint;
                }

                Tdel = get_corner_value(model.getInternalDelay(), "slow", "min");
                name << "Tdel" << std::scientific << Tdel;

                switch_name = name.str();
                type = entry.first ? SwitchType::MUX : SwitchType::PASS_GATE;
            }

            /* Should never happen */
            if (switch_name == std::string(VPR_DELAYLESS_SWITCH_NAME)) {
                archfpga_throw(arch_file_, __LINE__,
                               "Switch name '%s' is a reserved name for VPR internal usage!", switch_name.c_str());
            }

            as->name = vtr::strdup(switch_name.c_str());
            as->set_type(type);
            as->mux_trans_size = as->type() == SwitchType::MUX ? 1 : 0;

            as->R = R;
            as->Cin = Cin;
            as->Cout = Cout;
            as->Cinternal = Cint;
            as->set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, Tdel);

            if (as->type() == SwitchType::SHORT || as->type() == SwitchType::PASS_GATE) {
                as->buf_size_type = BufferSize::ABSOLUTE;
                as->buf_size = 0;
                as->power_buffer_type = POWER_BUFFER_TYPE_ABSOLUTE_SIZE;
                as->power_buffer_size = 0.;
            } else {
                as->buf_size_type = BufferSize::AUTO;
                as->buf_size = 0.;
                as->power_buffer_type = POWER_BUFFER_TYPE_AUTO;
            }
        }
    }

    void process_segments() {
        // Segment names will be taken from wires connected to pips
        // They are good representation for nodes
        std::set<uint32_t> wire_names;
        for (auto tile_type : ar_.getTileTypeList()) {
            auto wires = tile_type.getWires();
            for (auto pip : tile_type.getPips()) {
                wire_names.insert(wires[pip.getWire0()]);
                wire_names.insert(wires[pip.getWire1()]);
            }
        }
        int num_seg = wire_names.size();
        arch_->Segments.resize(num_seg);
        uint32_t index = 0;
        for (auto i : wire_names) {
            // Use default values as we will populate rr_graph with correct values
            // This segments are just declaration of future use
            arch_->Segments[index].name = str(i);
            arch_->Segments[index].length = 1;
            arch_->Segments[index].frequency = 1;
            arch_->Segments[index].Rmetal = 0;
            arch_->Segments[index].Cmetal = 0;
            arch_->Segments[index].parallel_axis = BOTH_AXIS;

            // TODO: Only bi-directional segments are created, but it the interchange format
            //       has directionality information on PIPs, which may be used to infer the
            //       segments' directonality.
            arch_->Segments[index].directionality = BI_DIRECTIONAL;
            arch_->Segments[index].arch_wire_switch = 1;
            arch_->Segments[index].arch_opin_switch = 1;
            arch_->Segments[index].cb.resize(1);
            arch_->Segments[index].cb[0] = true;
            arch_->Segments[index].sb.resize(2);
            arch_->Segments[index].sb[0] = true;
            arch_->Segments[index].sb[1] = true;
            ++index;
        }
    }
};

void FPGAInterchangeReadArch(const char* FPGAInterchangeDeviceFile,
                             const bool /*timing_enabled*/,
                             t_arch* arch,
                             std::vector<t_physical_tile_type>& PhysicalTileTypes,
                             std::vector<t_logical_block_type>& LogicalBlockTypes) {
    // Decompress GZipped capnproto device file
    gzFile file = gzopen(FPGAInterchangeDeviceFile, "r");
    VTR_ASSERT(file != Z_NULL);

    std::vector<uint8_t> output_data;
    output_data.resize(4096);
    std::stringstream sstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    while (true) {
        int ret = gzread(file, output_data.data(), output_data.size());
        VTR_ASSERT(ret >= 0);
        if (ret > 0) {
            sstream.write((const char*)output_data.data(), ret);
            VTR_ASSERT(sstream);
        } else {
            VTR_ASSERT(ret == 0);
            int error;
            gzerror(file, &error);
            VTR_ASSERT(error == Z_OK);
            break;
        }
    }

    VTR_ASSERT(gzclose(file) == Z_OK);

    sstream.seekg(0);
    kj::std::StdInputStream istream(sstream);

    // Reader options
    capnp::ReaderOptions reader_options;
    reader_options.nestingLimit = std::numeric_limits<int>::max();
    reader_options.traversalLimitInWords = std::numeric_limits<uint64_t>::max();

    capnp::InputStreamMessageReader message_reader(istream, reader_options);

    auto device_reader = message_reader.getRoot<DeviceResources::Device>();

    arch->architecture_id = vtr::strdup(vtr::secure_digest_file(FPGAInterchangeDeviceFile).c_str());

    ArchReader reader(arch, device_reader, FPGAInterchangeDeviceFile, PhysicalTileTypes, LogicalBlockTypes);
    reader.read_arch();
}
