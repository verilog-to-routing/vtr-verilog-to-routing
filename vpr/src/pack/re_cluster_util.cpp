#include "re_cluster_util.h"

#include "vpr_context.h"
#include "clustered_netlist_utils.h"
#include "cluster_util.h"
#include "cluster_router.h"

ClusterBlockId atom_to_cluster(const AtomBlockId& atom) {
    auto& atom_ctx = g_vpr_ctx.atom();
    return (atom_ctx.lookup.atom_clb(atom));
}

std::vector<AtomBlockId> cluster_to_atoms(const ClusterBlockId& cluster) {
    ClusterAtomsLookup cluster_lookup;
    return (cluster_lookup.atoms_in_cluster(cluster));
}

bool remove_mol_from_cluster(const t_pack_molecule* molecule, std::vector<t_lb_type_rr_node>* lb_type_rr_graphs, std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {

	//Function limitions are to remove single atom molecules only
	//TODO: remove the function limitations
	if(molecule->is_chain()) {
		VTR_LOG("re-cluster: Can not remove a molecule that is part of a chain\n");
        return false;
    }
	if(molecule->num_blocks != 1) {
        VTR_LOG("re-cluster: Can not remove a molecule that has more than 1 atom\n");
		return false;
    }

	//Determine the cluster ID
	ClusterBlockId clb_index = atom_to_cluster(molecule->atom_block_ids[0]);

	//re-build router_data structure for this cluster
	t_lb_router_data* router_data = load_router_data(lb_type_rr_graphs, clb_index);

	//remove atom from router_data
	remove_atom_from_target(router_data, molecule->atom_block_ids[0]);
	
	//check cluster legality
	bool is_cluster_legal = check_cluster_legality(0, E_DETAILED_ROUTE_FOR_EACH_ATOM, router_data);

	if(is_cluster_legal) {
		revert_place_atom_block(molecule->atom_block_ids[0], router_data, atom_molecules);
        VTR_LOG("re-cluster: Cluster is illegal after removing an atom\n");
    }


	//free router_data memory
	free_router_data(router_data);
	router_data = nullptr;

	//return true if succeeded
	return(is_cluster_legal);
}


t_lb_router_data* load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs, const ClusterBlockId& clb_index) {
	//build data structures used by intra-logic block router
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto block_type = cluster_ctx.clb_nlist.block_type(clb_index);
	t_lb_router_data* router_data = alloc_and_load_router_data(&lb_type_rr_graphs[block_type->index], block_type);

	//iterate over atoms of the current cluster and add them to router data
	for(auto atom_id : cluster_to_atoms(clb_index)) {
		add_atom_as_target(router_data, atom_id);
	}
    return(router_data);
}
