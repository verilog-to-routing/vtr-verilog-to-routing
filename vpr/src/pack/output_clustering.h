#ifndef OUTPUT_CLUSTERING_H
#define OUTPUT_CLUSTERING_H
#include <vector>
#include <unordered_set>
#include "vpr_types.h"
#include "pack_types.h"

void output_clustering(const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing, bool global_clocks, const std::unordered_set<AtomNetId>& is_clock, const std::string& architecture_id, const char* out_fname, bool skip_clustering);

void write_packing_results_to_xml(const bool& global_clocks, const std::string& architecture_id, const char* out_fname);

bool check_output_clustering(const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing, const std::string& architecture_id, const char* out_fname);

void print_stats();

#endif
