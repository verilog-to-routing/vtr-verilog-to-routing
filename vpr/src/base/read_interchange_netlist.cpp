/**
 * @file
 * @brief FPGA Interchange Netlist Loader
 *
 * This loader handles loading a post-technology mapping fully flattened (i.e not
 * hierarchical) netlist in FPGA Interchange Format file, and
 * builds a netlist data structure (AtomNetlist) from it.
 */
#include <cmath>
#include <limits>
#include <kj/std/iostream.h>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <zlib.h>
#include <iostream>

#include "LogicalNetlist.capnp.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"

#include "atom_netlist.h"

#include "vtr_assert.h"
#include "vtr_hash.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_logic.h"
#include "vtr_time.h"
#include "vtr_digest.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "read_interchange_netlist.h"
#include "arch_types.h"

struct NetlistReader {
  public:
    NetlistReader(AtomNetlist& main_netlist,
                  LogicalNetlist::Netlist::Reader& netlist_reader,
                  const std::string netlist_id,
                  const char* netlist_file,
                  const t_arch& arch)
        : main_netlist_(main_netlist)
        , nr_(netlist_reader)
        , netlist_file_(netlist_file)
        , arch_(arch) {
        // Define top module
        top_cell_instance_ = nr_.getTopInst();

        auto str_list = nr_.getStrList();
        main_netlist_ = AtomNetlist(str_list[top_cell_instance_.getName()], netlist_id);

        inpad_model_ = find_model(MODEL_INPUT);
        outpad_model_ = find_model(MODEL_OUTPUT);
        main_netlist_.set_block_types(inpad_model_, outpad_model_);

        prepare_port_net_maps();

        VTR_LOG("Reading IOs...\n");
        read_ios();
        VTR_LOG("Reading names...\n");
        read_names();
        VTR_LOG("Reading blocks...\n");
        read_blocks();
    }

  private:
    AtomNetlist& main_netlist_;
    // Netlist Reader
    LogicalNetlist::Netlist::Reader& nr_;

    const char* netlist_file_;

    const t_model* inpad_model_;
    const t_model* outpad_model_;
    const t_arch& arch_;

    LogicalNetlist::Netlist::CellInstance::Reader top_cell_instance_;

    std::unordered_map<size_t, std::unordered_map<std::pair<size_t, size_t>, std::string, vtr::hash_pair>> port_net_maps_;

    /** @brief Preprocesses the port net maps, populating the port_net_maps_ hash map to be later accessed for faster lookups */
    void prepare_port_net_maps() {
        auto inst_list = nr_.getInstList();
        auto decl_list = nr_.getCellDecls();
        auto str_list = nr_.getStrList();
        auto port_list = nr_.getPortList();
        auto top_cell = nr_.getCellList()[nr_.getTopInst().getCell()];

        for (auto net : top_cell.getNets()) {
            std::string net_name = str_list[net.getName()];

            // Rename constant nets to their correct name based on the device architecture
            // database
            for (auto port : net.getPortInsts()) {
                if (port.isExtPort())
                    continue;

                auto port_inst = port.getInst();
                auto cell = inst_list[port_inst].getCell();
                if (str_list[decl_list[cell].getName()] == arch_.gnd_cell.first)
                    net_name = arch_.gnd_net;

                if (str_list[decl_list[cell].getName()] == arch_.vcc_cell.first)
                    net_name = arch_.vcc_net;
            }

            for (auto port : net.getPortInsts()) {
                if (!port.isInst())
                    continue;

                size_t inst = port.getInst();

                size_t port_bit = get_port_bit(port);

                auto port_idx = port.getPort();
                int start, end;
                std::tie(start, end) = get_bus_range(port_list[port_idx]);

                int bus_size = std::abs(end - start);

                port_bit = start < end ? port_bit : bus_size - port_bit;

                auto pair = std::make_pair(port_idx, port_bit);
                std::unordered_map<std::pair<size_t, size_t>, std::string, vtr::hash_pair> map{{pair, net_name}};

                auto result = port_net_maps_.emplace(inst, map);
                if (!result.second)
                    result.first->second.emplace(pair, net_name);
            }
        }
    }

