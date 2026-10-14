#pragma once

// A special ID to identify the legalization clusters. This is separate from the
// ClusterBlockId since ClusterLegalizer does not represent the final clustered
// netlist, but provides routines and structures that assist in its construction.
typedef vtr::StrongId<struct legalization_cluster_id_tag, size_t> LegalizationClusterId;

// A unique ID for each candidate cluster created by the ClusterLegalizer,
// whether or not it is later committed. Unlike LegalizationClusterId, it is
// assigned when the cluster is started and is never reused.
typedef vtr::StrongId<struct candidate_cluster_id_tag, size_t> CandidateClusterId;
