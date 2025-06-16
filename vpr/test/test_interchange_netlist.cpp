#include "catch2/catch_test_macros.hpp"

#include "globals.h"
#include "read_circuit.h"
#include "read_fpga_interchange_arch.h"
#include "vpr_types.h"
#include <cstring>
#include <vector>

namespace {

static constexpr const char kArchFile[] = "testarch.device";

TEST_CASE("read_interchange_netlist", "[vpr]") {
    t_arch arch;
    t_vpr_setup vpr_setup;

    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    vpr_setup.PackerOpts.circuit_file_name = "lut.netlist";

    /* Read blif file and sweep unused components */
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    atom_ctx.mutable_netlist() = read_and_process_circuit(e_circuit_format::FPGA_INTERCHANGE, vpr_setup, arch);
}

} // namespace
