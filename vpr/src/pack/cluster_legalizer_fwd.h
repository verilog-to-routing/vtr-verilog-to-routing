#pragma once

// A special ID to identify the legalization clusters. This is separate from the
// ClusterBlockId since ClusterLegalizer does not represent the final clustered
// netlist, but provides routines and structures that assist in its construction.
typedef vtr::StrongId<struct legalization_cluster_id_tag, size_t> LegalizationClusterId;
