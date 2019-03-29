// Tool to output FASM from placed and routed design via metadata tagging.
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
using namespace std;

#include "vtr_error.h"
#include "vtr_memory.h"
#include "vtr_log.h"

#include "tatum/error.hpp"

#include "vpr_error.h"
#include "vpr_api.h"
#include "vpr_signal_handler.h"

#include "globals.h"

#include "net_delay.h"
#include "RoutingDelayCalculator.h"

#include "fasm.h"

/*
 * Exit codes to signal success/failure to scripts
 * calling vpr
 */
constexpr int SUCCESS_EXIT_CODE = 0; //Everything OK
constexpr int ERROR_EXIT_CODE = 1; //Something went wrong internally
constexpr int UNIMPLEMENTABLE_EXIT_CODE = 2; //Could not implement (e.g. unroutable)
constexpr int INTERRUPTED_EXIT_CODE = 3; //VPR was interrupted by the user (e.g. SIGINT/ctr-C)

static bool write_fasm() {
  auto& atom_ctx = g_vpr_ctx.atom();

  std::string fasm_filename = atom_ctx.nlist.netlist_name() + ".fasm";
  vtr::printf("Writing Implementation FASM: %s\n", fasm_filename.c_str());
  std::ofstream fasm_os(fasm_filename);
  fasm::FasmWriterVisitor visitor(fasm_os);
  NetlistWalker nl_walker(visitor);
  nl_walker.walk();

  return true;
}

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
    t_options Options = t_options();
    t_arch Arch = t_arch();
    t_vpr_setup vpr_setup = t_vpr_setup();
    clock_t entire_flow_begin, entire_flow_end;

    entire_flow_begin = clock();

    try {
        vpr_install_signal_handler();

        /* Read options, architecture, and circuit netlist */
        vpr_init(argc, argv, &Options, &vpr_setup, &Arch);

        vpr_setup.PackerOpts.doPacking    = STAGE_LOAD;
        vpr_setup.PlacerOpts.doPlacement  = STAGE_LOAD;
        vpr_setup.RouterOpts.doRouting    = STAGE_LOAD;
        vpr_setup.AnalysisOpts.doAnalysis = STAGE_SKIP;

        bool flow_succeeded = false;
        flow_succeeded = vpr_flow(vpr_setup, Arch);

        flow_succeeded = write_fasm();
        if (!flow_succeeded) {
            return UNIMPLEMENTABLE_EXIT_CODE;
        }

        entire_flow_end = clock();

        vtr::printf_info("The entire flow of VPR took %g seconds.\n",
                (float) (entire_flow_end - entire_flow_begin) / CLOCKS_PER_SEC);

        /* free data structures */
        vpr_free_all(Arch, vpr_setup);

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
