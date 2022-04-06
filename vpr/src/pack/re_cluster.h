#ifndef RE_CLUSTER_H
#define RE_CLUSTER_H

#include "pack_types.h"
#include "clustered_netlist_utils.h"

bool move_atom_to_new_cluster(const AtomBlockId& atom_id,
							  std::vector<t_lb_type_rr_node>* lb_type_rr_graphs);
#endif