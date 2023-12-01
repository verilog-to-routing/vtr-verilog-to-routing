//
// Created by elgammal on 2022-07-28.
//

#include "packing_move_generator.h"
#include "re_cluster.h"
#include <string.h>
#include "re_cluster_util.h"
#include "pack_move_utils.h"
#include "packing_cost.h"

const int MAX_ITERATIONS = 10;

/******************* Packing move base class ************************/
/********************************************************************/
bool packingMoveGenerator::apply_move(std::vector<molMoveDescription>& new_locs, t_clustering_data& clustering_data, int thread_id) {
    if (new_locs.size() == 1) {
        //We need to move a molecule to an existing CLB
        return (move_mol_to_existing_cluster(new_locs[0].molecule_to_move,
                                             new_locs[1].new_clb,
                                             true,
                                             0,
                                             clustering_data,
                                             thread_id));
    } else if (new_locs.size() == 2) {
        //We need to swap two molecules
        return (swap_two_molecules(new_locs[0].molecule_to_move,
                                   new_locs[1].molecule_to_move,
                                   true,
                                   0,
                                   clustering_data,
                                   thread_id));
    } else {
        //We have a more complicated move (moving multiple molecules at once)
        //TODO: This case is not supported yet
        return false;
    }
}

/****************** Random packing move class *******************/
/****************************************************************/
bool randomPackingSwap::propose_move(std::vector<molMoveDescription>& new_locs) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1, clb_index_2;
    t_logical_block_type_ptr block_type_1, block_type_2;
    int iteration = 0;
    bool found = false;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);
    block_type_1 = cluster_ctx.clb_nlist.block_type(clb_index_1);

    do {
        mol_2 = pick_molecule_randomly();
        clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        block_type_2 = cluster_ctx.clb_nlist.block_type(clb_index_2);
        if (block_type_1 == block_type_2 && clb_index_1 != clb_index_2) {
            found = true;
            build_mol_move_description(new_locs, mol_1, clb_index_1, mol_2, clb_index_2);
        }
#ifdef PACK_MULTITHREADED
        else {
            packing_multithreading_ctx.mu[clb_index_2]->unlock();
        }
#endif
        ++iteration;
    } while (!found && iteration < MAX_ITERATIONS);

#ifdef PACK_MULTITHREADED
    if (!found) {
        packing_multithreading_ctx.mu[clb_index_1]->unlock();
    }
#endif

    return found;
}

bool randomPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}

/***************** Quasi directed packing move class *******************/
/***********************************************************************/
bool quasiDirectedPackingSwap::propose_move(std::vector<molMoveDescription>& new_locs) {
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);

    //pick the 2nd molecule from a cluster that is directly connected to mol_1 cluster
    mol_2 = nullptr;
    bool found = pick_molecule_connected(mol_1, mol_2);

    if (found) {
        ClusterBlockId clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        build_mol_move_description(new_locs, mol_1, clb_index_1, mol_2, clb_index_2);
    }
#ifdef PACK_MULTITHREADED
    else {
        packing_multithreading_ctx.mu[clb_index_1]->unlock();
    }
#endif
    return found;
}

bool quasiDirectedPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}

/***************** Quasi directed same type packing move class *******************/
/*********************************************************************************/
bool quasiDirectedSameTypePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}

bool quasiDirectedSameTypePackingSwap::propose_move(std::vector<molMoveDescription>& new_locs) {
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);

    //pick the 2nd molecule from a cluster that is directly connected to mol_1 cluster
    mol_2 = nullptr;
    bool found = pick_molecule_connected_same_type(mol_1, mol_2);

    if (found) {
        ClusterBlockId clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        build_mol_move_description(new_locs, mol_1, clb_index_1, mol_2, clb_index_2);
    }
#ifdef PACK_MULTITHREADED
    else {
        packing_multithreading_ctx.mu[clb_index_1]->unlock();
    }
#endif

    return found;
}

