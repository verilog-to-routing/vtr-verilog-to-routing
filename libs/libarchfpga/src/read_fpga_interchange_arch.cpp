#include <algorithm>
#include <kj/std/iostream.h>
#include <limits>
#include <map>
#include <regex>
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

// Necessary to reduce code verbosity when getting the pin directions
static const auto INPUT = LogicalNetlist::Netlist::Direction::INPUT;
static const auto OUTPUT = LogicalNetlist::Netlist::Direction::OUTPUT;
static const auto INOUT = LogicalNetlist::Netlist::Direction::INOUT;

static const auto LOGIC = Device::BELCategory::LOGIC;
static const auto ROUTING = Device::BELCategory::ROUTING;
static const auto SITE_PORT = Device::BELCategory::SITE_PORT;

// Enum for pack pattern expansion direction
enum e_pp_dir {
    FORWARD = 0,
    BACKWARD = 1
};

struct t_package_pin {
    std::string name;

    std::string site_name;
    std::string bel_name;
};

struct t_bel_cell_mapping {
    size_t cell;
    size_t site;
    std::vector<std::pair<size_t, size_t>> pins;

    bool operator<(const t_bel_cell_mapping& other) const {
        return cell < other.cell || (cell == other.cell && site < other.site);
    }
};

// Intermediate data type to store information on interconnects to be created
struct t_ic_data {
    std::string input;
    std::set<std::string> outputs;

    bool requires_pack_pattern;
};

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

/** @brief Returns the port corresponding to the given model in the architecture */
static t_model_ports* get_model_port(t_arch* arch, std::string model, std::string port, bool fail = true) {
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

    if (fail)
        archfpga_throw(__FILE__, __LINE__,
                       "Could not find model port: %s (%s)\n", port.c_str(), model.c_str());

    return nullptr;
}

/** @brief Returns the specified architecture model */
static t_model* get_model(t_arch* arch, std::string model) {
    for (t_model* m : {arch->models, arch->model_library})
        for (; m != nullptr; m = m->next)
            if (std::string(m->name) == model)
                return m;

    archfpga_throw(__FILE__, __LINE__,
                   "Could not find model: %s\n", model.c_str());
}

/** @brief Returns the physical or logical type by its name */
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

/** @brief Returns a generic port instantiation for a complex block */
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
    port.is_non_clock_global = false;
    port.model_port = nullptr;
    port.port_class = vtr::strdup(nullptr);
    port.port_power = (t_port_power*)vtr::calloc(1, sizeof(t_port_power));

    if (!model.empty())
        port.model_port = get_model_port(arch, model, name);

    return port;
}

/** @brief Returns true if a given port name exists in the given complex block */
static bool block_port_exists(t_pb_type* pb_type, std::string port_name) {
    for (int iport = 0; iport < pb_type->num_ports; iport++) {
        const t_port port = pb_type->ports[iport];

        if (std::string(port.name) == port_name)
            return true;
    }

    return false;
}

