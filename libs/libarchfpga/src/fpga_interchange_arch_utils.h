#ifndef FPGAINTERCHANGE_ARCH_UTILS_FILE_H
#define FPGAINTERCHANGE_ARCH_UTILS_FILE_H

#include "arch_types.h"
#include "arch_util.h"
#include "arch_error.h"
#include "vtr_error.h"
#include "vtr_util.h"

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"
#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// Necessary to reduce code verbosity when getting the pin directions
static const auto INPUT = LogicalNetlist::Netlist::Direction::INPUT;
static const auto OUTPUT = LogicalNetlist::Netlist::Direction::OUTPUT;
static const auto INOUT = LogicalNetlist::Netlist::Direction::INOUT;

struct t_bel_cell_mapping {
    size_t cell;
    size_t site;
    std::vector<std::pair<size_t, size_t>> pins;

    bool operator<(const t_bel_cell_mapping& other) const {
        return cell < other.cell || (cell == other.cell && site < other.site);
    }
};

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

float get_corner_value(DeviceResources::Device::CornerModel::Reader model, const char* speed_model, const char* value);

char* int_to_string(char* buff, int value);

void pip_types(std::set<std::tuple<int, bool>>& seen,
               DeviceResources::Device::Reader ar_);

void process_cell_bel_mappings(DeviceResources::Device::Reader ar_,
                               std::unordered_map<uint32_t, std::set<t_bel_cell_mapping>>& bel_cell_mappings_,
                               std::function<std::string(size_t)> str);

#ifdef __cplusplus
}
#endif
template<typename T>
void fill_switch(T* switch_,
                 float R,
                 float Cin,
                 float Cout,
                 float Cinternal,
                 float Tdel,
                 float mux_trans_size,
                 float buf_size,
                 char* name,
                 SwitchType type);

void fill_switch(t_arch_switch_inf* switch_,
                 float R,
                 float Cin,
                 float Cout,
                 float Cinternal,
                 float Tdel,
                 float mux_trans_size,
                 float buf_size,
                 char* name,
                 SwitchType type);

template<typename T1, typename T2>
void process_switches_array(DeviceResources::Device::Reader ar_,
                            std::set<std::tuple<int, bool>>& seen,
                            T1 switch_array,
                            std::vector<std::tuple<std::tuple<int, bool>, int>>& pips_models_);

#include "fpga_interchange_arch_utils.impl.h"
#endif
