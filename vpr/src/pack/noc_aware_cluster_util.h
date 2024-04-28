
#ifndef VTR_NOC_AWARE_CLUSTER_UTIL_H
#define VTR_NOC_AWARE_CLUSTER_UTIL_H

#include <vector>

#include "vpr_types.h"

std::vector<AtomBlockId> find_noc_router_atoms();

void update_noc_reachability_partitions(const std::vector<AtomBlockId>& noc_atoms);

#endif
