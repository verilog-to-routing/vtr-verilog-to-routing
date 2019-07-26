#ifndef CLUSTER_H
#define CLUSTER_H
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>

#include "physical_types.h"
#include "vpr_types.h"
#include "atom_netlist_fwd.h"

std::map<t_type_ptr, size_t> do_clustering(const t_packer_opts& packer_opts,
                                           const t_analysis_opts& analysis_opts,
                                           const t_arch* arch,
                                           t_pack_molecule* molecule_head,
                                           int num_models,
                                           const std::unordered_set<AtomNetId>& is_clock,
                                           std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                           const std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
                                           bool allow_unrelated_clustering,
                                           bool balance_block_type_utilization,
                                           std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                           const t_ext_pin_util_targets& ext_pin_util_targets,
                                           const t_pack_high_fanout_thresholds& high_fanout_thresholds);

int get_cluster_of_block(int blkidx);
#endif