    void read_ios() {
        const t_model* input_model = find_model(MODEL_INPUT);
        const t_model* output_model = find_model(MODEL_OUTPUT);

        auto str_list = nr_.getStrList();

        auto top_cell_decl = nr_.getCellDecls()[top_cell_instance_.getCell()];
        for (auto top_port : top_cell_decl.getPorts()) {
            auto port = nr_.getPortList()[top_port];
            std::string name = str_list[port.getName()];
            auto dir = port.getDir();

            int start, end;
            std::tie(start, end) = get_bus_range(port);

            int bus_size = std::abs(end - start) + 1;
            int start_bit = start < end ? start : end;

            for (int bit = start_bit; bit < start_bit + bus_size; bit++) {
                auto port_name = name;
                if (bus_size > 1)
                    port_name = name + "[" + std::to_string(bit) + "]";

                AtomBlockId blk_id;
                AtomPortId port_id;
                AtomNetId net_id;

                switch (dir) {
                    case LogicalNetlist::Netlist::Direction::INPUT:
                        blk_id = main_netlist_.create_block(port_name, input_model);
                        port_id = main_netlist_.create_port(blk_id, input_model->outputs);
                        net_id = main_netlist_.create_net(port_name);
                        main_netlist_.create_pin(port_id, 0, net_id, PinType::DRIVER);
                        break;
                    case LogicalNetlist::Netlist::Direction::OUTPUT:
                        blk_id = main_netlist_.create_block(port_name, output_model);
                        port_id = main_netlist_.create_port(blk_id, output_model->inputs);
                        net_id = main_netlist_.create_net(port_name);
                        main_netlist_.create_pin(port_id, 0, net_id, PinType::SINK);
                        break;
                    default:
                        VTR_ASSERT(0);
                        break;
                }
            }
        }
    }

