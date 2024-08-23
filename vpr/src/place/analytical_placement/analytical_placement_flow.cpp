
#include "analytical_placement_flow.h"
#include <memory>
#include "PartialPlacement.h"
#include "ap_netlist.h"
#include "ap_utils.h"
#include "AnalyticalSolver.h"
#include "PlacementLegalizer.h"
#include "atom_netlist_fwd.h"
#include "globals.h"
#include "physical_types.h"
#include "read_atom_netlist.h"
#include "user_place_constraints.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

void run_analytical_placement_flow() {
    vtr::ScopedStartFinishTimer timer("Analytical Placement Flow");

    // The global state used/modified by this flow.
    AtomContext& mutable_atom_ctx = g_vpr_ctx.mutable_atom();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const UserPlaceConstraints& constraints = g_vpr_ctx.floorplanning().constraints;

    // Create the ap netlist from the atom netlist.
    // FIXME: Resolve name.
    APNetlist ap_netlist = read_atom_netlist(mutable_atom_ctx, constraints);
    // std::set<const t_model*> unique_t_model_pointer;
    // std::set<AtomBlockId> unique_atom_id;
    // for (APBlockId blk_id : ap_netlist.blocks()) {
    //     auto mol = ap_netlist.block_molecule(blk_id);
    //     VTR_LOG("mol: %d, %d\n", mol->num_blocks, mol->atom_block_ids.size());
    //     for (AtomBlockId atom_id : mol->atom_block_ids) {

    //         const t_model* model = mutable_atom_ctx.nlist.block_model(atom_id);            
    //         unique_t_model_pointer.insert(model);
    //         unique_atom_id.insert(atom_id);
    //         VTR_LOG("atom name: %s:", mutable_atom_ctx.nlist.block_name(atom_id).c_str());
    //         VTR_LOG("model name: %s\n", model->name);
    //         auto node = model->pb_types;
    //         while(node != nullptr) {
    //             t_pb_type*  type= (t_pb_type*) node->data_vptr;
    //             VTR_LOG("pb type name: %s\n", type->name);
    //             node = node->next; 
    //         }
    //         VTR_LOG("\n");
    //     }
    // }
    // VTR_LOG("number of blocks: %d\n", mutable_atom_ctx.nlist.blocks().size()); // 1201
    // VTR_LOG("number of unique atoms: %d\n", unique_atom_id.size()); // 1583
    // VTR_LOG("number of unique t model: %d\n", unique_t_model_pointer.size());
    // // 4
    // for (auto p : unique_t_model_pointer) {
    //     auto node = p->pb_types;
    //     VTR_LOG("%s\n", p -> name);
    //     while(node != nullptr) {
    //         t_pb_type*  type= (t_pb_type*) node->data_vptr;
    //         if(type->parent_mode == nullptr){
    //             VTR_LOG("is root\n");
    //         }
    //         else{
    //             if(type->num_modes == 0 && type->model != nullptr){
    //                 VTR_LOG("is primitive\n");
    //             } else{
    //                 VTR_LOG("is intermediate\n");
    //             }
    //         }

    //         VTR_LOG("pb type name: %s, %d\n", type->name, type->num_pb);
    //         node = node->next; 
    //     }
    // }
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

