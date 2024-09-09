#ifndef CLUSTER_H
#define CLUSTER_H

#include <unordered_set>
#include <map>
#include <vector>

#include "physical_types.h"
#include "vpr_types.h"
#include "atom_netlist_fwd.h"
#include "attraction_groups.h"
#include "cluster_util.h"

class Prepacker;

std::map<t_logical_block_type_ptr, size_t> do_clustering(const t_packer_opts& packer_opts,
                                                         const t_analysis_opts& analysis_opts,
                                                         const t_arch* arch,
                                                         Prepacker& prepacker,
                                                         const std::unordered_set<AtomNetId>& is_clock,
                                                         const std::unordered_set<AtomNetId>& is_global,
                                                         bool allow_unrelated_clustering,
                                                         bool balance_block_type_utilization,
                                                         std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                                         AttractionInfo& attraction_groups,
                                                         bool& floorplan_regions_overfull,
                                                         t_clustering_data& clustering_data);

int get_cluster_of_block(int blkidx);

void print_pb_type_count(const ClusteredNetlist& clb_nlist);
#endif