    void read_names() {
        const t_model* blk_model = find_model(MODEL_NAMES);

        // Set the max size of the LUT
        int lut_size = 0;
        for (auto lut : arch_.lut_cells)
            lut_size = std::max((int)lut.inputs.size(), lut_size);
        blk_model->inputs[0].size = lut_size;

        auto top_cell = nr_.getCellList()[nr_.getTopInst().getCell()];
        auto decl_list = nr_.getCellDecls();
        auto inst_list = nr_.getInstList();
        auto port_list = nr_.getPortList();
        auto str_list = nr_.getStrList();

        std::vector<std::tuple<size_t, int, std::string>> insts;
        for (auto cell_inst : top_cell.getInsts()) {
            auto cell = decl_list[inst_list[cell_inst].getCell()];

            bool is_lut;
            int width;
            std::string init_param;
            std::tie(is_lut, width, init_param) = is_lut_cell(str_list[cell.getName()]);

            if (!is_lut)
                continue;

            insts.emplace_back(cell_inst, width, init_param);
        }

        for (auto inst : insts) {
            size_t inst_idx;
            int lut_width;
            std::string init_param;
            std::tie(inst_idx, lut_width, init_param) = inst;

            std::string inst_name = str_list[inst_list[inst_idx].getName()];

            auto props = inst_list[inst_idx].getPropMap().getEntries();
            std::vector<bool> init;
            for (auto entry : props) {
                if (str_list[entry.getKey()] != init_param)
                    continue;

                // TODO: export this to a library function to have generic parameter decoding
                if (entry.which() == LogicalNetlist::Netlist::PropertyMap::Entry::TEXT_VALUE) {
                    const std::regex vhex_regex("[0-9]+'h([0-9A-Z]+)");
                    const std::regex vbit_regex("[0-9]+'b([0-9]+)");
                    const std::regex chex_regex("0x([0-9A-Za-z]+)");
                    const std::regex cbit_regex("0b([0-9]+)");
                    const std::regex bit_regex("[0-1]+");
                    std::string init_str = str_list[entry.getTextValue()];
                    std::smatch regex_matches;

                    // Fill the init vector
                    if (std::regex_match(init_str, regex_matches, vhex_regex))
                        for (const char& c : regex_matches[1].str()) {
                            int value = std::stoi(std::string(1, c), 0, 16);
                            for (int bit = 3; bit >= 0; bit--)
                                init.push_back((value >> bit) & 1);
                        }
                    else if (std::regex_match(init_str, regex_matches, chex_regex))
                        for (const char& c : regex_matches[1].str()) {
                            int value = std::stoi(std::string(1, c), 0, 16);
                            for (int bit = 3; bit >= 0; bit--)
                                init.push_back((value >> bit) & 1);
                        }
                    else if (std::regex_match(init_str, regex_matches, vbit_regex))
                        for (const char& c : regex_matches[1].str())
                            init.push_back((bool)std::stoi(std::string(1, c), 0, 2));
                    else if (std::regex_match(init_str, regex_matches, cbit_regex))
                        for (const char& c : regex_matches[1].str())
                            init.push_back((bool)std::stoi(std::string(1, c), 0, 2));
                    else if (std::regex_match(init_str, regex_matches, bit_regex))
                        for (const char& c : init_str)
                            init.push_back((bool)std::stoi(std::string(1, c), 0, 2));
                }
            }

            // Add proper LUT mapping function here based on LUT size and init value
            AtomNetlist::TruthTable truth_table;
            bool is_const = false;
            for (int bit = 0; bit < (int)init.size(); bit++) {
                bool bit_set = init[init.size() - bit - 1];

                if (bit_set == 0)
                    continue;

                is_const = bit == 0;

                truth_table.emplace_back();
                for (int row_bit = lut_width - 1; row_bit >= 0; row_bit--) {
                    bool row_bit_set = (bit >> row_bit) & 1;
                    auto log_value = row_bit_set ? vtr::LogicValue::TRUE : vtr::LogicValue::FALSE;

                    truth_table[truth_table.size() - 1].push_back(log_value);
                }
                truth_table[truth_table.size() - 1].push_back(vtr::LogicValue::TRUE);
            }

            //Figure out if the output is a constant generator
            bool output_is_const = false;
            if (truth_table.empty()) {
                //An empty truth table in BLIF corresponds to a constant-zero
                //  e.g.
                //
                //  #gnd is a constant 0 generator
                //  .names gnd
                //
                //An single entry truth table with value '0' also corresponds to a constant-zero
                //  e.g.
                //
                //  #gnd2 is a constant 0 generator
                //  .names gnd2
                //  0
                //
                output_is_const = true;
                VTR_LOG("Found constant-zero generator '%s'\n", inst_name.c_str());
            } else if (truth_table.size() == 1 && is_const) {
                //A single-entry truth table with value '1' in BLIF corresponds to a constant-one
                //  e.g.
                //
                //  #vcc is a constant 1 generator
                //  .names vcc
                //  1
                //
                output_is_const = true;
                VTR_LOG("Found constant-one generator '%s'\n", inst_name.c_str());
            }

            AtomBlockId blk_id = main_netlist_.create_block(inst_name, blk_model, truth_table);

            AtomPortId iport_id = main_netlist_.create_port(blk_id, blk_model->inputs);
            AtomPortId oport_id = main_netlist_.create_port(blk_id, blk_model->outputs);

            auto cell_lib = decl_list[inst_list[inst_idx].getCell()];
            auto port_net_map = port_net_maps_.at(inst_idx);
            int inum = 0;
            for (auto port : cell_lib.getPorts()) {
                std::pair<size_t, size_t> pair{port, 0};

                if (port_net_map.find(pair) == port_net_map.end())
                    continue;

                auto net_name = port_net_map.at(pair);
                AtomNetId net_id = main_netlist_.create_net(net_name);

                auto dir = port_list[port].getDir();
                switch (dir) {
                    case LogicalNetlist::Netlist::Direction::INPUT:
                        if (!output_is_const) main_netlist_.create_pin(iport_id, inum++, net_id, PinType::SINK);
                        break;
                    case LogicalNetlist::Netlist::Direction::OUTPUT:
                        main_netlist_.create_pin(oport_id, 0, net_id, PinType::DRIVER, output_is_const);
                        break;
                    default:
                        VTR_ASSERT(0);
                        break;
                }
            }
        }
    }

