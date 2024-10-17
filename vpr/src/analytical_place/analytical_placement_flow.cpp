/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Implementation of the Analytical Placement flow.
 */

#include "analytical_placement_flow.h"
#include <memory>
#include "analytical_solver.h"
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

/**
 * @brief A helper method to log statistics on the APNetlist.
 */
static void print_ap_netlist_stats(const APNetlist& netlist) {
    // Get the number of moveable and fixed blocks
    size_t num_moveable_blocks = 0;
    size_t num_fixed_blocks = 0;
    for (APBlockId blk_id : netlist.blocks()) {
        if (netlist.block_mobility(blk_id) == APBlockMobility::MOVEABLE)
            num_moveable_blocks++;
        else
            num_fixed_blocks++;
    }
    // Get the fanout information of nets
    size_t highest_fanout = 0;
    float average_fanout = 0.f;
    for (APNetId net_id : netlist.nets()) {
        size_t net_fanout = netlist.net_pins(net_id).size();
        if (net_fanout > highest_fanout)
            highest_fanout = net_fanout;
        average_fanout += static_cast<float>(net_fanout);
    }
    average_fanout /= static_cast<float>(netlist.nets().size());
    // Print the statistics
    VTR_LOG("Analytical Placement Netlist Statistics:\n");
    VTR_LOG("\tBlocks: %zu\n", netlist.blocks().size());
    VTR_LOG("\t\tMoveable Blocks: %zu\n", num_moveable_blocks);
    VTR_LOG("\t\tFixed Blocks: %zu\n", num_fixed_blocks);
    VTR_LOG("\tNets: %zu\n", netlist.nets().size());
    VTR_LOG("\t\tAverage Fanout: %.2f\n", average_fanout);
    VTR_LOG("\t\tHighest Fanout: %zu\n", highest_fanout);
    VTR_LOG("\tPins: %zu\n", netlist.pins().size());
}

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
    print_ap_netlist_stats(ap_netlist);

    // Run the Global Placer
    //  For now, just runs the solver.
    PartialPlacement p_placement(ap_netlist);
    std::unique_ptr<AnalyticalSolver> solver = make_analytical_solver(e_analytical_solver::QP_HYBRID,
                                                                      ap_netlist);
    solver->solve(0, p_placement);

    // Verify that the partial placement is valid before running the full
    // legalizer.
    const size_t device_width = device_ctx.grid.width();
    const size_t device_height = device_ctx.grid.height();
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

