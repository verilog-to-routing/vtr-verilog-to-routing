#include "re_cluster.h"
#include "re_cluster_util.h"

bool move_atom_to_new_cluster(const AtomBlockId& atom_id,
                              const enum e_pad_loc_type& pad_loc_type,
                              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                              t_clustering_data& clustering_data,
                              bool during_packing) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& helper_ctx = g_vpr_ctx.mutable_helper();
    auto& device_ctx = g_vpr_ctx.device();

    bool is_removed, is_created;
    ClusterBlockId old_clb;
    PartitionRegion temp_cluster_pr;
    int imacro;
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
    is_removed = remove_atom_from_cluster(atom_id, lb_type_rr_graphs, old_clb, clustering_data, imacro, during_packing);
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
                                            imacro,
                                            helper_ctx.enable_pin_feasibility_filter,
                                            new_clb,
                                            &router_data,
                                            lb_type_rr_graphs,
                                            temp_cluster_pr,
                                            clustering_data,
                                            during_packing);

    //Print the move result
    if (is_created)
        VTR_LOG("Atom:%zu is moved to a new cluster\n", atom_id);
    else
        VTR_LOG("Atom:%zu move failed. Can't start a new cluster of the same type and mode\n", atom_id);

    //If the move is done after packing not during it, some fixes need to be done on the
    //clustered netlist
    if (is_created && !during_packing) {
        fix_clustered_netlist(atom_id, old_clb, new_clb);
    }

    return (is_created);
}