    void read_blocks() {
        auto top_cell = nr_.getCellList()[nr_.getTopInst().getCell()];
        auto decl_list = nr_.getCellDecls();
        auto inst_list = nr_.getInstList();
        auto port_list = nr_.getPortList();
        auto str_list = nr_.getStrList();

        std::vector<std::pair<size_t, size_t>> insts;
        for (auto cell_inst : top_cell.getInsts()) {
            auto cell = decl_list[inst_list[cell_inst].getCell()];

            bool is_lut;
            std::tie(is_lut, std::ignore, std::ignore) = is_lut_cell(str_list[cell.getName()]);

            if (!is_lut)
                insts.emplace_back(cell_inst, inst_list[cell_inst].getCell());
        }

        for (auto inst_pair : insts) {
            auto inst_idx = inst_pair.first;
            auto cell_idx = inst_pair.second;

            auto model_name = str_list[decl_list[cell_idx].getName()];
            const t_model* blk_model = find_model(model_name);

            std::string inst_name = str_list[inst_list[inst_idx].getName()];
            VTR_ASSERT(inst_name.empty() == 0);

            //The name for every block should be unique, check that there is no name conflict
            AtomBlockId blk_id = main_netlist_.find_block(inst_name);
            if (blk_id) {
                const t_model* conflicting_model = main_netlist_.block_model(blk_id);
                vpr_throw(VPR_ERROR_IC_NETLIST_F, netlist_file_, -1,
                          "Duplicate blocks named '%s' found in netlist."
                          " Existing block of type '%s' conflicts with subckt of type '%s'.",
                          inst_name.c_str(), conflicting_model->name, blk_model->name);
            }

            auto port_net_map = port_net_maps_.at(inst_idx);

            auto cell = decl_list[inst_list[inst_idx].getCell()];
            if (str_list[cell.getName()] == arch_.vcc_cell.first)
                inst_name = arch_.vcc_cell.first;
            else if (str_list[cell.getName()] == arch_.gnd_cell.first)
                inst_name = arch_.gnd_cell.first;

            if (main_netlist_.find_block(inst_name))
                continue;

            //Create the block
            blk_id = main_netlist_.create_block(inst_name, blk_model);

            std::unordered_set<AtomPortId> added_ports;
            for (auto port_net : port_net_map) {
                auto port_idx = port_net.first.first;
                auto port_bit = port_net.first.second;

                auto net_name = port_net.second;
                if (inst_name == arch_.vcc_cell.first)
                    net_name = arch_.vcc_net;
                else if (inst_name == arch_.gnd_cell.first)
                    net_name = arch_.gnd_net;

                auto port = port_list[port_idx];
                auto port_name = str_list[port.getName()];

                //Check for consistency between model and ports
                const t_model_ports* model_port = find_model_port(blk_model, port_name);
                VTR_ASSERT(model_port);

                //Determine the pin type
                PinType pin_type = PinType::SINK;
                if (model_port->dir == OUT_PORT) {
                    pin_type = PinType::DRIVER;
                } else {
                    VTR_ASSERT_MSG(model_port->dir == IN_PORT, "Unexpected port type");
                }

                AtomPortId port_id = main_netlist_.create_port(blk_id, model_port);

                //Make the net
                AtomNetId net_id = main_netlist_.create_net(std::string(net_name));

                //Make the pin
                main_netlist_.create_pin(port_id, port_bit, net_id, pin_type);

                added_ports.emplace(port_id);
            }

            // Bind unconnected ports to VCC by default
            for (const t_model_ports* ports : {blk_model->inputs, blk_model->outputs}) {
                for (const t_model_ports* port = ports; port != nullptr; port = port->next) {
                    AtomPortId port_id = main_netlist_.create_port(blk_id, port);

                    if (added_ports.count(port_id))
                        continue;

                    if (port->dir != IN_PORT)
                        continue;

                    //Make the net
                    AtomNetId net_id = main_netlist_.create_net(arch_.vcc_net);

                    PinType pin_type = PinType::SINK;
                    //Make the pin
                    for (int i = 0; i < port->size; i++)
                        main_netlist_.create_pin(port_id, i, net_id, pin_type);
                }
            }
        }
    }

