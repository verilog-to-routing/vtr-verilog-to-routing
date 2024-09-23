/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Implementation of the Analytical Placement flow.
 */

#include "analytical_placement_flow.h"
#include "atom_netlist.h"
#include "globals.h"
#include "prepack.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_time.h"

void run_analytical_placement_flow(t_vpr_setup& vpr_setup) {
    (void)vpr_setup;
    // Start an overall timer for the Analytical Placement flow.
    vtr::ScopedStartFinishTimer timer("Analytical Placement Flow");

    // The global state used/modified by this flow.
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Run the prepacker
    Prepacker prepacker;
    prepacker.init(atom_nlist, device_ctx.logical_block_types);

    // AP is currently under-construction. Fail gracefully just in case this
    // is somehow being called.
    VPR_FATAL_ERROR(VPR_ERROR_AP,
                    "Analytical Placement flow not implemented yet");
}

