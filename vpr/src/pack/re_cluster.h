#ifndef RE_CLUSTER_H
#define RE_CLUSTER_H

#include "pack_types.h"

bool move_mol_to_new_cluster(const t_pack_molecule* molecule, 
							 std::vector<t_lb_type_rr_node>* lb_type_rr_graphs);

#endif