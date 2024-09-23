#ifndef CLUSTER_H
#define CLUSTER_H

#include <map>
#include <unordered_set>

#include "physical_types.h"
#include "vpr_types.h"

class AtomNetid;
class AttractionInfo;
class ClusterLegalizer;
class ClusteredNetlist;
class Prepacker;
struct t_clustering_data;

std::map<t_logical_block_type_ptr, size_t> do_clustering(const t_packer_opts& packer_opts,
                                                         const t_analysis_opts& analysis_opts,
                                                         const t_arch* arch,
                                                         Prepacker& prepacker,
                                                         ClusterLegalizer& cluster_legalizer,
                                                         const std::unordered_set<AtomNetId>& is_clock,
                                                         const std::unordered_set<AtomNetId>& is_global,
                                                         bool allow_unrelated_clustering,
                                                         bool balance_block_type_utilization,
                                                         AttractionInfo& attraction_groups,
                                                         bool& floorplan_regions_overfull,
                                                         const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                                                         t_clustering_data& clustering_data);

void print_pb_type_count(const ClusteredNetlist& clb_nlist);
#endif
