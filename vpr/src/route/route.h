#pragma once

#include "RoutingDelayCalculator.h"
#include "timing_info.h"
#include "vpr_net_pins_matrix.h"
#include "vpr_types.h"
#include "netlist.h"

/** Attempts a routing via the AIR algorithm [0].
 *
 * \p width_fac specifies the relative width of the channels, while the members of
 * \p router_opts determine the value of the costs assigned to routing
 * resource node, etc. \p det_routing_arch describes the detailed routing
 * architecture (connection and switch boxes) of the FPGA; it is used
 * only if a DETAILED routing has been selected.
 *
 * [0]: K. E. Murray, S. Zhong, and V. Betz, "AIR: A fast but lazy timing-driven FPGA router", in ASPDAC 2020
 *
 * \return Success status. */
bool route(const Netlist<>& net_list,
           int width_fac,
           const t_router_opts& router_opts,
           const t_analysis_opts& analysis_opts,
           t_det_routing_arch* det_routing_arch,
           std::vector<t_segment_inf>& segment_inf,
           NetPinsMatrix<float>& net_delay,
           std::shared_ptr<SetupHoldTimingInfo> timing_info,
           std::shared_ptr<RoutingDelayCalculator> delay_calc,
           t_chan_width_dist chan_width_dist,
           const std::vector<t_direct_inf>& directs,
           ScreenUpdatePriority first_iteration_priority,
           bool is_flat);
