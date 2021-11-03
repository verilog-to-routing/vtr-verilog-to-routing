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

#include "arch_types.h"
#include "arch_util.h"
#include "arch_error.h"

#include "read_fpga_interchange_arch.h"
#include "read_common_func.h"

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

struct ArchReader {
  public:
    ArchReader(t_arch* arch, Device::Reader& arch_reader, const char* arch_file, std::vector<t_physical_tile_type>& phys_types, std::vector<t_logical_block_type>& logical_types)
        : arch_(arch)
        , arch_file_(arch_file)
        , ar_(arch_reader)
        , ptypes_(phys_types)
        , ltypes_(logical_types) {
    }

    void read_arch() {
        process_models();
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