/** @brief Returns a pack pattern given it's name, input and output strings */
static t_pin_to_pin_annotation get_pack_pattern(std::string pp_name, std::string input, std::string output) {
    t_pin_to_pin_annotation pp;

    pp.prop = (int*)vtr::calloc(1, sizeof(int));
    pp.value = (char**)vtr::calloc(1, sizeof(char*));

    pp.type = E_ANNOT_PIN_TO_PIN_PACK_PATTERN;
    pp.format = E_ANNOT_PIN_TO_PIN_CONSTANT;
    pp.prop[0] = (int)E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME;
    pp.value[0] = vtr::strdup(pp_name.c_str());
    pp.input_pins = vtr::strdup(input.c_str());
    pp.output_pins = vtr::strdup(output.c_str());
    pp.num_value_prop_pairs = 1;
    pp.clock = nullptr;

    return pp;
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

        for (std::string str : ar_.getStrList()) {
            auto interned_string = arch_->strings.intern_string(vtr::string_view(str.c_str()));
            arch_->interned_strings.push_back(interned_string);
        }
    }

    void read_arch() {
        // Preprocess arch information
        process_luts();
        process_package_pins();
        process_cell_bel_mappings();
        process_constants();
        process_bels_and_sites();

        process_models();
        process_constant_model();

        process_device();

        process_layout();
        process_switches();
        process_segments();

        process_sites();
        process_constant_block();

        process_tiles();
        process_constant_tile();

        link_physical_logical_types(ptypes_, ltypes_);

        SyncModelsPbTypes(arch_, ltypes_);
        check_models(arch_);
    }

  private:
    t_arch* arch_;
    const char* arch_file_;
    Device::Reader& ar_;
    std::vector<t_physical_tile_type>& ptypes_;
    std::vector<t_logical_block_type>& ltypes_;

    t_default_fc_spec default_fc_;

    std::string bel_dedup_suffix_ = "_bel";
    std::string const_block_ = "constant_block";

    std::unordered_set<int> take_bels_;
    std::unordered_set<int> take_sites_;

    // Package pins

    // TODO: add possibility to have multiple packages
    std::vector<t_package_pin> package_pins_;
    std::unordered_set<std::string> pad_bels_;
    std::string out_suffix_ = "_out";
    std::string in_suffix_ = "_in";

    // Bel Cell mappings
    std::unordered_map<uint32_t, std::set<t_bel_cell_mapping>> bel_cell_mappings_;
    std::unordered_map<std::string, int> segment_name_to_segment_idx;

    // Utils

    /** @brief Returns the string corresponding to the given index */
    std::string str(size_t idx) {
        return arch_->interned_strings[idx].get(&arch_->strings);
    }

    /** @brief Get the BEL count of a site depending on its category (e.g. logic or routing BELs) */
    int get_bel_type_count(Device::SiteType::Reader& site, Device::BELCategory category, bool skip_lut = false) {
        int count = 0;
        for (auto bel : site.getBels()) {
            auto bel_name = str(bel.getName());
            bool is_logic = category == LOGIC;

            if (skip_lut && is_lut(bel_name, str(site.getName())))
                continue;

            bool skip_bel = is_logic && take_bels_.count(bel.getName()) == 0;

            if (bel.getCategory() == category && !skip_bel)
                count++;
        }

        return count;
    }

    /** @brief Get the BEL reader given its name and site */
    Device::BEL::Reader get_bel_reader(Device::SiteType::Reader& site, std::string bel_name) {
        for (auto bel : site.getBels())
            if (str(bel.getName()) == bel_name)
                return bel;
        VTR_ASSERT_MSG(0, "Could not find the BEL reader!\n");
    }

    /** @brief Get the BEL pin reader given its name, site and corresponding BEL */
    Device::BELPin::Reader get_bel_pin_reader(Device::SiteType::Reader& site, Device::BEL::Reader& bel, std::string pin_name) {
        auto bel_pins = site.getBelPins();

        for (auto bel_pin : bel.getPins()) {
            auto pin_reader = bel_pins[bel_pin];
            if (str(pin_reader.getName()) == pin_name)
                return pin_reader;
        }
        VTR_ASSERT_MSG(0, "Could not find the BEL pin reader!\n");
    }

    /** @brief Get the BEL name, with an optional deduplication suffix in case its name collides with the site name */
    std::string get_bel_name(Device::SiteType::Reader& site, Device::BEL::Reader& bel) {
        if (bel.getCategory() == SITE_PORT)
            return str(site.getName());

        auto site_name = str(site.getName());
        auto bel_name = str(bel.getName());

        return site_name == bel_name ? bel_name + bel_dedup_suffix_ : bel_name;
    }

    /** @brief Returns the name of the input argument BEL with optionally the de-duplication suffix removed */
    std::string remove_bel_suffix(std::string bel) {
        std::smatch regex_matches;
        std::string regex = std::string("(.*)") + bel_dedup_suffix_;
        const std::regex bel_regex(regex.c_str());
        if (std::regex_match(bel, regex_matches, bel_regex))
            return regex_matches[1].str();

        return bel;
    }

    /** @brief Returns true in case the input argument corresponds to the name of a LUT */
    bool is_lut(std::string name, const std::string site = std::string()) {
        for (auto cell : arch_->lut_cells)
            if (cell.name == name)
                return true;

        for (const auto& it : arch_->lut_elements) {
            if (!site.empty() && site != it.first) {
                continue;
            }

            for (const auto& lut_element : it.second) {
                for (const auto& lut_bel : lut_element.lut_bels) {
                    if (lut_bel.name == name) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    t_lut_element* get_lut_element_for_bel(const std::string& site_type, const std::string& bel_name) {
        if (!arch_->lut_elements.count(site_type)) {
            return nullptr;
        }

        for (auto& lut_element : arch_->lut_elements.at(site_type)) {
            for (auto& lut_bel : lut_element.lut_bels) {
                if (lut_bel.name == bel_name) {
                    return &lut_element;
                }
            }
        }

        return nullptr;
    }

    /** @brief Returns true in case the input argument corresponds to a PAD BEL */
    bool is_pad(std::string name) {
        return pad_bels_.count(name) != 0;
    }

    /** @brief Utility function to fill in all the necessary information for the sub_tile
     *
     *  Given a physical tile type and a corresponding sub tile with additional information on the IO pin count
     *  this function populates all the data structures corresponding to the sub tile, and modifies also the parent
     *  physical tile type, updating the pin numberings as  well as the directs pin mapping for the equivalent sites
     *
     *  Affected data structures:
     *      - pinloc
     *      - fc_specs
     *      - equivalent_sites
     *      - tile_block_pin_directs_map
     **/
    void fill_sub_tile(t_physical_tile_type& type, t_sub_tile& sub_tile, int num_pins, int input_count, int output_count) {
        sub_tile.num_phy_pins += num_pins;
        type.num_pins += num_pins;
        type.num_inst_pins += num_pins;

        type.num_input_pins += input_count;
        type.num_output_pins += output_count;
        type.num_receivers += input_count;
        type.num_drivers += output_count;

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

        vtr::bimap<t_logical_pin, t_physical_pin> directs_map;

        for (int npin = 0; npin < type.num_pins; npin++) {
            t_physical_pin physical_pin(npin);
            t_logical_pin logical_pin(npin);

            directs_map.insert(logical_pin, physical_pin);
        }

        auto ltype = get_type_by_name<t_logical_block_type>(sub_tile.name, ltypes_);
        sub_tile.equivalent_sites.push_back(ltype);

        type.tile_block_pin_directs_map[ltype->index][sub_tile.index] = directs_map;

        // Assign FC specs
        int iblk_pin = 0;
        for (const auto& port : sub_tile.ports) {
            t_fc_specification fc_spec;

            // FIXME: Use always one segment for the time being.
            //        Can use the right segment for this IOPIN as soon
            //        as the RR graph reading from the interchange is complete.
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
    }

    /** @brief Returns an intermediate map representing all the interconnects to be added in a site */
    std::unordered_map<std::string, t_ic_data> get_interconnects(Device::SiteType::Reader& site) {
        // dictionary:
        //   - key: interconnect name
        //   - value: interconnect data
        std::unordered_map<std::string, t_ic_data> ics;

        const std::string site_type = str(site.getName());

        for (auto wire : site.getSiteWires()) {
            std::string wire_name = str(wire.getName());

            // pin name, bel name
            int pin_id = OPEN;
            bool pad_exists = false;
            bool all_inout_pins = true;
            std::string pad_bel_name;
            std::string pad_bel_pin_name;
            for (auto pin : wire.getPins()) {
                auto bel_pin = site.getBelPins()[pin];
                auto dir = bel_pin.getDir();
                std::string bel_pin_name = str(bel_pin.getName());

                auto bel = get_bel_reader(site, str(bel_pin.getBel()));
                auto bel_name = get_bel_name(site, bel);

                auto bel_is_pad = is_pad(bel_name);

                pad_exists |= bel_is_pad;
                all_inout_pins &= dir == INOUT;

                if (bel_is_pad) {
                    pad_bel_name = bel_name;
                    pad_bel_pin_name = bel_pin_name;
                }

                if (dir == OUTPUT)
                    pin_id = pin;
            }

            if (pin_id == OPEN) {
                // If no driver pin has been found, the assumption is that
                // there must be a PAD with inout pin connected to other inout pins
                for (auto pin : wire.getPins()) {
                    auto bel_pin = site.getBelPins()[pin];
                    std::string bel_pin_name = str(bel_pin.getName());

                    auto bel = get_bel_reader(site, str(bel_pin.getBel()));
                    auto bel_name = get_bel_name(site, bel);

                    if (!is_pad(bel_name))
                        continue;

                    pin_id = pin;
                }
            }

            VTR_ASSERT(pin_id != OPEN);

            auto out_pin = site.getBelPins()[pin_id];
            auto out_pin_bel = get_bel_reader(site, str(out_pin.getBel()));
            auto out_pin_name = str(out_pin.getName());

            for (auto pin : wire.getPins()) {
                if ((int)pin == pin_id)
                    continue;

                auto bel_pin = site.getBelPins()[pin];
                std::string out_bel_pin_name = str(bel_pin.getName());

                auto out_bel = get_bel_reader(site, str(bel_pin.getBel()));
                auto out_bel_name = get_bel_name(site, out_bel);

                auto in_bel = out_pin_bel;
                auto in_bel_name = get_bel_name(site, in_bel);
                auto in_bel_pin_name = out_pin_name;

                bool skip_in_bel = in_bel.getCategory() == LOGIC && take_bels_.count(in_bel.getName()) == 0;
                bool skip_out_bel = out_bel.getCategory() == LOGIC && take_bels_.count(out_bel.getName()) == 0;
                if (skip_in_bel || skip_out_bel)
                    continue;

                // LUT bels are nested under pb_types which represent LUT
                // elements. Check if a BEL belongs to a LUT element and
                // adjust pb_type name in the interconnect accordingly.
                auto get_lut_element_index = [&](const std::string& bel_name) {
                    auto lut_element = get_lut_element_for_bel(site_type, bel_name);
                    if (lut_element == nullptr)
                        return -1;

                    const auto& lut_elements = arch_->lut_elements.at(site_type);
                    auto it = std::find(lut_elements.begin(), lut_elements.end(), *lut_element);
                    VTR_ASSERT(it != lut_elements.end());

                    return (int)std::distance(lut_elements.begin(), it);
                };

                // TODO: This avoids having LUTs that can be used in other ways than LUTs, e.g. as DRAMs.
                //       Once support is added for macro expansion, all the connections currently marked as
                //       invalid will be re-enabled.
                auto is_lut_connection_valid = [&](const std::string& bel_name, const std::string& pin_name) {
                    auto lut_element = get_lut_element_for_bel(site_type, bel_name);
                    if (lut_element == nullptr)
                        return false;

                    bool pin_found = false;
                    for (auto lut_bel : lut_element->lut_bels) {
                        for (auto lut_bel_pin : lut_bel.input_pins)
                            pin_found |= lut_bel_pin == pin_name;

                        pin_found |= lut_bel.output_pin == pin_name;
                    }

                    if (!pin_found)
                        return false;

                    return true;
                };

                int index = get_lut_element_index(out_bel_name);
                bool valid_lut = is_lut_connection_valid(out_bel_name, out_bel_pin_name);
                if (index >= 0) {
                    out_bel_name = "LUT" + std::to_string(index);

                    if (!valid_lut)
                        continue;
                }

                index = get_lut_element_index(in_bel_name);
                valid_lut = is_lut_connection_valid(in_bel_name, in_bel_pin_name);
                if (index >= 0) {
                    in_bel_name = "LUT" + std::to_string(index);

                    if (!valid_lut)
                        continue;
                }

                std::string ostr = out_bel_name + "." + out_bel_pin_name;
                std::string istr = in_bel_name + "." + in_bel_pin_name;

                // TODO: If the bel pin is INOUT (e.g. PULLDOWN/PULLUP in Series7)
                //       for now treat as input only and assign the in suffix
                if (bel_pin.getDir() == INOUT && !all_inout_pins && !is_pad(out_bel_name))
                    ostr += in_suffix_;

                auto ic_name = wire_name + "_" + out_bel_pin_name;

                bool requires_pack_pattern = pad_exists;

                std::vector<std::pair<std::string, t_ic_data>> ics_data;
                if (all_inout_pins) {
                    std::string extra_istr = out_bel_name + "." + out_bel_pin_name + out_suffix_;
                    std::string extra_ostr = in_bel_name + "." + in_bel_pin_name + in_suffix_;
                    std::string extra_ic_name = ic_name + "_extra";

                    std::set<std::string> extra_ostrs{extra_ostr};
                    t_ic_data extra_ic_data = {
                        extra_istr,           // ic input
                        extra_ostrs,          // ic outputs
                        requires_pack_pattern // pack pattern required
                    };

                    ics_data.push_back(std::make_pair(extra_ic_name, extra_ic_data));

                    istr += out_suffix_;
                    ostr += in_suffix_;
                } else if (pad_exists) {
                    if (out_bel_name == pad_bel_name)
                        ostr += in_suffix_;
                    else { // Create new wire to connect PAD output to the BELs input
                        ic_name = wire_name + "_" + pad_bel_pin_name + out_suffix_;
                        istr = pad_bel_name + "." + pad_bel_pin_name + out_suffix_;
                    }
                }

                std::set<std::string> ostrs{ostr};
                t_ic_data ic_data = {
                    istr,
                    ostrs,
                    requires_pack_pattern};

                ics_data.push_back(std::make_pair(ic_name, ic_data));

                for (auto entry : ics_data) {
                    auto name = entry.first;
                    auto data = entry.second;

                    auto res = ics.emplace(name, data);

                    if (!res.second) {
                        auto old_data = res.first->second;

                        VTR_ASSERT(old_data.input == data.input);
                        VTR_ASSERT(data.outputs.size() == 1);

                        for (auto out : data.outputs)
                            res.first->second.outputs.insert(out);
                        res.first->second.requires_pack_pattern |= data.requires_pack_pattern;
                    }
                }
            }
        }

        return ics;
    }

    /**
     * Preprocessors:
     *   - process_bels_and_sites: information on whether sites and bels need to be expanded in pb types
     *   - process_luts: processes information on which cells and bels are LUTs
     *   - process_package_pins: processes information on the device's pinout and which sites and bels
     *                           contain IO pads
     *   - process_cell_bel_mapping: processes mappings between a cell and the possible BELs location for that cell
     *   - process_constants: processes constants cell and net names
     */
    void process_bels_and_sites() {
        auto tiles = ar_.getTileList();
        auto tile_types = ar_.getTileTypeList();
        auto site_types = ar_.getSiteTypeList();

        for (auto tile : tiles) {
            auto tile_type = tile_types[tile.getType()];

            for (auto site : tile.getSites()) {
                auto site_type_in_tile = tile_type.getSiteTypes()[site.getType()];
                auto site_type = site_types[site_type_in_tile.getPrimaryType()];

                bool found = false;
                for (auto bel : site_type.getBels()) {
                    auto bel_name = bel.getName();
                    bool res = bel_cell_mappings_.find(bel_name) != bel_cell_mappings_.end();

                    found |= res;

                    if (res || is_pad(str(bel_name)))
                        take_bels_.insert(bel_name);
                }

                if (found)
                    take_sites_.insert(site_type.getName());

                // TODO: Enable also alternative site types handling
            }
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
                t_lut_element element;
                element.site_type = lut_elem.getSite().cStr();
                element.width = lut.getWidth();

                for (auto bel : lut.getBels()) {
                    t_lut_bel lut_bel;
                    lut_bel.name = bel.getName().cStr();
                    std::vector<std::string> ipins;

                    for (auto pin : bel.getInputPins())
                        ipins.push_back(pin.cStr());

                    lut_bel.input_pins = ipins;
                    lut_bel.output_pin = bel.getOutputPin().cStr();

                    element.lut_bels.push_back(lut_bel);
                }

                arch_->lut_elements[element.site_type].push_back(element);
            }
        }
    }

    void process_package_pins() {
        for (auto package : ar_.getPackages()) {
            for (auto pin : package.getPackagePins()) {
                t_package_pin pckg_pin;
                pckg_pin.name = str(pin.getPackagePin());

                if (pin.getBel().isBel()) {
                    pckg_pin.bel_name = str(pin.getBel().getBel());
                    pad_bels_.insert(pckg_pin.bel_name);
                }

                if (pin.getSite().isSite())
                    pckg_pin.site_name = str(pin.getSite().getSite());

                package_pins_.push_back(pckg_pin);
            }
        }
    }

    void process_cell_bel_mappings() {
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

    void process_constants() {
        auto consts = ar_.getConstants();

        arch_->gnd_cell = std::make_pair(str(consts.getGndCellType()), str(consts.getGndCellPin()));
        arch_->vcc_cell = std::make_pair(str(consts.getVccCellType()), str(consts.getVccCellPin()));

        arch_->gnd_net = consts.getGndNetName().isName() ? str(consts.getGndNetName().getName()) : "$__gnd_net";
        arch_->vcc_net = consts.getVccNetName().isName() ? str(consts.getVccNetName().getName()) : "$__vcc_net";
    }

    /* end preprocessors */

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

                if (is_lut(prim_name))
                    continue;

                // Check whether the model can be placed in at least one
                // BEL that was marked as valid (e.g. added to the take_bels_ data structure)
                bool has_bel = false;
                for (auto bel_cell_map : bel_cell_mappings_) {
                    auto bel_name = bel_cell_map.first;

                    bool take_bel = take_bels_.count(bel_name) != 0;

                    if (!take_bel || is_lut(str(bel_name)))
                        continue;

                    for (auto map : bel_cell_map.second)
                        has_bel |= primitive.getName() == map.cell;
                }

                if (!has_bel)
                    continue;

                try {
                    temp = new t_model;
                    temp->index = model_index++;

                    temp->never_prune = true;
                    temp->name = vtr::strdup(prim_name.c_str());

                    ret_map_name = model_name_map.insert(std::pair<std::string, int>(temp->name, 0));
                    if (!ret_map_name.second) {
                        archfpga_throw(arch_file_, __LINE__,
                                       "Duplicate model name: '%s'.\n", temp->name);
                    }

                    if (!process_model_ports(temp, primitive)) {
                        free_arch_model(temp);
                        continue;
                    }

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

    bool process_model_ports(t_model* model, Netlist::CellDeclaration::Reader primitive) {
        auto primLib = ar_.getPrimLibs();
        auto portList = primLib.getPortList();

        std::set<std::pair<std::string, enum PORTS>> port_names;

        for (auto port_idx : primitive.getPorts()) {
            auto port = portList[port_idx];
            enum PORTS dir = ERR_PORT;
            switch (port.getDir()) {
                case INPUT:
                    dir = IN_PORT;
                    break;
                case OUTPUT:
                    dir = OUT_PORT;
                    break;
                case INOUT:
                    return false;
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

        return true;
    }

    // Complex Blocks
    void process_sites() {
        auto siteTypeList = ar_.getSiteTypeList();

        int index = 0;
        // TODO: Make this dynamic depending on data from the interchange
        auto EMPTY = get_empty_logical_type();
        EMPTY.index = index;
        ltypes_.push_back(EMPTY);

        for (auto site : siteTypeList) {
            auto bels = site.getBels();

            if (bels.size() == 0)
                continue;

            t_logical_block_type ltype;

            std::string name = str(site.getName());

            if (take_sites_.count(site.getName()) == 0)
                continue;

            // Check for duplicates
            auto is_duplicate = [name](t_logical_block_type l) { return std::string(l.name) == name; };
            VTR_ASSERT(std::find_if(ltypes_.begin(), ltypes_.end(), is_duplicate) == ltypes_.end());

            ltype.name = vtr::strdup(name.c_str());
            ltype.index = ++index;

            auto pb_type = new t_pb_type;
            ltype.pb_type = pb_type;

            pb_type->name = vtr::strdup(name.c_str());
            pb_type->num_pb = 1;
            process_block_ports(pb_type, site);

            // Process modes (for simplicity, only the default mode is allowed for the time being)
            pb_type->num_modes = 1;
            pb_type->modes = new t_mode[pb_type->num_modes];

            auto mode = &pb_type->modes[0];
            mode->parent_pb_type = pb_type;
            mode->index = 0;
            mode->name = vtr::strdup("default");
            mode->disable_packing = false;

            // Get LUT elements for this site
            std::vector<t_lut_element> lut_elements;
            if (arch_->lut_elements.count(name))
                lut_elements = arch_->lut_elements.at(name);

            // Count non-LUT BELs plus LUT elements
            int block_count = get_bel_type_count(site, LOGIC, true) + get_bel_type_count(site, ROUTING, true) + lut_elements.size();

            mode->num_pb_type_children = block_count;
            mode->pb_type_children = new t_pb_type[mode->num_pb_type_children];

            // Add regular BELs
            int count = 0;
            for (auto bel : bels) {
                auto category = bel.getCategory();
                if (bel.getCategory() == SITE_PORT)
                    continue;

                bool is_logic = category == LOGIC;

                if (take_bels_.count(bel.getName()) == 0 && is_logic)
                    continue;

                if (is_lut(str(bel.getName()), name))
                    continue;

                auto bel_name = str(bel.getName());
                std::pair<std::string, std::string> key(name, bel_name);

                auto mid_pb_type = &mode->pb_type_children[count++];
                std::string mid_pb_type_name = bel_name == name ? bel_name + bel_dedup_suffix_ : bel_name;

                mid_pb_type->name = vtr::strdup(mid_pb_type_name.c_str());
                mid_pb_type->num_pb = 1;
                mid_pb_type->parent_mode = mode;
                mid_pb_type->blif_model = nullptr;

                if (!is_pad(bel_name))
                    process_block_ports(mid_pb_type, site, bel.getName());

                if (is_pad(bel_name))
                    process_pad_block(mid_pb_type, bel, site);
                else if (is_logic)
                    process_generic_block(mid_pb_type, bel, site);
                else {
                    VTR_ASSERT(category == ROUTING);
                    process_routing_block(mid_pb_type);
                }
            }

            // Add LUT elements
            for (size_t i = 0; i < lut_elements.size(); ++i) {
                const auto& lut_element = lut_elements[i];

                auto mid_pb_type = &mode->pb_type_children[count++];
                std::string lut_name = "LUT" + std::to_string(i);
                mid_pb_type->name = vtr::strdup(lut_name.c_str());
                mid_pb_type->num_pb = 1;
                mid_pb_type->parent_mode = mode;
                mid_pb_type->blif_model = nullptr;

                process_lut_element(mid_pb_type, lut_element);
            }

            process_interconnects(mode, site);
            ltypes_.push_back(ltype);
        }
    }

    /** @brief Processes a LUT element starting from the intermediate pb type */
    void process_lut_element(t_pb_type* parent, const t_lut_element& lut_element) {
        // Collect ports for the parent pb_type representing the whole LUT
        // element
        std::set<std::tuple<std::string, PORTS, int>> parent_ports;
        for (const auto& lut_bel : lut_element.lut_bels) {
            for (const auto& name : lut_bel.input_pins) {
                parent_ports.emplace(name, IN_PORT, 1);
            }

            parent_ports.emplace(lut_bel.output_pin, OUT_PORT, 1);
        }

        // Create the ports
        create_ports(parent, parent_ports);

        // Make a single mode for each member LUT of the LUT element
        parent->num_modes = (int)lut_element.lut_bels.size();
        parent->modes = new t_mode[parent->num_modes];

        for (size_t i = 0; i < lut_element.lut_bels.size(); ++i) {
            const t_lut_bel& lut_bel = lut_element.lut_bels[i];
            auto mode = &parent->modes[i];

            mode->name = vtr::strdup(lut_bel.name.c_str());
            mode->parent_pb_type = parent;
            mode->index = i;

            // Leaf pb_type block for the LUT
            mode->num_pb_type_children = 1;
            mode->pb_type_children = new t_pb_type[mode->num_pb_type_children];

            auto pb_type = &mode->pb_type_children[0];
            pb_type->name = vtr::strdup(lut_bel.name.c_str());
            pb_type->num_pb = 1;
            pb_type->parent_mode = mode;
            pb_type->blif_model = nullptr;

            process_lut_block(pb_type, lut_bel);

            // Mode interconnect
            mode->num_interconnect = lut_bel.input_pins.size() + 1;
            mode->interconnect = new t_interconnect[mode->num_interconnect];

            std::string istr, ostr, name;

            // Inputs
            for (size_t j = 0; j < lut_bel.input_pins.size(); ++j) {
                auto* ic = &mode->interconnect[j];

                ic->type = DIRECT_INTERC;
                ic->parent_mode = mode;
                ic->parent_mode_index = mode->index;

                istr = std::string(parent->name) + "." + lut_bel.input_pins[j];
                ostr = std::string(pb_type->name) + ".in[" + std::to_string(j) + "]";
                name = istr + "_to_" + ostr;

                ic->input_string = vtr::strdup(istr.c_str());
                ic->output_string = vtr::strdup(ostr.c_str());
                ic->name = vtr::strdup(name.c_str());
            }

            // Output
            auto* ic = &mode->interconnect[mode->num_interconnect - 1];
            ic->type = DIRECT_INTERC;
            ic->parent_mode = mode;
            ic->parent_mode_index = mode->index;

            istr = std::string(pb_type->name) + ".out";
            ostr = std::string(parent->name) + "." + lut_bel.output_pin;
            name = istr + "_to_" + ostr;

            ic->input_string = vtr::strdup(istr.c_str());
            ic->output_string = vtr::strdup(ostr.c_str());
            ic->name = vtr::strdup(name.c_str());
        }
    }

    /** @brief Processes a LUT primitive starting from the intermediate pb type */
    void process_lut_block(t_pb_type* pb_type, const t_lut_bel& lut_bel) {
        // Create port list
        size_t width = lut_bel.input_pins.size();

        std::set<std::tuple<std::string, PORTS, int>> ports;
        ports.emplace("in", IN_PORT, width);
        ports.emplace("out", OUT_PORT, 1);

        create_ports(pb_type, ports);

        // Make two modes. One for LUT-thru and another for the actual LUT bel
        pb_type->num_modes = 2;
        pb_type->modes = new t_mode[pb_type->num_modes];

        // ................................................
        // LUT-thru
        t_mode* mode = &pb_type->modes[0];

        // Mode
        mode->name = vtr::strdup("wire");
        mode->parent_pb_type = pb_type;
        mode->index = 0;
        mode->num_pb_type_children = 0;

        // Mode interconnect
        mode->num_interconnect = 1;
        mode->interconnect = new t_interconnect[mode->num_interconnect];
        t_interconnect* ic = &mode->interconnect[0];

        std::string istr, ostr, name;

        istr = std::string(pb_type->name) + ".in";
        ostr = std::string(pb_type->name) + ".out";
        name = "passthrough";

        ic->input_string = vtr::strdup(istr.c_str());
        ic->output_string = vtr::strdup(ostr.c_str());
        ic->name = vtr::strdup(name.c_str());

        ic->type = COMPLETE_INTERC;
        ic->parent_mode = mode;
        ic->parent_mode_index = mode->index;

        // ................................................
        // LUT BEL
        mode = &pb_type->modes[1];

        // Mode
        mode->name = vtr::strdup("lut");
        mode->parent_pb_type = pb_type;
        mode->index = 1;

        // Leaf pb_type
        mode->num_pb_type_children = 1;
        mode->pb_type_children = new t_pb_type[mode->num_pb_type_children];

        auto lut = &mode->pb_type_children[0];
        lut->name = vtr::strdup("lut");
        lut->num_pb = 1;
        lut->parent_mode = mode;

        lut->blif_model = vtr::strdup(MODEL_NAMES);
        lut->model = get_model(arch_, std::string(MODEL_NAMES));

        lut->num_ports = 2;
        lut->ports = (t_port*)vtr::calloc(lut->num_ports, sizeof(t_port));
        lut->ports[0] = get_generic_port(arch_, lut, IN_PORT, "in", MODEL_NAMES, width);
        lut->ports[1] = get_generic_port(arch_, lut, OUT_PORT, "out", MODEL_NAMES);

        lut->ports[0].equivalent = PortEquivalence::FULL;

        // Set classes
        pb_type->class_type = LUT_CLASS;
        lut->class_type = LUT_CLASS;
        lut->ports[0].port_class = vtr::strdup("lut_in");
        lut->ports[1].port_class = vtr::strdup("lut_out");

        // Mode interconnect
        mode->num_interconnect = 2;
        mode->interconnect = new t_interconnect[mode->num_interconnect];

        // Input
        ic = &mode->interconnect[0];
        ic->type = DIRECT_INTERC;
        ic->parent_mode = mode;
        ic->parent_mode_index = mode->index;

        istr = std::string(pb_type->name) + ".in";
        ostr = std::string(lut->name) + ".in";
        name = istr + "_to_" + ostr;

        ic->input_string = vtr::strdup(istr.c_str());
        ic->output_string = vtr::strdup(ostr.c_str());
        ic->name = vtr::strdup(name.c_str());

        // Output
        ic = &mode->interconnect[1];
        ic->type = DIRECT_INTERC;
        ic->parent_mode = mode;
        ic->parent_mode_index = mode->index;

        istr = std::string(lut->name) + ".out";
        ostr = std::string(pb_type->name) + ".out";
        name = istr + "_to_" + ostr;

        ic->input_string = vtr::strdup(istr.c_str());
        ic->output_string = vtr::strdup(ostr.c_str());
        ic->name = vtr::strdup(name.c_str());
    }

    /** @brief Generates the leaf pb types for the PAD type */
    void process_pad_block(t_pb_type* pad, Device::BEL::Reader& bel, Device::SiteType::Reader& site) {
        // For now, hard-code two modes for pads, so that PADs can either be IPADs or OPADs
        pad->num_modes = 2;
        pad->modes = new t_mode[2];

        // Add PAD pb_type ports
        VTR_ASSERT(bel.getPins().size() == 1);
        std::string pin = str(site.getBelPins()[bel.getPins()[0]].getName());
        std::string ipin = pin + in_suffix_;
        std::string opin = pin + out_suffix_;

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

    /** @brief Generates the leaf pb types for a generic intermediate block, with as many modes
     *         as the number of models that can be used in this complex block.
     */
    void process_generic_block(t_pb_type* pb_type, Device::BEL::Reader& bel, Device::SiteType::Reader& site) {
        std::string pb_name = std::string(pb_type->name);

        std::set<t_bel_cell_mapping> maps(bel_cell_mappings_[bel.getName()]);

        std::vector<t_bel_cell_mapping> map_to_erase;
        for (auto map : maps) {
            auto name = str(map.cell);
            bool is_compatible = map.site == site.getName();

            for (auto pin_map : map.pins) {
                if (is_compatible == false)
                    break;

                auto cell_pin = str(pin_map.first);
                auto bel_pin = str(pin_map.second);

                if (cell_pin == arch_->vcc_cell.first || cell_pin == arch_->gnd_cell.first)
                    continue;

                // Assign suffix to bel pin as it is a inout pin which was split in out and in ports
                auto pin_reader = get_bel_pin_reader(site, bel, bel_pin);
                bool is_inout = pin_reader.getDir() == INOUT;

                auto model_port = get_model_port(arch_, name, cell_pin, false);

                if (is_inout && model_port != nullptr)
                    bel_pin = model_port->dir == IN_PORT ? bel_pin + in_suffix_ : bel_pin + out_suffix_;

                is_compatible &= block_port_exists(pb_type, bel_pin);
            }

            if (!is_compatible)
                map_to_erase.push_back(map);
        }

        for (auto map : map_to_erase)
            VTR_ASSERT(maps.erase(map) == 1);

        int num_modes = maps.size();

        VTR_ASSERT(num_modes > 0);

        pb_type->num_modes = num_modes;
        pb_type->modes = new t_mode[num_modes];

        int count = 0;
        for (auto map : maps) {
            if (map.site != site.getName())
                continue;

            int idx = count++;
            t_mode* mode = &pb_type->modes[idx];
            auto name = str(map.cell);
            mode->name = vtr::strdup(name.c_str());
            mode->parent_pb_type = pb_type;
            mode->index = idx;
            mode->num_pb_type_children = 1;
            mode->pb_type_children = new t_pb_type[1];

            auto leaf = &mode->pb_type_children[0];
            std::string leaf_name = name == std::string(pb_type->name) ? name + std::string("_leaf") : name;
            leaf->name = vtr::strdup(leaf_name.c_str());
            leaf->num_pb = 1;
            leaf->parent_mode = mode;

            // Pre-count pins
            int ic_count = 0;
            for (auto pin_map : map.pins) {
                auto cell_pin = str(pin_map.first);

                if (cell_pin == arch_->vcc_cell.first || cell_pin == arch_->gnd_cell.first)
                    continue;

                ic_count++;
            }

            int num_ports = ic_count;
            leaf->num_ports = num_ports;
            leaf->ports = (t_port*)vtr::calloc(num_ports, sizeof(t_port));
            leaf->blif_model = vtr::strdup((std::string(".subckt ") + name).c_str());
            leaf->model = get_model(arch_, name);

            mode->num_interconnect = num_ports;
            mode->interconnect = new t_interconnect[num_ports];
            std::set<std::tuple<std::string, PORTS, int>> pins;
            ic_count = 0;
            for (auto pin_map : map.pins) {
                auto cell_pin = str(pin_map.first);
                auto bel_pin = str(pin_map.second);

                if (cell_pin == arch_->vcc_cell.first || cell_pin == arch_->gnd_cell.first)
                    continue;

                std::smatch regex_matches;
                std::string pin_suffix;
                const std::regex port_regex("([0-9A-Za-z-]+)\\[([0-9]+)\\]");
                if (std::regex_match(cell_pin, regex_matches, port_regex)) {
                    cell_pin = regex_matches[1].str();
                    pin_suffix = std::string("[") + regex_matches[2].str() + std::string("]");
                }

                auto model_port = get_model_port(arch_, name, cell_pin);

                auto size = model_port->size;
                auto dir = model_port->dir;

                // Assign suffix to bel pin as it is a inout pin which was split in out and in ports
                auto pin_reader = get_bel_pin_reader(site, bel, bel_pin);
                bool is_inout = pin_reader.getDir() == INOUT;

                pins.emplace(cell_pin, dir, size);

                std::string istr, ostr, ic_name;
                switch (dir) {
                    case IN_PORT:
                        bel_pin = is_inout ? bel_pin + in_suffix_ : bel_pin;
                        istr = pb_name + std::string(".") + bel_pin;
                        ostr = leaf_name + std::string(".") + cell_pin + pin_suffix;
                        break;
                    case OUT_PORT:
                        bel_pin = is_inout ? bel_pin + out_suffix_ : bel_pin;
                        istr = leaf_name + std::string(".") + cell_pin + pin_suffix;
                        ostr = pb_name + std::string(".") + bel_pin;
                        break;
                    default:
                        VTR_ASSERT(0);
                }

                ic_name = istr + std::string("_") + ostr;

                auto ic = &mode->interconnect[ic_count++];
                ic->name = vtr::strdup(ic_name.c_str());
                ic->type = DIRECT_INTERC;
                ic->parent_mode_index = idx;
                ic->parent_mode = mode;
                ic->input_string = vtr::strdup(istr.c_str());
                ic->output_string = vtr::strdup(ostr.c_str());
            }

            create_ports(leaf, pins, name);
        }
    }

    /** @brief Generates a routing block to allow for cascading routing blocks to be
     *         placed in the same complex block type.
     */
    void process_routing_block(t_pb_type* pb_type) {
        pb_type->num_modes = 1;
        pb_type->modes = new t_mode[1];

        int idx = 0;
        auto mode = &pb_type->modes[idx];

        std::string name = std::string(pb_type->name);
        mode->name = vtr::strdup(name.c_str());
        mode->parent_pb_type = pb_type;
        mode->index = idx;
        mode->num_pb_type_children = 0;

        std::string istr, ostr, ic_name;

        // The MUX interconnections can only have a single output
        VTR_ASSERT(pb_type->num_output_pins == 1);

        for (int iport = 0; iport < pb_type->num_ports; iport++) {
            const t_port port = pb_type->ports[iport];
            auto port_name = name + "." + std::string(port.name);
            switch (port.type) {
                case IN_PORT:
                    istr += istr.empty() ? port_name : " " + port_name;
                    break;
                case OUT_PORT:
                    ostr = port_name;
                    break;
                default:
                    VTR_ASSERT(0);
            }
        }

        ic_name = std::string(pb_type->name);

        mode->num_interconnect = 1;
        mode->interconnect = new t_interconnect[1];

        e_interconnect ic_type = pb_type->num_input_pins == 1 ? DIRECT_INTERC : MUX_INTERC;

        auto ic = &mode->interconnect[idx];
        ic->name = vtr::strdup(ic_name.c_str());
        ic->type = ic_type;
        ic->parent_mode_index = idx;
        ic->parent_mode = mode;
        ic->input_string = vtr::strdup(istr.c_str());
        ic->output_string = vtr::strdup(ostr.c_str());
    }

    /** @brief Processes all the ports of a given complex block.
     *         If a bel name index is specified, the bel pins are processed, otherwise the site ports
     *         are processed instead.
     */
    void process_block_ports(t_pb_type* pb_type, Device::SiteType::Reader& site, size_t bel_name = OPEN) {
        // Prepare data based on pb_type level
        std::set<std::tuple<std::string, PORTS, int>> pins;
        if (bel_name == (size_t)OPEN) {
            for (auto pin : site.getPins()) {
                auto dir = pin.getDir() == INPUT ? IN_PORT : OUT_PORT;
                pins.emplace(str(pin.getName()), dir, 1);
            }
        } else {
            auto bel = get_bel_reader(site, str(bel_name));

            for (auto bel_pin : bel.getPins()) {
                auto pin = site.getBelPins()[bel_pin];
                auto dir = pin.getDir();

                switch (dir) {
                    case INPUT:
                        pins.emplace(str(pin.getName()), IN_PORT, 1);
                        break;
                    case OUTPUT:
                        pins.emplace(str(pin.getName()), OUT_PORT, 1);
                        break;
                    case INOUT:
                        pins.emplace(str(pin.getName()) + in_suffix_, IN_PORT, 1);
                        pins.emplace(str(pin.getName()) + out_suffix_, OUT_PORT, 1);
                        break;
                    default:
                        VTR_ASSERT(0);
                }
            }
        }

        create_ports(pb_type, pins);
    }

    /** @brief Generates all the port for a complex block, given its pointer and a map of ports (key) and their direction and width */
    void create_ports(t_pb_type* pb_type, std::set<std::tuple<std::string, PORTS, int>>& pins, std::string model = "") {
        std::unordered_set<std::string> names;

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
            for (auto pin_tuple : pins) {
                std::string pin_name;
                PORTS pin_dir;
                int num_pins;
                std::tie(pin_name, pin_dir, num_pins) = pin_tuple;

                if (pin_dir != dir)
                    continue;

                bool is_input = dir == IN_PORT;
                pb_type->num_input_pins += is_input ? 1 : 0;
                pb_type->num_output_pins += is_input ? 0 : 1;

                auto port = get_generic_port(arch_, pb_type, dir, pin_name, /*string_model=*/"", num_pins);
                ports[pin_count] = port;
                port.index = pin_count++;
                port.port_index_by_type = pins_dir_count++;
                port.absolute_first_pin_index = pin_abs++;

                if (!model.empty())
                    port.model_port = get_model_port(arch_, model, pin_name);
            }
        }
    }

    /** @brief Processes and creates the interconnects corresponding to a given mode */
    void process_interconnects(t_mode* mode, Device::SiteType::Reader& site) {
        auto ics = get_interconnects(site);
        auto num_ic = ics.size();

        mode->num_interconnect = num_ic;
        mode->interconnect = new t_interconnect[num_ic];

        int curr_ic = 0;
        std::unordered_set<std::string> names;

        // Handle site wires, namely direct interconnects
        for (auto ic_pair : ics) {
            auto ic_name = ic_pair.first;
            auto ic_data = ic_pair.second;

            auto input = ic_data.input;
            auto outputs = ic_data.outputs;

            auto merge_string = [](std::string ss, std::string s) {
                return ss.empty() ? s : ss + " " + s;
            };

            std::string outputs_str = std::accumulate(outputs.begin(), outputs.end(), std::string(), merge_string);

            t_interconnect* ic = &mode->interconnect[curr_ic++];

            // No line num for interconnects, as line num is XML specific
            // TODO: probably line_num should be deprecated as it is dependent
            //       on the input architecture format.
            ic->line_num = 0;
            ic->type = DIRECT_INTERC;
            ic->parent_mode_index = mode->index;
            ic->parent_mode = mode;

            VTR_ASSERT(names.insert(ic_name).second);
            ic->name = vtr::strdup(ic_name.c_str());
            ic->input_string = vtr::strdup(input.c_str());
            ic->output_string = vtr::strdup(outputs_str.c_str());
        }

        // Checks and, in case, adds all the necessary pack patterns to the marked interconnects
        for (size_t iic = 0; iic < num_ic; iic++) {
            t_interconnect* ic = &mode->interconnect[iic];

            auto ic_data = ics.at(std::string(ic->name));

            if (ic_data.requires_pack_pattern) {
                auto backward_pps_map = propagate_pack_patterns(ic, site, BACKWARD);
                auto forward_pps_map = propagate_pack_patterns(ic, site, FORWARD);

                std::unordered_map<t_interconnect*, std::set<std::string>> pps_map;

                for (auto pp : backward_pps_map)
                    pps_map.emplace(pp.first, std::set<std::string>{});

                for (auto pp : forward_pps_map)
                    pps_map.emplace(pp.first, std::set<std::string>{});

                // Cross-product of all pack-patterns added both when exploring backwards and forward.
                // E.g.:
                //   Generated pack patterns
                //      - backward: OBUFDS, OBUF
                //      - forward: OPAD
                //  Final pack patterns:
                //      - OBUFDS_OPAD, OBUF_OPAD
                for (auto for_pp_pair : forward_pps_map)
                    for (auto back_pp_pair : backward_pps_map)
                        for (auto for_pp : for_pp_pair.second)
                            for (auto back_pp : back_pp_pair.second) {
                                std::string pp_name = for_pp + "_" + back_pp;
                                pps_map.at(for_pp_pair.first).insert(pp_name);
                                pps_map.at(back_pp_pair.first).insert(pp_name);
                            }

                for (auto pair : pps_map) {
                    t_interconnect* pp_ic = pair.first;

                    auto num_pp = pair.second.size();
                    pp_ic->num_annotations = num_pp;
                    pp_ic->annotations = new t_pin_to_pin_annotation[num_pp];

                    int idx = 0;
                    for (auto pp_name : pair.second)
                        pp_ic->annotations[idx++] = get_pack_pattern(pp_name, pp_ic->input_string, pp_ic->output_string);
                }
            }
        }
    }

    /** @brief Propagates and generates all pack_patterns required for the given ic.
     *         This is necessary to find all root blocks that generate the pack pattern.
     */
    std::unordered_map<t_interconnect*, std::set<std::string>> propagate_pack_patterns(t_interconnect* ic, Device::SiteType::Reader& site, e_pp_dir direction) {
        auto site_pins = site.getBelPins();

        std::string endpoint = direction == BACKWARD ? ic->input_string : ic->output_string;
        auto ic_endpoints = vtr::split(endpoint, " ");

        std::unordered_map<t_interconnect*, std::set<std::string>> pps_map;

        bool is_backward = direction == BACKWARD;

        for (auto ep : ic_endpoints) {
            auto parts = vtr::split(ep, ".");
            auto bel = parts[0];
            auto pin = parts[1];

            if (bel == str(site.getName()))
                return pps_map;

            // Assign mode and pb_type
            t_mode* parent_mode = ic->parent_mode;
            t_pb_type* pb_type = nullptr;

            for (int ipb = 0; ipb < parent_mode->num_pb_type_children; ipb++)
                if (std::string(parent_mode->pb_type_children[ipb].name) == bel)
                    pb_type = &parent_mode->pb_type_children[ipb];

            VTR_ASSERT(pb_type != nullptr);

            auto bel_reader = get_bel_reader(site, remove_bel_suffix(bel));

            // Passing through routing mux. Check at the muxes input pins interconnects
            if (bel_reader.getCategory() == ROUTING) {
                for (auto bel_pin : bel_reader.getPins()) {
                    auto pin_reader = site_pins[bel_pin];
                    auto pin_name = str(pin_reader.getName());

                    if (pin_reader.getDir() != (is_backward ? INPUT : OUTPUT))
                        continue;

                    for (int iic = 0; iic < parent_mode->num_interconnect; iic++) {
                        t_interconnect* other_ic = &parent_mode->interconnect[iic];

                        if (std::string(ic->name) == std::string(other_ic->name))
                            continue;

                        std::string ic_to_find = bel + "." + pin_name;

                        bool found = false;
                        for (auto out : vtr::split(is_backward ? other_ic->output_string : other_ic->input_string, " "))
                            found |= out == ic_to_find;

                        if (found) {
                            // An output interconnect to propagate was found, continue searching
                            auto res = propagate_pack_patterns(other_ic, site, direction);

                            for (auto pp_map : res)
                                pps_map.emplace(pp_map.first, pp_map.second);
                        }
                    }
                }
            } else {
                VTR_ASSERT(bel_reader.getCategory() == LOGIC);

                for (int imode = 0; imode < pb_type->num_modes; imode++) {
                    t_mode* mode = &pb_type->modes[imode];

                    for (int iic = 0; iic < mode->num_interconnect; iic++) {
                        t_interconnect* other_ic = &mode->interconnect[iic];

                        bool found = false;
                        for (auto other_ep : vtr::split(is_backward ? other_ic->output_string : other_ic->input_string, " ")) {
                            found |= other_ep == ep;
                        }

                        if (found) {
                            std::string pp_name = std::string(pb_type->name) + "." + std::string(mode->name);

                            std::set<std::string> pp{pp_name};
                            auto res = pps_map.emplace(other_ic, pp);

                            if (!res.second)
                                res.first->second.insert(pp_name);
                        }
                    }
                }
            }
        }

        return pps_map;
    }

    // Physical Tiles
    void process_tiles() {
        auto EMPTY = get_empty_physical_type();
        int index = 0;
        EMPTY.index = index;
        ptypes_.push_back(EMPTY);

        auto tileTypeList = ar_.getTileTypeList();
        auto siteTypeList = ar_.getSiteTypeList();

        for (auto tile : tileTypeList) {
            t_physical_tile_type ptype;
            auto name = str(tile.getName());

            if (name == EMPTY.name)
                continue;

            bool has_valid_sites = false;

            for (auto site_type : tile.getSiteTypes())
                has_valid_sites |= take_sites_.count(siteTypeList[site_type.getPrimaryType()].getName()) != 0;

            if (!has_valid_sites)
                continue;

            ptype.name = vtr::strdup(name.c_str());
            ptype.index = ++index;
            ptype.width = ptype.height = ptype.area = 1;
            ptype.capacity = 0;

            process_sub_tiles(ptype, tile);

            setup_pin_classes(&ptype);

            bool is_io = false;
            for (auto site : tile.getSiteTypes()) {
                auto site_type = ar_.getSiteTypeList()[site.getPrimaryType()];

                for (auto bel : site_type.getBels())
                    is_io |= is_pad(str(bel.getName()));
            }

            ptype.is_input_type = ptype.is_output_type = is_io;

            // TODO: remove the following once the RR graph generation is fully enabled from the device database
            ptype.switchblock_locations = vtr::Matrix<e_sb_type>({{1, 1}}, e_sb_type::FULL);
            ptype.switchblock_switch_overrides = vtr::Matrix<int>({{1, 1}}, DEFAULT_SWITCH);

            ptypes_.push_back(ptype);
        }
    }

    void process_sub_tiles(t_physical_tile_type& type, Device::TileType::Reader& tile) {
        // TODO: only one subtile at the moment
        auto siteTypeList = ar_.getSiteTypeList();
        for (auto site_in_tile : tile.getSiteTypes()) {
            t_sub_tile sub_tile;

            auto site = siteTypeList[site_in_tile.getPrimaryType()];

            if (take_sites_.count(site.getName()) == 0)
                continue;

            auto pins_to_wires = site_in_tile.getPrimaryPinsToTileWires();

            sub_tile.index = type.capacity;
            sub_tile.name = vtr::strdup(str(site.getName()).c_str());
            sub_tile.capacity.set(type.capacity, type.capacity);
            type.capacity++;

            int port_idx = 0;
            int abs_first_pin_idx = 0;
            int icount = 0;
            int ocount = 0;

            std::unordered_map<std::string, std::string> port_name_to_wire_name;
            int idx = 0;
            for (auto dir : {INPUT, OUTPUT}) {
                int port_idx_by_type = 0;
                for (auto pin : site.getPins()) {
                    if (pin.getDir() != dir)
                        continue;

                    t_physical_tile_port port;

                    port.name = vtr::strdup(str(pin.getName()).c_str());

                    port_name_to_wire_name[std::string(port.name)] = str(pins_to_wires[idx++]);

                    sub_tile.sub_tile_to_tile_pin_indices.push_back(type.num_pins + port_idx);
                    port.index = port_idx++;

                    port.absolute_first_pin_index = abs_first_pin_idx++;
                    port.port_index_by_type = port_idx_by_type++;

                    if (dir == INPUT) {
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
            fill_sub_tile(type, sub_tile, pins_size, icount, ocount);

            type.sub_tiles.push_back(sub_tile);
        }
    }

    /** @brief The constant block is a synthetic tile which is used to assign a virtual
     *         location in the grid to the constant signals which are than driven to
     *         all the real constant wires.
     *
     * The block's diagram can be seen below. The port names are specified in
     * the interchange device database, therefore GND and VCC are mainly
     * examples in this case.
     *
     * +---------------+
     * |               |
     * |  +-------+    |
     * |  |       |    |
     * |  |  GND  +----+--> RR Graph node
     * |  |       |    |
     * |  +-------+    |
     * |               |
     * |               |
     * |  +-------+    |
     * |  |       |    |
     * |  |  VCC  +----+--> RR Graph node
     * |  |       |    |
     * |  +-------+    |
     * |               |
     * +---------------+
     */
    void process_constant_block() {
        std::vector<std::pair<std::string, std::string>> const_cells{arch_->gnd_cell, arch_->vcc_cell};

        // Create constant complex block
        t_logical_block_type block;

        block.name = vtr::strdup(const_block_.c_str());
        block.index = ltypes_.size();

        auto pb_type = new t_pb_type;
        block.pb_type = pb_type;

        pb_type->name = vtr::strdup(const_block_.c_str());
        pb_type->num_pb = 1;

        pb_type->num_modes = 1;
        pb_type->modes = new t_mode[pb_type->num_modes];

        pb_type->num_ports = 2;
        pb_type->ports = (t_port*)vtr::calloc(pb_type->num_ports, sizeof(t_port));

        pb_type->num_output_pins = 2;
        pb_type->num_input_pins = 0;
        pb_type->num_clock_pins = 0;
        pb_type->num_pins = 2;

        auto mode = &pb_type->modes[0];
        mode->parent_pb_type = pb_type;
        mode->index = 0;
        mode->name = vtr::strdup("default");
        mode->disable_packing = false;

        mode->num_interconnect = 2;
        mode->interconnect = new t_interconnect[mode->num_interconnect];

        mode->num_pb_type_children = 2;
        mode->pb_type_children = new t_pb_type[mode->num_pb_type_children];

        int count = 0;
        for (auto const_cell : const_cells) {
            auto leaf_pb_type = &mode->pb_type_children[count];

            std::string leaf_name = const_cell.first;
            leaf_pb_type->name = vtr::strdup(leaf_name.c_str());
            leaf_pb_type->num_pb = 1;
            leaf_pb_type->parent_mode = mode;
            leaf_pb_type->blif_model = nullptr;

            leaf_pb_type->num_output_pins = 1;
            leaf_pb_type->num_input_pins = 0;
            leaf_pb_type->num_clock_pins = 0;
            leaf_pb_type->num_pins = 1;

            int num_ports = 1;
            leaf_pb_type->num_ports = num_ports;
            leaf_pb_type->ports = (t_port*)vtr::calloc(num_ports, sizeof(t_port));
            leaf_pb_type->blif_model = vtr::strdup(const_cell.first.c_str());
            leaf_pb_type->model = get_model(arch_, const_cell.first);

            leaf_pb_type->ports[0] = get_generic_port(arch_, leaf_pb_type, OUT_PORT, const_cell.second, const_cell.first);
            pb_type->ports[count] = get_generic_port(arch_, leaf_pb_type, OUT_PORT, const_cell.first + "_" + const_cell.second);

            std::string istr = leaf_name + "." + const_cell.second;
            std::string ostr = const_block_ + "." + const_cell.first + "_" + const_cell.second;
            std::string ic_name = const_cell.first;

            auto ic = &mode->interconnect[count];

            ic->name = vtr::strdup(ic_name.c_str());
            ic->type = DIRECT_INTERC;
            ic->parent_mode_index = 0;
            ic->parent_mode = mode;
            ic->input_string = vtr::strdup(istr.c_str());
            ic->output_string = vtr::strdup(ostr.c_str());

            count++;
        }

        ltypes_.push_back(block);
    }

    /** @brief Creates the models corresponding to the constant cells that are used in a given interchange device */
    void process_constant_model() {
        std::vector<std::pair<std::string, std::string>> const_cells{arch_->gnd_cell, arch_->vcc_cell};

        // Create constant models
        for (auto const_cell : const_cells) {
            t_model* model = new t_model;
            model->index = arch_->models->index + 1;

            model->never_prune = true;
            model->name = vtr::strdup(const_cell.first.c_str());

            t_model_ports* model_port = new t_model_ports;
            model_port->dir = OUT_PORT;
            model_port->name = vtr::strdup(const_cell.second.c_str());

            model_port->min_size = 1;
            model_port->size = 1;
            model_port->next = model->outputs;
            model->outputs = model_port;

            model->next = arch_->models;
            arch_->models = model;
        }
    }

    /** @brief Creates a synthetic constant tile that will be located in the external layer of the device.
     *
     *  The constant tile has two output ports, one for GND and the other for VCC. The constant tile hosts
     *  the constant pb type that is generated as well. See process_constant_model and process_constant_block.
     */
    void process_constant_tile() {
        std::vector<std::pair<std::string, std::string>> const_cells{arch_->gnd_cell, arch_->vcc_cell};
        // Create constant tile
        t_physical_tile_type constant;
        constant.name = vtr::strdup(const_block_.c_str());
        constant.index = ptypes_.size();
        constant.width = constant.height = constant.area = 1;
        constant.capacity = 1;
        constant.is_input_type = constant.is_output_type = false;

        constant.switchblock_locations = vtr::Matrix<e_sb_type>({{1, 1}}, e_sb_type::FULL);
        constant.switchblock_switch_overrides = vtr::Matrix<int>({{1, 1}}, DEFAULT_SWITCH);

        t_sub_tile sub_tile;
        sub_tile.index = 0;
        sub_tile.name = vtr::strdup(const_block_.c_str());
        int count = 0;
        for (auto const_cell : const_cells) {
            sub_tile.sub_tile_to_tile_pin_indices.push_back(count);

            t_physical_tile_port port;
            port.type = OUT_PORT;
            port.num_pins = 1;

            port.name = vtr::strdup((const_cell.first + "_" + const_cell.second).c_str());

            port.index = port.absolute_first_pin_index = port.port_index_by_type = 0;

            sub_tile.ports.push_back(port);

            count++;
        }

        fill_sub_tile(constant, sub_tile, 2, 0, 2);
        constant.sub_tiles.push_back(sub_tile);

        setup_pin_classes(&constant);

        ptypes_.push_back(constant);
    }

    // Layout Processing
    void process_layout() {
        auto tiles = ar_.getTileList();
        auto tile_types = ar_.getTileTypeList();
        auto site_types = ar_.getSiteTypeList();

        std::vector<std::string> packages;
        for (auto package : ar_.getPackages())
            packages.push_back(str(package.getName()));

        for (auto name : packages) {
            t_grid_def grid_def;
            grid_def.width = grid_def.height = 0;
            for (auto tile : tiles) {
                grid_def.width = std::max(grid_def.width, tile.getCol() + 1);
                grid_def.height = std::max(grid_def.height, tile.getRow() + 1);
            }

            grid_def.width += 2;
            grid_def.height += 2;

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
            for (auto tile : tiles) {
                auto tile_type = tile_types[tile.getType()];

                bool found = false;
                for (auto site : tile.getSites()) {
                    auto site_type_in_tile = tile_type.getSiteTypes()[site.getType()];
                    auto site_type = site_types[site_type_in_tile.getPrimaryType()];

                    found |= take_sites_.count(site_type.getName()) != 0;
                }

                if (!found)
                    continue;

                t_metadata_dict data;
                std::string tile_prefix = str(tile.getName());
                std::string tile_type_name = str(tile_type.getName());

                size_t pos = tile_prefix.find(tile_type_name);
                if (pos != std::string::npos && pos == 0)
                    tile_prefix.erase(pos, tile_type_name.length() + 1);
                t_grid_loc_def single(tile_type_name, 1);
                single.x.start_expr = std::to_string(tile.getCol() + 1);
                single.y.start_expr = std::to_string(tile.getRow() + 1);

                single.x.end_expr = single.x.start_expr + " + w - 1";
                single.y.end_expr = single.y.start_expr + " + h - 1";

                single.owned_meta = std::make_unique<t_metadata_dict>(data);
                single.meta = single.owned_meta.get();
                grid_def.loc_defs.emplace_back(std::move(single));
            }

            // The constant source tile will be placed at (0, 0)
            t_grid_loc_def constant(const_block_, 1);
            constant.x.start_expr = std::to_string(1);
            constant.y.start_expr = std::to_string(1);

            constant.x.end_expr = constant.x.start_expr + " + w - 1";
            constant.y.end_expr = constant.y.start_expr + " + h - 1";

            grid_def.loc_defs.emplace_back(std::move(constant));

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

        size_t num_switches = pip_timing_models.size() + 2;
        std::string switch_name;

        arch_->num_switches = num_switches;

        if (num_switches > 0) {
            arch_->Switches = new t_arch_switch_inf[num_switches];
        }

        float R, Cin, Cint, Cout, Tdel;
        for (size_t i = 0; i < num_switches; ++i) {
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

                switch_name = name.str() + std::to_string(i);
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

        // FIXME: have only one segment type for the time being, so that
        //        the RR graph generation is correct.
        //        This can be removed once the RR graph reader from the interchange
        //        device is ready and functional.
        size_t num_seg = 1; //wire_names.size();

        arch_->Segments.resize(num_seg);
        size_t index = 0;
        for (auto i : wire_names) {
            if (index >= num_seg) break;

            // Use default values as we will populate rr_graph with correct values
            // This segments are just declaration of future use
            arch_->Segments[index].name = str(i);
            arch_->Segments[index].length = 1;
            arch_->Segments[index].frequency = 1;
            arch_->Segments[index].Rmetal = 1e-12;
            arch_->Segments[index].Cmetal = 1e-12;
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
            segment_name_to_segment_idx[str(i)] = index;
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
