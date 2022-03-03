#include "re_cluster.h"
#include "re_cluster_util.h"

bool move_mol_to_new_cluster(const t_pack_molecule* molecule, std::vector<t_lb_type_rr_node>* lb_type_rr_graphs) {
	bool is_mol_moved = move_mol_to_new_cluster(molecule, lb_type_rr_graphs);

	if(!is_mol_moved) {
		VTR_LOG("Abort moving a molecule.\n");
		return(false);
	}


	return(true);
}