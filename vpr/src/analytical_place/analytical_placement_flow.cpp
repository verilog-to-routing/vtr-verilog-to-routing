/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Implementation of the Analytical Placement flow.
 */

#include "analytical_placement_flow.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "full_legalizer.h"
#include "gen_ap_netlist_from_atoms.h"
#include "globals.h"
#include "partial_placement.h"
#include "prepack.h"
#include "user_place_constraints.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_time.h"

void run_analytical_placement_flow(t_vpr_setup& vpr_setup) {
    (void)vpr_setup;
    // Start an overall timer for the Analytical Placement flow.
    vtr::ScopedStartFinishTimer timer("Analytical Placement");

    // The global state used/modified by this flow.
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const UserPlaceConstraints& constraints = g_vpr_ctx.floorplanning().constraints;

    // Run the prepacker
    Prepacker prepacker;
    prepacker.init(atom_nlist, device_ctx.logical_block_types);

    // Create the ap netlist from the atom netlist using the result from the
    // prepacker.
    APNetlist ap_netlist = gen_ap_netlist_from_atoms(atom_nlist,
                                                     prepacker,
                                                     constraints);

    // Run the Global Placer
    //  For now, just put all the moveable blocks at the center of the device
    //  grid. This will be replaced later. This is just for testing.
    PartialPlacement p_placement(ap_netlist);
    const size_t device_width = device_ctx.grid.width();
    const size_t device_height = device_ctx.grid.height();
    double device_center_x = static_cast<double>(device_width) / 2.0;
    double device_center_y = static_cast<double>(device_height) / 2.0;
    for (APBlockId ap_blk_id : ap_netlist.blocks()) {
        if (ap_netlist.block_mobility(ap_blk_id) != APBlockMobility::MOVEABLE)
            continue;
        // If the APBlock is moveable, put it on the center for the device.
        p_placement.block_x_locs[ap_blk_id] = device_center_x;
        p_placement.block_y_locs[ap_blk_id] = device_center_y;
    }
    VTR_ASSERT(p_placement.verify(ap_netlist,
                                  device_width,
                                  device_height,
                                  device_ctx.grid.get_num_layers()));

    // Run the Full Legalizer.
    FullLegalizer full_legalizer(ap_netlist,
                                 vpr_setup,
                                 device_ctx.grid,
                                 device_ctx.arch,
                                 atom_nlist,
                                 prepacker,
                                 device_ctx.logical_block_types,
                                 vpr_setup.PackerRRGraph,
                                 device_ctx.arch->models,
                                 device_ctx.arch->model_library,
                                 vpr_setup.PackerOpts);
    full_legalizer.legalize(p_placement);
}

