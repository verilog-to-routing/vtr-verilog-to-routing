#ifndef VTR_SATROUTING_H
#define VTR_SATROUTING_H

#include <utility>
#include <map>

#include "noc_data_types.h"
#include "vpr_types.h"
#include "vtr_hash.h"
#include "vtr_vector.h"
#include "clustered_netlist_fwd.h"

/**
 * @brief Assuming that logical NoC routers are fixed,
 * this function formulates routing traffic flows as a SAT
 * problem and tries to find a deadlock-free and congestion-free
 * routing with the minimum aggregate bandwidth while meeting
 * traffic flow latency constraints.
 *
 * @param minimize_aggregate_bandwidth Indicates whether the SAT solver
 * should minimize the aggregate bandwidth or not. A congestion-free
 * and deadlock-free solution can be found faster if the solver does not
 * need to minimize the aggregate bandwidth.
 * @param seed An integer seed to initialize the SAT solver.
 * @return The generated routes for all traffic flows.
 */
vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> noc_sat_route(bool minimize_aggregate_bandwidth,
                                                                    int bandwidth_resolution,
                                                                    int seed);

//void noc_sat_place_and_route(vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& traffic_flow_routes,
//                             std::map<ClusterBlockId, t_pl_loc>& noc_router_locs,
//                             int seed);

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