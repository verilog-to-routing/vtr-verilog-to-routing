#ifndef OUTPUT_CLUSTERING_H
#define OUTPUT_CLUSTERING_H

#include <unordered_set>
#include <string>

class AtomNetId;
class ClusterLegalizer;

/// @brief Output the clustering, given by the ClusterLegalizer or a clustered
///        netlist, to a clustered netlist file.
///
/// The clustering can be output from the following sources:
///     1) From the clustering
///     2) From another clustered netlist
/// If from_legalizer is true, the ClusterLegalizer will be used to generate the
/// clustered netlist. If from_legalizer is false, the clustered netlist currently
/// in the global scope will be used.
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