/***************** Quasi directed compatible type packing move class *******************/
/*********************************************************************************/
bool quasiDirectedCompatibleTypePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}

bool quasiDirectedCompatibleTypePackingSwap::propose_move(std::vector<molMoveDescription>& new_locs) {
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);

    //pick the 2nd molecule from a cluster that is directly connected to mol_1 cluster
    mol_2 = nullptr;
    bool found = pick_molecule_connected_compatible_type(mol_1, mol_2);

    if (found) {
        ClusterBlockId clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        build_mol_move_description(new_locs, mol_1, clb_index_1, mol_2, clb_index_2);
    }
#ifdef PACK_MULTITHREADED
    else {
        packing_multithreading_ctx.mu[clb_index_1]->unlock();
    }
#endif

    return found;
}

/***************** Quasi directed same size packing move class *******************/
/*********************************************************************************/
bool quasiDirectedSameSizePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}

bool quasiDirectedSameSizePackingSwap::propose_move(std::vector<molMoveDescription>& new_locs) {
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);

    //pick the 2nd molecule from a cluster that is directly connected to mol_1 cluster
    mol_2 = nullptr;
    bool found = pick_molecule_connected_same_size(mol_1, mol_2);

    if (found) {
        ClusterBlockId clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        build_mol_move_description(new_locs, mol_1, clb_index_1, mol_2, clb_index_2);
    }
#ifdef PACK_MULTITHREADED
    else {
        packing_multithreading_ctx.mu[clb_index_1]->unlock();
    }
#endif

    return found;
}

bool randomConnPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_connection(new_locs));
}

bool quasiDirectedConnPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_connection(new_locs));
}

bool quasiDirectedSameTypeConnPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_connection(new_locs));
}

bool quasiDirectedCompatibleTypeConnPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_connection(new_locs));
}

bool quasiDirectedSameSizeConnPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_connection(new_locs));
}

bool randomTerminalPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals(new_locs));
}

bool quasiDirectedTerminalPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals(new_locs));
}

bool quasiDirectedSameTypeTerminalPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals(new_locs));
}

bool quasiDirectedCompatibleTypeTerminalPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals(new_locs));
}

bool quasiDirectedSameSizeTerminalPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals(new_locs));
}

bool randomTerminalNetPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_and_nets(new_locs));
}

bool quasiDirectedTerminalNetPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_and_nets(new_locs));
}

bool quasiDirectedSameTypeTerminalNetPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_and_nets(new_locs));
}

bool quasiDirectedCompatibleTypeTerminalNetPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_and_nets(new_locs));
}

bool quasiDirectedSameSizeTerminalNetPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_and_nets(new_locs));
}

bool randomTerminalNetNewFormulaPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_new_formula(new_locs));
}

bool quasiDirectedTerminalNetNewFormulaPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_new_formula(new_locs));
}

bool quasiDirectedSameTypeTerminalNetNewFormulaPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_new_formula(new_locs));
}

bool quasiDirectedCompatibleTypeTerminalNetNewFormulaPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_new_formula(new_locs));
}

bool quasiDirectedSameSizeTerminalNetNewFormulaPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_new_formula(new_locs));
}

bool randomTerminalOutsidePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_outside(new_locs));
}

bool quasiDirectedTerminalOutsidePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_outside(new_locs));
}

bool quasiDirectedSameTypeTerminalOutsidePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_outside(new_locs));
}

bool quasiDirectedCompatibleTypeTerminalOutsidePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_outside(new_locs));
}

bool quasiDirectedSameSizeTerminalOutsidePackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_terminals_outside(new_locs));
}

bool randomCostEvaluationPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_attraction(new_locs));
}

bool quasiDirectedCostEvaluationPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_attraction(new_locs));
}

bool quasiDirectedSameTypeCostEvaluationPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_attraction(new_locs));
}

bool quasiDirectedCompatibleTypeCostEvaluationPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_attraction(new_locs));
}

bool quasiDirectedSameSizeCostEvaluationPackingSwap::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_attraction(new_locs));
}