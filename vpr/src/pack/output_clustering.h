#ifndef OUTPUT_CLUSTERING_H
#define OUTPUT_CLUSTERING_H

#include <unordered_set>
#include <string>

class AtomNetId;
class ClusterLegalizer;

void output_clustering(ClusterLegalizer* cluster_legalizer_ptr,
                       bool global_clocks,
                       const std::unordered_set<AtomNetId>& is_clock,
                       const std::string& architecture_id,
                       const char* out_fname,
                       bool skip_clustering,
                       bool from_legalizer);

void write_packing_results_to_xml(const bool& global_clocks,
                                  const std::string& architecture_id,
                                  const char* out_fname);

#endif
