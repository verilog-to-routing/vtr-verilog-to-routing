#ifndef VTR_SATROUTING_H
#define VTR_SATROUTING_H

/**
 * @file
 * @brief SAT formulation of NoC traffic flow routing.
 *
 * This file implements a SAT formulation of NoC routing problem.
 * Each (traffic flow, link) pair is associated with boolean variable.
 * When one of these boolean variables are set by the SAT solver, it means
 * that the corresponding traffic flow is routed through the corresponding link.
 * To ensure that traffic flow routes are continuous, deadlock-free, with
 * minimum link congestion, several constraints are added to the SAT formulation.
 *
 * For more details refer to the following paper:
 * The Road Less Traveled: Congestion-Aware NoC Placement and Packet Routing for FPGAs
 */

#ifdef ENABLE_NOC_SAT_ROUTING

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
                                                                    const t_noc_opts& noc_opts,
                                                                    int seed);

namespace std {

template<>
struct hash<std::pair<NocTrafficFlowId, NocLinkId>> {
    /**
     * @brief Generates a hash value for a (NocTrafficFlowId, NocLinkId) pair.
     *
     * This hash function is used to store SAT boolean variables for each
     * (traffic flow, link) pair in a hash map.
     *
     * @param flow_link A (traffic flow, link) pair whose hash value is desired.
     * @return The computed hash value.
     */
    std::size_t operator()(const std::pair<NocTrafficFlowId, NocLinkId>& flow_link) const noexcept {
        std::size_t seed = std::hash<NocTrafficFlowId>{}(flow_link.first);
        vtr::hash_combine(seed, flow_link.second);
        return seed;
    }
};
} // namespace std


#endif
#endif