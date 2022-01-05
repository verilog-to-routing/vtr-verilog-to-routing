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

struct ArchReader {
  public:
    ArchReader(t_arch* arch, Device::Reader& arch_reader, const char* arch_file, std::vector<t_physical_tile_type>& phys_types, std::vector<t_logical_block_type>& logical_types)
        : arch_(arch)
        , arch_file_(arch_file)
        , ar_(arch_reader)
        , ptypes_(phys_types)
        , ltypes_(logical_types) {
        set_arch_file_name(arch_file);
    }

    void read_arch() {
        process_luts();
        process_models();
        process_device();

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

    // Model processing
    void process_models() {
        // Populate the common library, namely .inputs, .outputs, .names, .latches
        CreateModelLibrary(arch_);

        t_model* temp = nullptr;
        std::map<std::string, int> model_name_map;
        std::pair<std::map<std::string, int>::iterator, bool> ret_map_name;

        int model_index = NUM_MODELS_IN_LIBRARY;
        arch_->models = nullptr;

        auto strList = ar_.getStrList();
        auto primLib = ar_.getPrimLibs();
        for (auto primitive : primLib.getCellDecls()) {
            if (std::string(strList[primitive.getLib()]) == std::string("primitives")) {
                try {
                    temp = new t_model;
                    temp->index = model_index++;

                    temp->never_prune = true;
                    temp->name = vtr::strdup(std::string(strList[primitive.getName()]).c_str());
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
        return;
    }

    void process_model_ports(t_model* model, Netlist::CellDeclaration::Reader primitive) {
        auto strList = ar_.getStrList();
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
            model_port->name = vtr::strdup(std::string(strList[port.getName()]).c_str());

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
    }

    // Layout Processing
    void process_layout() {
        auto strList = ar_.getStrList();
        auto tileList = ar_.getTileList();
        auto tileTypeList = ar_.getTileTypeList();
        t_grid_def grid_def;

        grid_def.width = grid_def.height = 0;
        for (auto tile : tileList) {
            grid_def.width = std::max(grid_def.width, tile.getCol() + 1);
            grid_def.height = std::max(grid_def.height, tile.getRow() + 1);
        }

        grid_def.grid_type = GridDefType::FIXED;
        std::string name = std::string(ar_.getName());

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
            std::string tile_prefix(strList[tile.getName()].cStr());
            auto tileType = tileTypeList[tile.getType()];
            std::string tile_type(strList[tileType.getName()].cStr());

            size_t pos = tile_prefix.find(tile_type);
            if (pos != std::string::npos && pos == 0)
                tile_prefix.erase(pos, tile_type.length() + 1);
            data.add(arch_->strings.intern_string(vtr::string_view("fasm_prefix")),
                     arch_->strings.intern_string(vtr::string_view(tile_prefix.c_str())));
            t_grid_loc_def single(tile_type, 1);
            single.x.start_expr = tile.getCol();
            single.y.start_expr = tile.getRow();
            single.x.end_expr = single.x.start_expr + " + w - 1";
            single.y.end_expr = single.y.start_expr + " + h - 1";
            single.owned_meta = std::make_unique<t_metadata_dict>(data);
            single.meta = single.owned_meta.get();
            grid_def.loc_defs.emplace_back(std::move(single));
        }

        arch_->grid_layouts.emplace_back(std::move(grid_def));
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
        auto* switches = arch_->Switches;

        if (num_switches > 0) {
            switches = new t_arch_switch_inf[num_switches];
        }

        float R, Cin, Cint, Cout, Tdel;
        for (int i = 0; i < (int)num_switches; ++i) {
            t_arch_switch_inf& as = switches[i];

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

            as.name = vtr::strdup(switch_name.c_str());
            as.set_type(type);
            as.mux_trans_size = as.type() == SwitchType::MUX ? 1 : 0;

            as.R = R;
            as.Cin = Cin;
            as.Cout = Cout;
            as.Cinternal = Cint;
            as.set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, Tdel);

            if (as.type() == SwitchType::SHORT || as.type() == SwitchType::PASS_GATE) {
                as.buf_size_type = BufferSize::ABSOLUTE;
                as.buf_size = 0;
                as.power_buffer_type = POWER_BUFFER_TYPE_ABSOLUTE_SIZE;
                as.power_buffer_size = 0.;
            } else {
                as.buf_size_type = BufferSize::AUTO;
                as.buf_size = 0.;
                as.power_buffer_type = POWER_BUFFER_TYPE_AUTO;
            }
        }
    }

    void process_segments() {
        auto strList = ar_.getStrList();

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
            arch_->Segments[index].name = std::string(strList[i]);
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
