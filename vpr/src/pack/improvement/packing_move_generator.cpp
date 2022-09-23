//
// Created by elgammal on 2022-07-28.
//

#include "packing_move_generator.h"
#include "re_cluster.h"
#include "re_cluster_util.h"
#include "pack_move_utils.h"

const int MAX_ITERATIONS = 100;

/******************* Packing move base class ************************/
/********************************************************************/
bool packingMoveGenerator::apply_move(std::vector<molMoveDescription>& new_locs, t_clustering_data& clustering_data) {
    if(new_locs.size()== 1) {
        //We need to move a molecule to an existing CLB
        return (move_mol_to_existing_cluster(new_locs[0].molecule_to_move,
                                             new_locs[1].new_clb,
                                             true,
                                             0,
                                             clustering_data));
    } else if(new_locs.size() == 2) {
        //We need to swap two molecules
        return (swap_two_molecules(new_locs[0].molecule_to_move,
                                   new_locs[1].molecule_to_move,
                                   true,
                                   0,
                                   clustering_data));
    } else {
        //We have a more complicated move (moving multiple molecules at once)
        //TODO: This case is not supported yet
        return false;
    }
}

/****************** Random packing move class *******************/
/****************************************************************/
bool randomPackingMove::propose_move(std::vector<molMoveDescription>& new_locs) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1, clb_index_2;
    t_logical_block_type_ptr block_type_1, block_type_2;
    int iteration = 0;
    bool found = false;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);
    block_type_1 = cluster_ctx.clb_nlist.block_type(clb_index_1);


    //pick the 2nd molecule randomly but make sure that its cluster is of the same type as the 1st molecule's clb
    do {
        mol_2 = pick_molecule_randomly();
        clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        block_type_2 = cluster_ctx.clb_nlist.block_type(clb_index_2);
        if(block_type_1 == block_type_2 && clb_index_1 != clb_index_2) {
            found = true;
            build_mol_move_description(new_locs, mol_1,clb_index_1,mol_2,clb_index_2);
        }
        ++iteration;
    } while( !found && iteration < MAX_ITERATIONS );

    return found;
}

bool randomPackingMove::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}

/***************** Quasi directed packing move class *******************/
/***********************************************************************/
bool quasiDirectedPackingMove::propose_move(std::vector<molMoveDescription>& new_locs) {

    t_pack_molecule *mol_1, *mol_2;
    ClusterBlockId clb_index_1;

    //pick the 1st molecule randomly
    mol_1 = pick_molecule_randomly();
    clb_index_1 = atom_to_cluster(mol_1->atom_block_ids[mol_1->root]);

    //pick the 2nd molecule from a cluster that is directly connected to mol_1 cluster
    mol_2 = nullptr;
    bool found = pick_molecule_connected(mol_1, mol_2);

    if(found) {
        ClusterBlockId clb_index_2 = atom_to_cluster(mol_2->atom_block_ids[mol_2->root]);
        build_mol_move_description(new_locs, mol_1, clb_index_1, mol_2, clb_index_2);
    }
    return found;
}

bool quasiDirectedPackingMove::evaluate_move(const std::vector<molMoveDescription>& new_locs) {
    return (evaluate_move_based_on_cutsize(new_locs));
}
