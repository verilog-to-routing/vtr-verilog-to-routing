#include "re_cluster.h"
#include "re_cluster_util.h"
#include "initial_placement.h"
#include "cluster_placement.h"
#include "cluster_router.h"

bool move_mol_to_new_cluster(t_pack_molecule* molecule,
                             t_clustering_data& clustering_data,
                             bool during_packing) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& device_ctx = g_vpr_ctx.device();

    bool is_removed, is_created;
    ClusterBlockId old_clb = atom_to_cluster(molecule->atom_block_ids[molecule->root]);
    ;
    int molecule_size = get_array_size_of_molecule(molecule);

    PartitionRegion temp_cluster_pr;
    t_lb_router_data* old_router_data = nullptr;
    t_lb_router_data* router_data = nullptr;

    //Check that there is a place for a new cluster of the same type
    t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(old_clb);
    int block_mode = cluster_ctx.clb_nlist.block_pb(old_clb)->mode;

    unsigned int num_instances = 0;
    for (auto equivalent_tile : block_type->equivalent_tiles) {
        num_instances += device_ctx.grid.num_instances(equivalent_tile);
    }

    if (helper_ctx.num_used_type_instances[block_type] == num_instances) {
        VTR_LOG("The utilization of block_type %s is 100%. No space for new clusters\n", block_type->name);
        VTR_LOG("Atom %d move aborted\n", molecule->atom_block_ids[molecule->root]);
        return false;
    }

    //remove the molecule from its current cluster and check the cluster legality after
    is_removed = remove_mol_from_cluster(molecule, molecule_size, old_clb, old_router_data, clustering_data, during_packing);

    if (!is_removed) {
        VTR_LOG("Atom: %zu move failed. Can't remove it from the old cluster\n", molecule->atom_block_ids[molecule->root]);
        return (is_removed);
    }

    //Create new cluster of the same type and mode.
    ClusterBlockId new_clb(helper_ctx.total_clb_num);
    is_created = start_new_cluster_for_mol(molecule,
                                           block_type,
                                           block_mode,
                                           helper_ctx.feasible_block_array_size,
                                           helper_ctx.enable_pin_feasibility_filter,
                                           new_clb,
                                           during_packing,
                                           clustering_data,
                                           &router_data,
                                           temp_cluster_pr);

    //Commit or revert the move
    if (is_created) {
        commit_mol_move(old_clb, new_clb, during_packing, true);
        VTR_LOG("Atom:%zu is moved to a new cluster\n", molecule->atom_block_ids[molecule->root]);
    } else {
        revert_mol_move(old_clb, molecule, old_router_data, during_packing, clustering_data);
        VTR_LOG("Atom:%zu move failed. Can't start a new cluster of the same type and mode\n", molecule->atom_block_ids[molecule->root]);
    }

    free_router_data(old_router_data);
    old_router_data = nullptr;

    //If the move is done after packing not during it, some fixes need to be done on the
    //clustered netlist
    if (is_created && !during_packing) {
        fix_clustered_netlist(molecule, molecule_size, old_clb, new_clb);
    }

    return (is_created);
}

bool move_mol_to_existing_cluster(t_pack_molecule* molecule,
                                  const ClusterBlockId& new_clb,
                                  bool during_packing,
                                  t_clustering_data& clustering_data) {
    //define required contexts
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //define local variables
    bool is_removed, is_added;
    AtomBlockId root_atom_id = molecule->atom_block_ids[molecule->root];
    int molecule_size = get_array_size_of_molecule(molecule);
    t_lb_router_data* old_router_data = nullptr;
    std::vector<AtomBlockId> new_clb_atoms = cluster_to_atoms(new_clb);

    //Check that the old and new clusters are the same type
    ClusterBlockId old_clb = atom_to_cluster(root_atom_id);
    if (cluster_ctx.clb_nlist.block_type(old_clb) != cluster_ctx.clb_nlist.block_type(new_clb)) {
        VTR_LOG("Atom:%zu move aborted. New and old cluster blocks are not of the same type",
                root_atom_id);
        return false;
    }

    //Check that the old and new clusters are the mode
    if (cluster_ctx.clb_nlist.block_pb(old_clb)->mode != cluster_ctx.clb_nlist.block_pb(new_clb)->mode) {
        VTR_LOG("Atom:%zu move aborted. New and old cluster blocks are not of the same mode",
                root_atom_id);
        return false;
    }

    //remove the molecule from its current cluster and check the cluster legality
    is_removed = remove_mol_from_cluster(molecule, molecule_size, old_clb, old_router_data, clustering_data, during_packing);
    if (!is_removed) {
        VTR_LOG("Atom: %zu move failed. Can't remove it from the old cluster\n", root_atom_id);
        return false;
    }

    rebuild_cluster_placemet_stats(new_clb, new_clb_atoms, cluster_ctx.clb_nlist.block_type(new_clb)->index, cluster_ctx.clb_nlist.block_pb(new_clb)->mode);
    //Add the atom to the new cluster
    is_added = pack_mol_in_existing_cluster(molecule, new_clb, new_clb_atoms, during_packing, clustering_data);

    //Commit or revert the move
    if (is_added) {
        commit_mol_move(old_clb, new_clb, during_packing, false);
        VTR_LOG("Atom:%zu is moved to a new cluster\n", molecule->atom_block_ids[molecule->root]);
    } else {
        revert_mol_move(old_clb, molecule, old_router_data, during_packing, clustering_data);
        VTR_LOG("Atom:%zu move failed. Can't start a new cluster of the same type and mode\n", molecule->atom_block_ids[molecule->root]);
    }

    free_router_data(old_router_data);
    old_router_data = nullptr;

    //If the move is done after packing not during it, some fixes need to be done on the
    //clustered netlist
    if (is_added && !during_packing) {
        fix_clustered_netlist(molecule, molecule_size, old_clb, new_clb);
    }

    return (is_added);
}

