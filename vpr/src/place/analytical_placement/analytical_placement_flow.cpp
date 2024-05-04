
#include "analytical_placement_flow.h"
#include <map>
#include <set>
#include <vector>
#include "PartialPlacement.h"
#include "atom_netlist.h"
#include "cad_types.h"
#include "clustered_netlist.h"
#include "clustered_netlist_utils.h"
#include "AnalyticalSolver.h"
#include "PlacementLegalizer.h"
#include "globals.h"
#include "prepack.h"
#include "vpr_context.h"
#include "vtr_time.h"

// Pre-pack the atoms into molecules
// This will create "forced" packs (LUT+FF) and carry chains.
// The molecules can be looked up in the atom_molecules member of the atom
// context.
// See /pack/prepack.h
// Also see /pack/pack.cpp:try_pack
static void prepack_atom_netlist() {
    VTR_LOG("Pre-Packing Atom Netlist\n");
    AtomContext& atom_mutable_ctx = g_vpr_ctx.mutable_atom();
    std::vector<t_pack_patterns> list_of_pack_patterns = alloc_and_load_pack_patterns();
    // This is a map from the AtomBlock to its expected lowest cost primitive.
    std::unordered_map<AtomBlockId, t_pb_graph_node*> expected_lowest_cost_pb_gnode;
    atom_mutable_ctx.list_of_pack_molecules.reset(alloc_and_load_pack_molecules(list_of_pack_patterns.data(), expected_lowest_cost_pb_gnode, list_of_pack_patterns.size()));
    // Not entirely sure if this free can be put here. I do not like how the
    // packer did it.
    free_list_of_pack_patterns(list_of_pack_patterns);
}

void run_analytical_placement_flow() {
    vtr::ScopedStartFinishTimer timer("Analytical Placement Flow");

    // Prepack the atom netlist into molecules.
    // This will initialize a data structure in the Atom context which will
    // dictate which atom goes into which molecule.
    // TODO: This creates internal state which is hard to understand. Consider
    //       having this method return the atom_to_molecule structure.
    //       This can then be stored in the Partial Placement class.
    prepack_atom_netlist();

    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().nlist;

    // For now we are assuming all of the IO are fixed blocks
    // Using the placement from the SA to get the fixed block locations
    std::set<AtomBlockId> fixed_blocks;
    std::map<AtomBlockId, double> fixed_blocks_x, fixed_blocks_y;
    for (const AtomNetId& net_id : atom_netlist.nets()) {
        if (PartialPlacement::net_is_ignored_for_placement(atom_netlist, net_id))
            continue;
        for (AtomPinId pin_id : atom_netlist.net_pins(net_id)) {
            AtomBlockId blk_id = atom_netlist.pin_block(pin_id);
            AtomBlockType blk_type = atom_netlist.block_type(blk_id);
            if (blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD)
                fixed_blocks.insert(blk_id);
        }
    }
    const PlacementContext& SA_placement_ctx = g_vpr_ctx.placement();
    const ClusteredNetlist& clustered_netlist = g_vpr_ctx.clustering().clb_nlist;
    ClusterAtomsLookup clusterBlockToAtomBlockLookup;
    clusterBlockToAtomBlockLookup.init_lookup();
    for (ClusterBlockId cluster_blk_id : clustered_netlist.blocks()) {
        for (AtomBlockId atom_blk_id : clusterBlockToAtomBlockLookup.atoms_in_cluster(cluster_blk_id)) {
            // Only transfer the fixed block locations
            if (fixed_blocks.find(atom_blk_id) != fixed_blocks.end()) {
                fixed_blocks_x[atom_blk_id] = SA_placement_ctx.block_locs[cluster_blk_id].loc.x;
                fixed_blocks_y[atom_blk_id] = SA_placement_ctx.block_locs[cluster_blk_id].loc.y;
            }
        }
    }

    // Set up the partial placement object
    PartialPlacement p_placement = PartialPlacement(atom_netlist, fixed_blocks, fixed_blocks_x, fixed_blocks_y);
    // Solve the QP problem
    QPHybridSolver().solve(p_placement);
    p_placement.print_stats();
    VTR_LOG("HPWL: %f\n", p_placement.get_HPWL());
    // Partial legalization using cut spreading algorithm
    FlowBasedLegalizer().legalize(p_placement);
    VTR_LOG("Post-Legalized HPWL: %f\n", p_placement.get_HPWL());
    FullLegalizer().legalize(p_placement);
}

