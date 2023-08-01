#pragma once

#include <unordered_map>
#include <vector>
#include "connection_based_routing.h"
#include "netlist.h"
#include "vpr_types.h"

#include "vpr_utils.h"
#include "timing_info_fwd.h"
#include "route_budgets.h"
#include "router_stats.h"
#include "router_lookahead.h"
#include "spatial_route_tree_lookup.h"
#include "connection_router_interface.h"
#include "heap_type.h"
#include "routing_predictor.h"

#ifdef VPR_USE_TBB
/** Route in parallel. The number of threads is set by the global -j option to VPR.
 * Return success status. */
bool try_parallel_route(const Netlist<>& net_list,
                        const t_det_routing_arch& det_routing_arch,
                        const t_router_opts& router_opts,
                        const t_analysis_opts& analysis_opts,
                        const std::vector<t_segment_inf>& segment_inf,
                        NetPinsMatrix<float>& net_delay,
                        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                        std::shared_ptr<SetupHoldTimingInfo> timing_info,
                        std::shared_ptr<RoutingDelayCalculator> delay_calc,
                        ScreenUpdatePriority first_iteration_priority,
                        bool is_flat);
#endif
