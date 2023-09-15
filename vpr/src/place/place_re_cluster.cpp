//
// Created by amin on 9/15/23.
//

#include "place_re_cluster.h"

#include "globals.h"
#include "move_utils.h"

static ClusterBlockId random_cluster();

static AtomBlockId random_atom_in_cluster(ClusterBlockId cluster_blk_id);

void PlaceReCluster::re_cluster() {
    const int num_moves = 2 << 20;

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
    }

}

static ClusterBlockId random_cluster() {

    const auto& cluster_ctx = g_vpr_ctx.clustering();

    int rand_id = vtr::irand(cluster_ctx.clb_nlist.blocks().size() - 1);

    return ClusterBlockId(rand_id);

}

static AtomBlockId random_atom_in_cluster(ClusterBlockId cluster_blk_id) {

    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const auto& cluster_atoms = g_vpr_ctx.cl_helper().atoms_lookup[cluster_blk_id];

    int rand_id = vtr::irand(cluster_atoms.size() - 1);

    auto it = cluster_atoms.begin();

    std::advance(it, rand_id);

    AtomBlockId atom_blk_id = *it;

    return atom_blk_id;

}

