/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef ROUTER_EXPORT_H
#define ROUTER_EXPORT_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */


/******** Function prototypes for functions in route_common.c that ***********
 ******** are used outside the router modules.                     ***********/
#include "vpr_types.h"
#include <memory>
#include "timing_info_fwd.h"
#include "RoutingDelayCalculator.h"

namespace router {

void try_graph(int width_fac, t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, std::vector<t_segment_inf>& segment_inf,
		t_chan_width_dist chan_width_dist,
		t_direct_inf *directs, int num_directs);

bool try_route(int width_fac,
        const t_router_opts& router_opts,
        const t_analysis_opts& analysis_opts,
		t_det_routing_arch *det_routing_arch,
        std::vector<t_segment_inf>& segment_inf,
		vtr::vector<ClusterNetId, float *> &net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack * slacks,
        const t_timing_inf& timing_inf,
#endif
        std::shared_ptr<SetupHoldTimingInfo> timing_info,
        std::shared_ptr<RoutingDelayCalculator> delay_calc, 
		t_chan_width_dist chan_width_dist,
		t_direct_inf *directs, int num_directs,
        ScreenUpdatePriority first_iteration_priority);

bool feasible_routing();

std::vector<int> collect_congested_rr_nodes();

std::vector<std::set<ClusterNetId>> collect_rr_node_nets();

t_clb_opins_used alloc_route_structs();

void free_route_structs();

vtr::vector<ClusterNetId, t_trace *> alloc_saved_routing();

void free_saved_routing(vtr::vector<ClusterNetId, t_trace *> &best_routing);

void save_routing(vtr::vector<ClusterNetId, t_trace *> &best_routing,
		const t_clb_opins_used& clb_opins_used_locally,
		t_clb_opins_used& saved_clb_opins_used_locally);

void restore_routing(vtr::vector<ClusterNetId, t_trace *> &best_routing,
		t_clb_opins_used& clb_opins_used_locally,
		const t_clb_opins_used& saved_clb_opins_used_locally);

void get_serial_num();

void print_route(const char* place_file, const char* route_file);
void print_route(FILE* fp, const vtr::vector<ClusterNetId,t_traceback>& tracebacks);
} /* end namespace router */

#endif
