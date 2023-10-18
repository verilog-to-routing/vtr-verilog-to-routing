//
// Created by amin on 9/15/23.
//

#include "place_re_cluster.h"

#include "globals.h"
#include "move_utils.h"
#include "net_cost_handler.h"

static ClusterBlockId random_cluster();

static AtomBlockId random_atom_in_cluster(ClusterBlockId cluster_blk_id);

static bool swap_atoms(const t_place_algorithm& place_algorithm,
                       const PlaceDelayModel* delay_model,
                       PlacerCriticalities* criticalities,
                       t_pl_atom_blocks_to_be_moved& blocks_affected,
                       AtomBlockId from_atom_blk_id,
                       AtomBlockId to_atom_blk_id);

void PlaceReCluster::re_cluster(const t_place_algorithm& place_algorithm,
                                const PlaceDelayModel* delay_model,
                                PlacerCriticalities* criticalities) {
    const int num_moves = 2 << 20;

    t_pl_atom_blocks_to_be_moved blocks_affected(g_vpr_ctx.atom().nlist.blocks().size());

    for (int move_num = 0; move_num < num_moves; ++move_num) {
        ClusterBlockId from_cluster_blk_id;
        AtomBlockId from_atom_blk_id;
        ClusterBlockId to_cluster_blk_id;
        AtomBlockId to_atom_blk_id;

        from_cluster_blk_id = random_cluster();
        from_atom_blk_id = random_atom_in_cluster(from_cluster_blk_id);


        while (true) {
            to_cluster_blk_id = random_cluster();
            to_atom_blk_id = random_atom_in_cluster(to_cluster_blk_id);

            if (from_cluster_blk_id != to_cluster_blk_id) {
                break;
            }
        }

        if(!swap_atoms(place_algorithm, delay_model, criticalities, blocks_affected, from_atom_blk_id, to_atom_blk_id)) {
            revert_move_blocks(blocks_affected);
        }
    }

}

static bool swap_atoms (const t_place_algorithm& place_algorithm,
                       const PlaceDelayModel* delay_model,
                       PlacerCriticalities* criticalities,
                       t_pl_atom_blocks_to_be_moved& blocks_affected,
                       AtomBlockId /* from_atom_blk_id */,
                       AtomBlockId /* to_atom_blk_id */) {

    double delta_c = 0;        //Change in cost due to this swap.
    double bb_delta_c = 0;     //Change in the bounding box (wiring) cost.
    double timing_delta_c = 0; //Change in the timing cost (delay * criticality).

//    const auto& to_atom_loc = get_atom_loc(to_atom_blk_id);

//    e_create_move create_move = ::create_move(blocks_affected, from_atom_blk_id, to_atom_loc);

//    if (!floorplan_legal(blocks_affected)) {
//        return false;
//    }

    apply_move_blocks(blocks_affected);

    int num_nets_affected = find_affected_nets_and_update_costs(
        place_algorithm, delay_model, criticalities, blocks_affected,
        bb_delta_c, timing_delta_c);

    // TODO:dummy return just to remove warnings
    return (num_nets_affected + delta_c) == 0;

}

static ClusterBlockId random_cluster() {

    const auto& cluster_ctx = g_vpr_ctx.clustering();

    int rand_id = vtr::irand(cluster_ctx.clb_nlist.blocks().size() - 1);

    return ClusterBlockId(rand_id);

}

static AtomBlockId random_atom_in_cluster(ClusterBlockId cluster_blk_id) {

//    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const auto& cluster_atoms = g_vpr_ctx.cl_helper().atoms_lookup[cluster_blk_id];

    int rand_id = vtr::irand(cluster_atoms.size() - 1);

    auto it = cluster_atoms.begin();

    std::advance(it, rand_id);

    AtomBlockId atom_blk_id = *it;

    return atom_blk_id;

}

