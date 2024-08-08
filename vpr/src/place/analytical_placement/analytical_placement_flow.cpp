
#include "analytical_placement_flow.h"
#include <memory>
#include "PartialPlacement.h"
#include "ap_netlist.h"
#include "ap_utils.h"
#include "AnalyticalSolver.h"
#include "PlacementLegalizer.h"
#include "globals.h"
#include "read_atom_netlist.h"
#include "vpr_constraints.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_time.h"

void run_analytical_placement_flow() {
    vtr::ScopedStartFinishTimer timer("Analytical Placement Flow");

    // The global state used/modified by this flow.
    AtomContext& mutable_atom_ctx = g_vpr_ctx.mutable_atom();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const VprConstraints& constraints = g_vpr_ctx.floorplanning().constraints;

    // Create the ap netlist from the atom netlist.
    // FIXME: Resolve name.
    APNetlist ap_netlist = read_atom_netlist(mutable_atom_ctx, constraints);
    print_ap_netlist_stats(ap_netlist);

    // Set up the partial placement object
    PartialPlacement p_placement(ap_netlist);
    // Create the solver
    std::unique_ptr<AnalyticalSolver> solver = make_analytical_solver(e_analytical_solver::QP_HYBRID, ap_netlist);
    // Create the legalizer
    FlowBasedLegalizer legalizer(ap_netlist);
    // This for loop always starts at iteration 0
    for (unsigned iteration = 0; iteration < 100; iteration++) {
        VTR_LOG("iteration: %ld\n", iteration);
        solver->solve(iteration, p_placement);
        VTR_ASSERT(p_placement.verify(ap_netlist, device_ctx) && "placement not valid after solve!");
        double post_solve_hpwl = get_hpwl(p_placement, ap_netlist);
        VTR_LOG("HPWL: %f\n", post_solve_hpwl);
        // Partial legalization using flow-based algorithm
        legalizer.legalize(p_placement);
        VTR_ASSERT(p_placement.verify(ap_netlist, device_ctx) && "placement not valid after legalize!");
        double post_legalize_hpwl = get_hpwl(p_placement, ap_netlist);
        VTR_LOG("Post-Legalized HPWL: %f\n", post_legalize_hpwl);
        if(std::abs(post_solve_hpwl - post_legalize_hpwl) < 20){
            VTR_LOG("ended because of convergence\n");
            break;
        }
        // p_placement.unicode_art();
    }

    // Export to a flat placement file.
    export_to_flat_placement_file(p_placement, ap_netlist, mutable_atom_ctx.nlist, "flat_placement_file.txt");

    // Run the full legalizer
    FullLegalizer(ap_netlist).legalize(p_placement);
}

