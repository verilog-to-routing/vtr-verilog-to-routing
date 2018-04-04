/**
 VPR is a CAD tool used to conduct FPGA architecture exploration.  It takes, as input, a technology-mapped netlist and a description of the FPGA architecture being investigated.  
 VPR then generates a packed, placed, and routed FPGA (in .net, .place, and .route files respectively) that implements the input netlist.
 
 This file is where VPR starts execution.

 Key files in VPR:
 1.  libarchfpga/physical_types.h - Data structures that define the properties of the FPGA architecture
 2.  vpr_types.h - Very major file that defines the core data structures used in VPR.  This includes detailed architecture information, user netlist data structures, and data structures that describe the mapping between those two.
 3.  globals.h - Defines the global variables used by VPR.
 */

#include <cstdio>
#include <cstring>
#include <ctime>
using namespace std;

#include "vtr_error.h"
#include "vtr_memory.h"
#include "vtr_log.h"

#include "tatum/error.hpp"

#include "vpr_error.h"
#include "vpr_api.h"
#include "vpr_signal_handler.h"

#include "globals.h"

#include "read_arch_pass.h"
#include "read_blif_pass.h"
#include "clean_atom_netlist_pass.h"

/*
 * Exit codes to signal success/failure to scripts
 * calling vpr
 */
constexpr int SUCCESS_EXIT_CODE = 0; //Everything OK
constexpr int ERROR_EXIT_CODE = 1; //Something went wrong internally
constexpr int UNIMPLEMENTABLE_EXIT_CODE = 2; //Could not implement (e.g. unroutable)
constexpr int INTERRUPTED_EXIT_CODE = 3; //VPR was interrupted by the user (e.g. SIGINT/ctr-C)

/**
 * VPR program
 * Generate FPGA architecture given architecture description
 * Pack, place, and route circuit into FPGA architecture
 * Electrical timing analysis on results
 *
 * Overall steps
 * 1.  Initialization
 * 2.  Pack
 * 3.  Place-and-route and timing analysis
 * 4.  Clean up
 */
int main(int argc, const char **argv) {
    vtr::ScopedPrintTimer vpr_timer("VPR");

    try {
        auto options = read_options(argc, argv);

        std::vector<std::unique_ptr<Pass>> passes;

        passes.push_back(std::make_unique<ReadArchPass>(g_vpr_ctx, options.ArchFile))

        passes.push_back(std::make_unique<ReadBlifPass>(g_vpr_ctx, options.circuit_format, options.BlifFile))

        passes.push_back(std::make_unique<CleanAtomPass>(g_vpr_ctx))

        for (const auto& pass : passes) {
            pass->run_pass();
        }

    } catch (const tatum::Error& tatum_error) {
        vtr::printf_error(__FILE__, __LINE__, "STA Engine: %s\n", tatum_error.what());

        return ERROR_EXIT_CODE;

    } catch (const VprError& vpr_error) {
        vpr_print_error(vpr_error);

        if (vpr_error.type() == VPR_ERROR_INTERRUPTED) {
            return INTERRUPTED_EXIT_CODE;
        } else {
            return ERROR_EXIT_CODE;
        }

    } catch (const vtr::VtrError& vtr_error) {
        vtr::printf_error(__FILE__, __LINE__, "%s:%d %s\n", vtr_error.filename_c_str(), vtr_error.line(), vtr_error.what());

        return ERROR_EXIT_CODE;
    }

    /* Signal success to scripts */
    return SUCCESS_EXIT_CODE;
}