    //
    // Utilities
    //
    const t_model* find_model(std::string name) {
        for (const auto models : {arch_.models, arch_.model_library})
            for (const t_model* model = models; model != nullptr; model = model->next)
                if (name == model->name)
                    return model;

        vpr_throw(VPR_ERROR_IC_NETLIST_F, netlist_file_, -1, "Failed to find matching architecture model for '%s'\n", name.c_str());
    }

    const t_model_ports* find_model_port(const t_model* blk_model, std::string name) {
        //We now look through all the ports on the model looking for the matching port
        for (const t_model_ports* ports : {blk_model->inputs, blk_model->outputs})
            for (const t_model_ports* port = ports; port != nullptr; port = port->next)
                if (name == std::string(port->name))
                    return port;

        //No match
        vpr_throw(VPR_ERROR_IC_NETLIST_F, netlist_file_, -1,
                  "Found no matching port '%s' on architecture model '%s'\n",
                  name.c_str(), blk_model->name);
        return nullptr;
    }

    std::pair<int, int> get_bus_range(LogicalNetlist::Netlist::Port::Reader port_reader) {
        if (port_reader.isBus()) {
            int s = port_reader.getBus().getBusStart();
            int e = port_reader.getBus().getBusEnd();

            return std::make_pair(s, e);
        }

        return std::make_pair(0, 0);
    }

    size_t get_port_bit(LogicalNetlist::Netlist::PortInstance::Reader port_inst_reader) {
        size_t port_bit = 0;
        if (port_inst_reader.getBusIdx().which() == LogicalNetlist::Netlist::PortInstance::BusIdx::IDX)
            port_bit = port_inst_reader.getBusIdx().getIdx();

        return port_bit;
    }

    std::tuple<bool, int, std::string> is_lut_cell(std::string cell_name) {
        for (auto lut_cell : arch_.lut_cells) {
            if (cell_name == lut_cell.name) {
                auto init_param = lut_cell.init_param;

                // Assign default value for the LUT init parameter, if inexistent
                if (init_param.empty())
                    init_param = "INIT";

                return std::make_tuple(true, lut_cell.inputs.size(), init_param);
            }
        }

        return std::make_tuple<bool, int, std::string>(false, 0, "");
    }
};

AtomNetlist read_interchange_netlist(const char* ic_netlist_file,
                                     t_arch& arch) {
    AtomNetlist netlist;
    std::string netlist_id = vtr::secure_digest_file(ic_netlist_file);

    // Decompress GZipped capnproto device file
    gzFile file = gzopen(ic_netlist_file, "r");
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

    auto netlist_reader = message_reader.getRoot<LogicalNetlist::Netlist>();

    NetlistReader reader(netlist, netlist_reader, netlist_id, ic_netlist_file, arch);

    return netlist;
}
