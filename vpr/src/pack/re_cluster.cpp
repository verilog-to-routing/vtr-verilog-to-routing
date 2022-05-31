#include "re_cluster.h"
#include "re_cluster_util.h"
#include "initial_placement.h"

bool move_atom_to_new_cluster(const AtomBlockId& atom_id,
                              const enum e_pad_loc_type& pad_loc_type,
                              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                              t_clustering_data& clustering_data,
                              bool during_packing) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& helper_ctx = g_vpr_ctx.mutable_helper();
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    bool is_removed, is_created;
    ClusterBlockId old_clb;

    //Backup the original pb of the atom before the move
    t_pb* atom_pb_packup = new t_pb;
    atom_pb_packup->pb_deep_copy(atom_ctx.lookup.atom_pb(atom_id));

    PartitionRegion temp_cluster_pr;
    t_lb_router_data* old_router_data = nullptr;
    t_lb_router_data* router_data = nullptr;

    //Check that there is a place for a new cluster of the same type
    old_clb = atom_to_cluster(atom_id);
    t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(old_clb);
    int block_mode = cluster_ctx.clb_nlist.block_pb(old_clb)->mode;

    unsigned int num_instances = 0;
    for (auto equivalent_tile : block_type->equivalent_tiles) {
        num_instances += device_ctx.grid.num_instances(equivalent_tile);
    }

    if (helper_ctx.num_used_type_instances[block_type] == num_instances) {
        VTR_LOG("The utilization of block_type %s is 100%. No space for new clusters\n", block_type->name);
        VTR_LOG("Atom %d move aborted\n", atom_id);
        return false;
    }

    //remove the atom from its current cluster and check its legality
    is_removed = remove_atom_from_cluster(atom_id, lb_type_rr_graphs, old_clb, old_router_data);

    if (!is_removed) {
        VTR_LOG("Atom: %zu move failed. Can't remove it from the old cluster\n", atom_id);
        return (is_removed);
    }

    //Create new cluster of the same type and mode.
    ClusterBlockId new_clb(helper_ctx.total_clb_num);
    is_created = start_new_cluster_for_atom(atom_id,
                                            pad_loc_type,
                                            block_type,
                                            block_mode,
                                            helper_ctx.feasible_block_array_size,
                                            helper_ctx.enable_pin_feasibility_filter,
                                            new_clb,
                                            &router_data,
                                            lb_type_rr_graphs,
                                            temp_cluster_pr,
                                            clustering_data,
                                            during_packing);

    //Print the move result
    if (is_created) {
        VTR_LOG("Atom:%zu is moved to a new cluster\n", atom_id);

        commit_atom_move(atom_id, old_clb, atom_pb_packup, old_router_data, clustering_data, during_packing);
        if (!during_packing) {
            int imacro;
            g_vpr_ctx.mutable_placement().block_locs.resize(g_vpr_ctx.placement().block_locs.size() + 1);
            get_imacro_from_iblk(&imacro, old_clb, g_vpr_ctx.placement().pl_macros);
            set_imacro_for_iblk(&imacro, new_clb);
            place_one_block(new_clb, pad_loc_type);
        }
    }
    else {
        atom_ctx.lookup.set_atom_clb(atom_id, old_clb);
        atom_ctx.lookup.set_atom_pb(atom_id, atom_pb_packup);
        VTR_LOG("Atom:%zu move failed. Can't start a new cluster of the same type and mode\n", atom_id);
    }

    //If the move is done after packing not during it, some fixes need to be done on the
    //clustered netlist
    if (is_created && !during_packing) {
        fix_clustered_netlist(atom_id, old_clb, new_clb);
    }

    cleanup_pb(atom_pb_packup);
    delete atom_pb_packup;
    
    return (is_created);
}