#if 0
bool swap_two_molecules(t_pack_molecule* molecule_1,
                        t_pack_molecule* molecule_2,
                        bool during_packing,
                        t_clustering_data& clustering_data) {
    //define required contexts
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();

    //define local variables
    PartitionRegion temp_cluster_pr_1, temp_cluster_pr_2;

    e_block_pack_status pack_1_result = BLK_STATUS_UNDEFINED;
    e_block_pack_status pack_2_result = BLK_STATUS_UNDEFINED;

    AtomBlockId root_1_atom_id = molecule_1->atom_block_ids[molecule_1->root];
    AtomBlockId root_2_atom_id = molecule_2->atom_block_ids[molecule_2->root];

    int molecule_1_size = get_array_size_of_molecule(molecule_1);
    int molecule_2_size = get_array_size_of_molecule(molecule_2);

    //Check that the 2 clusters are the same type
    ClusterBlockId clb_1 = atom_to_cluster(root_1_atom_id);
    ClusterBlockId clb_2 = atom_to_cluster(root_2_atom_id);

    t_logical_block_type_ptr block_1_type = cluster_ctx.clb_nlist.block_type(clb_1);
    t_logical_block_type_ptr block_2_type = cluster_ctx.clb_nlist.block_type(clb_2);

    t_ext_pin_util target_ext_pin_util = helper_ctx.target_external_pin_util.get_pin_util(cluster_ctx.clb_nlist.block_type(clb_1)->name);

    //Check that the old and new clusters are of the same type
    if (cluster_ctx.clb_nlist.block_type(clb_1) != cluster_ctx.clb_nlist.block_type(clb_2)) {
        VTR_LOG("swapping atoms:(%zu, %zu) is aborted. The 2 cluster blocks are not of the same type",
                root_1_atom_id, root_2_atom_id);
        return false;
    }

    //Check that the old and new clusters are of the same mode
    if (cluster_ctx.clb_nlist.block_pb(clb_1)->mode != cluster_ctx.clb_nlist.block_pb(clb_2)->mode) {
        VTR_LOG("swapping atoms:(%zu, %zu) is aborted. The 2 cluster blocks are not of the same mode",
                root_1_atom_id, root_2_atom_id);
        return false;
    }

    //Backup the original pb of the molecule before the move
    std::vector<t_pb*> mol_1_pb_backup, mol_2_pb_backup;
    for (int i_atom = 0; i_atom < molecule_1_size; i_atom++) {
        if (molecule_1->atom_block_ids[i_atom]) {
            t_pb* atom_pb_backup = new t_pb;
            atom_pb_backup->pb_deep_copy(atom_ctx.lookup.atom_pb(molecule_1->atom_block_ids[i_atom]));
            mol_1_pb_backup.push_back(atom_pb_backup);
        }
    }
    for (int i_atom = 0; i_atom < molecule_2_size; i_atom++) {
        if (molecule_2->atom_block_ids[i_atom]) {
            t_pb* atom_pb_backup = new t_pb;
            atom_pb_backup->pb_deep_copy(atom_ctx.lookup.atom_pb(molecule_2->atom_block_ids[i_atom]));
            mol_2_pb_backup.push_back(atom_pb_backup);
        }
    }

    //save the atoms of the 2 clusters
    std::vector<AtomBlockId> clb_1_atoms = cluster_to_atoms(clb_1);
    std::vector<AtomBlockId> clb_2_atoms = cluster_to_atoms(clb_2);

    //re-load the intra-cluster routing
    t_lb_router_data* old_1_router_data = lb_load_router_data(helper_ctx.lb_type_rr_graphs, clb_1, clb_1_atoms);
    t_lb_router_data* old_2_router_data = lb_load_router_data(helper_ctx.lb_type_rr_graphs, clb_2, clb_2_atoms);

    //remove molecules from their old clusters
    for(int i_atom = 0; i_atom < molecule_1_size; i_atom++) {
        if(molecule_1->atom_block_ids[i_atom]) {
            remove_atom_from_target(old_1_router_data, molecule_1->atom_block_ids[i_atom]);
            revert_place_atom_block(molecule_1->atom_block_ids[i_atom], old_1_router_data);
        }
    }
    for(int i_atom = 0; i_atom < molecule_2_size; i_atom++) {
        if(molecule_2->atom_block_ids[i_atom]) {
            remove_atom_from_target(old_2_router_data, molecule_2->atom_block_ids[i_atom]);
            revert_place_atom_block(molecule_2->atom_block_ids[i_atom], old_2_router_data);
        }
    }

    //add the molecules to their new clusters
    pack_2_result = try_pack_molecule(&(helper_ctx.cluster_placement_stats[block_1_type->index]),
                                        molecule_2,
                                        helper_ctx.primitives_list,
                                        cluster_ctx.clb_nlist.block_pb(clb_1),
                                        helper_ctx.num_models,
                                        helper_ctx.max_cluster_size,
                                        clb_1,
                                        E_DETAILED_ROUTE_FOR_EACH_ATOM,
                                        old_1_router_data,
                                        0,
                                        helper_ctx.enable_pin_feasibility_filter,
                                        helper_ctx.feasible_block_array_size,
                                        target_ext_pin_util,
                                        temp_cluster_pr_1);

    pack_1_result = try_pack_molecule(&(helper_ctx.cluster_placement_stats[block_2_type->index]),
                                        molecule_1,
                                        helper_ctx.primitives_list,
                                        cluster_ctx.clb_nlist.block_pb(clb_2),
                                        helper_ctx.num_models,
                                        helper_ctx.max_cluster_size,
                                        clb_2,
                                        E_DETAILED_ROUTE_FOR_EACH_ATOM,
                                        old_2_router_data,
                                        0,
                                        helper_ctx.enable_pin_feasibility_filter,
                                        helper_ctx.feasible_block_array_size,
                                        target_ext_pin_util,
                                        temp_cluster_pr_2);

    

    //commit the move if succeeded or revert if failed
    if (pack_1_result == BLK_PASSED && pack_2_result == BLK_PASSED) {
        if(during_packing) {
            free_intra_lb_nets(clustering_data.intra_lb_routing[clb_1]);
            free_intra_lb_nets(clustering_data.intra_lb_routing[clb_2]);

            clustering_data.intra_lb_routing[clb_1] = old_1_router_data->saved_lb_nets;
            clustering_data.intra_lb_routing[clb_2] = old_2_router_data->saved_lb_nets;
        } else {
            cluster_ctx.clb_nlist.block_pb(clb_1)->pb_route.clear();
            cluster_ctx.clb_nlist.block_pb(clb_1)->pb_route = alloc_and_load_pb_route(old_1_router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(clb_1)->pb_graph_node);
            cluster_ctx.clb_nlist.block_pb(clb_2)->pb_route.clear();
            cluster_ctx.clb_nlist.block_pb(clb_2)->pb_route = alloc_and_load_pb_route(old_2_router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(clb_1)->pb_graph_node);

        }

        commit_mol_move(clb_1, clb_2, during_packing, false);
        commit_mol_move(clb_2, clb_1, during_packing, false);
        
    } else {
        int atom_idx = 0;
        for(int i_atom = 0; i_atom < molecule_1_size; i_atom++) {
            if(molecule_1->atom_block_ids[i_atom]) {
                atom_ctx.lookup.set_atom_clb(molecule_1->atom_block_ids[i_atom], clb_1);
                atom_ctx.lookup.set_atom_pb(molecule_1->atom_block_ids[i_atom], mol_1_pb_backup[atom_idx]);
            }
        }

        atom_idx = 0;
        for(int i_atom = 0; i_atom < molecule_2_size; i_atom++) {
            if(molecule_2->atom_block_ids[i_atom]) {
                atom_ctx.lookup.set_atom_clb(molecule_2->atom_block_ids[i_atom], clb_2);
                atom_ctx.lookup.set_atom_pb(molecule_2->atom_block_ids[i_atom], mol_2_pb_backup[atom_idx]);
            }
        }
    }

    //If the move is done after packing not during it, some fixes need to be done on the clustered netlist
    if(pack_1_result == BLK_PASSED && pack_2_result == BLK_PASSED && !during_packing) {
        fix_clustered_netlist(molecule_1, molecule_1_size, clb_1, clb_2);
        fix_clustered_netlist(molecule_2, molecule_2_size, clb_2, clb_1);
    }

    //free memory
    old_1_router_data->saved_lb_nets = nullptr;
    old_2_router_data->saved_lb_nets = nullptr;

    free_router_data(old_1_router_data);
    free_router_data(old_2_router_data);

    old_1_router_data = nullptr;
    old_2_router_data = nullptr;

    //free backup memory
    for(auto atom_pb_backup : mol_1_pb_backup) {
        cleanup_pb(atom_pb_backup);
        delete atom_pb_backup;
    }
    for(auto atom_pb_backup : mol_2_pb_backup) {
        cleanup_pb(atom_pb_backup);
        delete atom_pb_backup;
    }

    //return the move result
    return (pack_1_result == BLK_PASSED && pack_2_result == BLK_PASSED);
}
#endif