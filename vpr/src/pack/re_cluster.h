#ifndef RE_CLUSTER_H
#define RE_CLUSTER_H

#include "pack_types.h"
#include "clustered_netlist_utils.h"
#include "cluster_util.h"

bool move_atom_to_new_cluster(const AtomBlockId& atom_id,
							  const t_placer_opts& placer_opts,
							  std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
							  t_clustering_data& clustering_data,
							  bool during_packing);
#endif