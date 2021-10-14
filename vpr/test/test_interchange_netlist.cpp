#include "catch2/catch_test_macros.hpp"

#include "read_circuit.h"
#include "read_fpga_interchange_arch.h"
#include "arch_util.h"
#include "vpr_api.h"
#include <cstring>
#include <unordered_set>
#include <vector>

namespace {

static constexpr const char kArchFile[] = "testarch.device";

TEST_CASE("read_interchange_netlist", "[vpr]") {
    t_arch arch;
    t_vpr_setup vpr_setup;

    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    vpr_setup.user_models = arch.models;
    vpr_setup.library_models = arch.model_library;
    vpr_setup.PackerOpts.circuit_file_name = "lut.netlist";

    /* Read blif file and sweep unused components */
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.nlist = read_and_process_circuit(e_circuit_format::FPGA_INTERCHANGE, vpr_setup, arch);
}

} // namespace
