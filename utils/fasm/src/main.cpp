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

#include "post_routing_pb_pin_fixup.h"
#include "sync_netlists_to_routing_flat.h"

/*
 * Exit codes to signal success/failure to scripts
 * calling vpr
 */
constexpr int SUCCESS_EXIT_CODE = 0; //Everything OK
constexpr int ERROR_EXIT_CODE = 1; //Something went wrong internally
constexpr int UNIMPLEMENTABLE_EXIT_CODE = 2; //Could not implement (e.g. unroutable)
constexpr int INTERRUPTED_EXIT_CODE = 3; //VPR was interrupted by the user (e.g. SIGINT/ctr-C)

/*
 * Writes FASM file based on the netlist name by walking the netlist.
 */
static bool write_fasm(bool is_flat) {
  auto& device_ctx = g_vpr_ctx.device();
  auto& atom_ctx = g_vpr_ctx.atom();

  std::string fasm_filename = atom_ctx.netlist().netlist_name() + ".fasm";
  vtr::printf("Writing Implementation FASM: %s\n", fasm_filename.c_str());
  std::ofstream fasm_os(fasm_filename);
  fasm::FasmWriterVisitor visitor(&device_ctx.arch->strings, fasm_os, is_flat);
  NetlistWalker nl_walker(visitor);
  nl_walker.walk();

  return true;
}

/*
 * Generate FASM utility.
 *
 * 1. Loads pack, place and route files
 * 2. Walks netlist and outputs FASM.
 * 3. Cleans up and exits.
 *
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

        vpr_setup.PackerOpts.doPacking             = STAGE_LOAD;
        vpr_setup.PlacerOpts.doPlacement           = STAGE_LOAD;
        vpr_setup.APOpts.doAP                      = STAGE_SKIP;
        vpr_setup.RouterOpts.doRouting             = STAGE_LOAD;
        vpr_setup.RouterOpts.read_rr_edge_metadata = true;
        vpr_setup.AnalysisOpts.doAnalysis          = STAGE_SKIP;

        bool flow_succeeded = false;
        flow_succeeded = vpr_flow(vpr_setup, Arch);

        /* Sync netlist to the actual routing (necessary if there are block
           ports with equivalent pins) */
        bool is_flat = vpr_setup.RouterOpts.flat_routing;
        if (flow_succeeded) {
            if(is_flat) {
                sync_netlists_to_routing_flat();
            } else {
                sync_netlists_to_routing((const Netlist<>&) g_vpr_ctx.clustering().clb_nlist,
                                         g_vpr_ctx.device(),
                                         g_vpr_ctx.mutable_atom(),
                                         g_vpr_ctx.mutable_clustering(),
                                         g_vpr_ctx.placement(),
                                         vpr_setup.PackerOpts.pack_verbosity > 2);
            }
        }

        /* Actually write output FASM file. */
        flow_succeeded = write_fasm(is_flat);
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
