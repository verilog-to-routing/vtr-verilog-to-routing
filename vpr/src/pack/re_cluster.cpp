#include "re_cluster.h"
#include "re_cluster_util.h"
#include "initial_placement.h"
#include "cluster_placement.h"
#include "cluster_router.h"

bool move_mol_to_new_cluster(t_pack_molecule* molecule,
                             bool during_packing,
                             int verbosity,
                             t_clustering_data& clustering_data) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& device_ctx = g_vpr_ctx.device();

    bool is_removed, is_created;
    ClusterBlockId old_clb = atom_to_cluster(molecule->atom_block_ids[molecule->root]);
    int molecule_size = get_array_size_of_molecule(molecule);

    NocGroupId temp_noc_grp_id = NocGroupId::INVALID();
    PartitionRegion temp_cluster_pr;
    t_lb_router_data* old_router_data = nullptr;
    t_lb_router_data* router_data = nullptr;

    //Check that there is a place for a new cluster of the same type
    t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(old_clb);
    int block_mode = cluster_ctx.clb_nlist.block_pb(old_clb)->mode;

    unsigned int num_instances = 0;
    for (auto equivalent_tile : block_type->equivalent_tiles) {
        num_instances += device_ctx.grid.num_instances(equivalent_tile, -1);
    }

    if (helper_ctx.num_used_type_instances[block_type] == num_instances) {
        VTR_LOGV(verbosity > 4, "The utilization of block_type %s is 100%. No space for new clusters\n", block_type->name);
        VTR_LOGV(verbosity > 4, "Atom %d move aborted\n", molecule->atom_block_ids[molecule->root]);
        return false;
    }

    //remove the molecule from its current cluster
    std::unordered_set<AtomBlockId>& old_clb_atoms = cluster_to_mutable_atoms(old_clb);
    if (old_clb_atoms.size() == 1) {
        VTR_LOGV(verbosity > 4, "Atom: %zu move failed. This is the last atom in its cluster.\n");
        return false;
    }
    remove_mol_from_cluster(molecule, molecule_size, old_clb, old_clb_atoms, false, old_router_data);

    //check old cluster legality after removing the molecule
    is_removed = is_cluster_legal(old_router_data);

    //if the cluster is legal, commit the molecule removal. Otherwise, abort the move
    if (is_removed) {
        commit_mol_removal(molecule, molecule_size, old_clb, during_packing, old_router_data, clustering_data);
    } else {
        VTR_LOGV(verbosity > 4, "Atom: %zu move failed. Can't remove it from the old cluster\n", molecule->atom_block_ids[molecule->root]);
        return false;
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
                                           verbosity,
                                           clustering_data,
                                           &router_data,
                                           temp_cluster_pr,
                                           temp_noc_grp_id);

    //Commit or revert the move
    if (is_created) {
        commit_mol_move(old_clb, new_clb, during_packing, true);
        VTR_LOGV(verbosity > 4, "Atom:%zu is moved to a new cluster\n", molecule->atom_block_ids[molecule->root]);
    } else {
        revert_mol_move(old_clb, molecule, old_router_data, during_packing, clustering_data);
        VTR_LOGV(verbosity > 4, "Atom:%zu move failed. Can't start a new cluster of the same type and mode\n", molecule->atom_block_ids[molecule->root]);
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
                                  int verbosity,
                                  t_clustering_data& clustering_data) {
    //define local variables
    bool is_removed, is_added;
    AtomBlockId root_atom_id = molecule->atom_block_ids[molecule->root];
    int molecule_size = get_array_size_of_molecule(molecule);
    t_lb_router_data* old_router_data = nullptr;
    std::unordered_set<AtomBlockId>& new_clb_atoms = cluster_to_mutable_atoms(new_clb);
    ClusterBlockId old_clb = atom_to_cluster(root_atom_id);

    //check old and new clusters compatibility
    bool is_compatible = check_type_and_mode_compatibility(old_clb, new_clb, verbosity);
    if (!is_compatible)
        return false;

    //remove the molecule from its current cluster
    std::unordered_set<AtomBlockId>& old_clb_atoms = cluster_to_mutable_atoms(old_clb);
    if (old_clb_atoms.size() == 1) {
        VTR_LOGV(verbosity > 4, "Atom: %zu move failed. This is the last atom in its cluster.\n");
        return false;
    }
    remove_mol_from_cluster(molecule, molecule_size, old_clb, old_clb_atoms, false, old_router_data);

    //check old cluster legality after removing the molecule
    is_removed = is_cluster_legal(old_router_data);

    //if the cluster is legal, commit the molecule removal. Otherwise, abort the move
    if (is_removed) {
        commit_mol_removal(molecule, molecule_size, old_clb, during_packing, old_router_data, clustering_data);
    } else {
        VTR_LOGV(verbosity > 4, "Atom: %zu move failed. Can't remove it from the old cluster\n", root_atom_id);
        return false;
    }

    //Add the atom to the new cluster
    t_lb_router_data* new_router_data = nullptr;
    is_added = pack_mol_in_existing_cluster(molecule, molecule_size, new_clb, new_clb_atoms, during_packing, clustering_data, new_router_data);

    //Commit or revert the move
    if (is_added) {
        commit_mol_move(old_clb, new_clb, during_packing, false);
        VTR_LOGV(verbosity > 4, "Atom:%zu is moved to a new cluster\n", molecule->atom_block_ids[molecule->root]);
    } else {
        revert_mol_move(old_clb, molecule, old_router_data, during_packing, clustering_data);
        VTR_LOGV(verbosity > 4, "Atom:%zu move failed. Can't start a new cluster of the same type and mode\n", molecule->atom_block_ids[molecule->root]);
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

#if 1
bool swap_two_molecules(t_pack_molecule* molecule_1,
                        t_pack_molecule* molecule_2,
                        bool during_packing,
                        int verbosity,
                        t_clustering_data& clustering_data) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    //define local variables
    PartitionRegion temp_cluster_pr_1, temp_cluster_pr_2;

    bool mol_1_success, mol_2_success;

    AtomBlockId root_1_atom_id = molecule_1->atom_block_ids[molecule_1->root];
    AtomBlockId root_2_atom_id = molecule_2->atom_block_ids[molecule_2->root];

    int molecule_1_size = get_array_size_of_molecule(molecule_1);
    int molecule_2_size = get_array_size_of_molecule(molecule_2);

    //Check that the 2 clusters are the same type
    ClusterBlockId clb_1 = atom_to_cluster(root_1_atom_id);
    ClusterBlockId clb_2 = atom_to_cluster(root_2_atom_id);

    if (clb_1 == clb_2) {
        VTR_LOGV(verbosity > 4, "Swap failed. Both atoms are already in the same cluster.\n");
        return false;
    }
    //Check that the old and new clusters are of the same type
    bool is_compitable = check_type_and_mode_compatibility(clb_1, clb_2, verbosity);
    if (!is_compitable)
        return false;

    t_lb_router_data* old_1_router_data = nullptr;
    t_lb_router_data* old_2_router_data = nullptr;

    //save the atoms of the 2 clusters
    std::unordered_set<AtomBlockId>& clb_1_atoms = cluster_to_mutable_atoms(clb_1);
    std::unordered_set<AtomBlockId>& clb_2_atoms = cluster_to_mutable_atoms(clb_2);

    if (clb_1_atoms.size() == 1 || clb_2_atoms.size() == 1) {
        VTR_LOGV(verbosity > 4, "Atom: %zu, %zu swap failed. This is the last atom in its cluster.\n",
                 molecule_1->atom_block_ids[molecule_1->root],
                 molecule_2->atom_block_ids[molecule_2->root]);
        return false;
    }

    t_pb* clb_pb_1 = cluster_ctx.clb_nlist.block_pb(clb_1);
    std::string clb_pb_1_name = static_cast<std::string>(clb_pb_1->name);
    t_pb* clb_pb_2 = cluster_ctx.clb_nlist.block_pb(clb_2);
    std::string clb_pb_2_name = static_cast<std::string>(clb_pb_2->name);

    //remove the molecule from its current cluster
    remove_mol_from_cluster(molecule_1, molecule_1_size, clb_1, clb_1_atoms, false, old_1_router_data);
    commit_mol_removal(molecule_1, molecule_1_size, clb_1, during_packing, old_1_router_data, clustering_data);

    remove_mol_from_cluster(molecule_2, molecule_2_size, clb_2, clb_2_atoms, false, old_2_router_data);
    commit_mol_removal(molecule_2, molecule_2_size, clb_2, during_packing, old_2_router_data, clustering_data);

    //Add the atom to the new cluster
    mol_1_success = pack_mol_in_existing_cluster(molecule_1, molecule_1_size, clb_2, clb_2_atoms, during_packing, clustering_data, old_2_router_data);
    if (!mol_1_success) {
        mol_1_success = pack_mol_in_existing_cluster(molecule_1, molecule_1_size, clb_1, clb_1_atoms, during_packing, clustering_data, old_1_router_data);
        mol_2_success = pack_mol_in_existing_cluster(molecule_2, molecule_2_size, clb_2, clb_2_atoms, during_packing, clustering_data, old_2_router_data);

        VTR_ASSERT(mol_1_success && mol_2_success);
        free_router_data(old_1_router_data);
        free_router_data(old_2_router_data);
        old_1_router_data = nullptr;
        old_2_router_data = nullptr;

        free(clb_pb_1->name);
        cluster_ctx.clb_nlist.block_pb(clb_1)->name = vtr::strdup(clb_pb_1_name.c_str());
        free(clb_pb_2->name);
        cluster_ctx.clb_nlist.block_pb(clb_2)->name = vtr::strdup(clb_pb_2_name.c_str());

        return false;
    }

    mol_2_success = pack_mol_in_existing_cluster(molecule_2, molecule_2_size, clb_1, clb_1_atoms, during_packing, clustering_data, old_1_router_data);
    if (!mol_2_success) {
        remove_mol_from_cluster(molecule_1, molecule_1_size, clb_2, clb_2_atoms, true, old_2_router_data);
        commit_mol_removal(molecule_1, molecule_1_size, clb_2, during_packing, old_2_router_data, clustering_data);
        mol_1_success = pack_mol_in_existing_cluster(molecule_1, molecule_1_size, clb_1, clb_1_atoms, during_packing, clustering_data, old_1_router_data);
        mol_2_success = pack_mol_in_existing_cluster(molecule_2, molecule_2_size, clb_2, clb_2_atoms, during_packing, clustering_data, old_2_router_data);

        VTR_ASSERT(mol_1_success && mol_2_success);
        free_router_data(old_1_router_data);
        free_router_data(old_2_router_data);
        old_1_router_data = nullptr;
        old_2_router_data = nullptr;

        free(clb_pb_1->name);
        cluster_ctx.clb_nlist.block_pb(clb_1)->name = vtr::strdup(clb_pb_1_name.c_str());
        free(clb_pb_2->name);
        cluster_ctx.clb_nlist.block_pb(clb_2)->name = vtr::strdup(clb_pb_2_name.c_str());

        return false;
    }

    //commit the move if succeeded or revert if failed
    VTR_ASSERT(mol_1_success && mol_2_success);

    //If the move is done after packing not during it, some fixes need to be done on the clustered netlist
    if (!during_packing) {
        fix_clustered_netlist(molecule_1, molecule_1_size, clb_1, clb_2);
        fix_clustered_netlist(molecule_2, molecule_2_size, clb_2, clb_1);
    }

    free_router_data(old_1_router_data);
    free_router_data(old_2_router_data);
    old_1_router_data = nullptr;
    old_2_router_data = nullptr;

    free(clb_pb_1->name);
    cluster_ctx.clb_nlist.block_pb(clb_1)->name = vtr::strdup(clb_pb_1_name.c_str());
    free(clb_pb_2->name);
    cluster_ctx.clb_nlist.block_pb(clb_2)->name = vtr::strdup(clb_pb_2_name.c_str());

    return true;
}
#endif
