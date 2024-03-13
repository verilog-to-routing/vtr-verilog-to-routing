#ifndef VTR_SATROUTING_H
#define VTR_SATROUTING_H

#include <utility>

#include "noc_data_types.h"
#include "vtr_hash.h"
#include "vtr_vector.h"
#include "clustered_netlist_fwd.h"

vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> noc_sat_route(bool minimize_aggregate_bandwidth,
                                                                    int seed);

namespace std {
template<>
struct hash<std::pair<NocTrafficFlowId, NocLinkId>> {
    std::size_t operator()(const std::pair<NocTrafficFlowId, NocLinkId>& flow_link) const noexcept {
        std::size_t seed = std::hash<NocTrafficFlowId>{}(flow_link.first);
        vtr::hash_combine(seed, flow_link.second);
        return seed;
    }
};

template<>
struct hash<std::pair<ClusterBlockId, NocRouterId>> {
    std::size_t operator()(const std::pair<ClusterBlockId, NocRouterId>& block_router) const noexcept {
        std::size_t seed = std::hash<ClusterBlockId>{}(block_router.first);
        vtr::hash_combine(seed, block_router.second);
        return seed;
    }
};

} // namespace std


#endif