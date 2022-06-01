/******** Function prototypes for functions in route_common.c that ***********
 ******** are used outside the router modules.                     ***********/
#include "vpr_types.h"
#include <memory>
#include "timing_info_fwd.h"
#include "route_common.h"
#include "RoutingDelayCalculator.h"

void try_graph(int width_fac,
               const t_router_opts& router_opts,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               t_chan_width_dist chan_width_dist,
               t_direct_inf* directs,
               int num_directs);

bool try_route(const Netlist<>& net_list,
               int width_fac,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               NetPinsMatrix<float>& net_delay,
               std::shared_ptr<SetupHoldTimingInfo> timing_info,
               std::shared_ptr<RoutingDelayCalculator> delay_calc,
               t_chan_width_dist chan_width_dist,
               t_direct_inf* directs,
               int num_directs,
               ScreenUpdatePriority first_iteration_priority);

bool feasible_routing();

std::vector<int> collect_congested_rr_nodes();

std::vector<std::set<ClusterNetId>> collect_rr_node_nets();

t_clb_opins_used alloc_route_structs();

void free_route_structs();

vtr::vector<ParentNetId, t_trace*> alloc_saved_routing(const Netlist<>& net_list);

void free_saved_routing(const Netlist<>& net_list,
                        vtr::vector<ParentNetId, t_trace*>& best_routing);

void save_routing(const Netlist<>& net_list,
                  vtr::vector<ParentNetId, t_trace*>& best_routing,
                  const t_clb_opins_used& clb_opins_used_locally,
                  t_clb_opins_used& saved_clb_opins_used_locally);

void restore_routing(const Netlist<>& net_list,
                     vtr::vector<ParentNetId, t_trace*>& best_routing,
                     t_clb_opins_used& clb_opins_used_locally,
                     const t_clb_opins_used& saved_clb_opins_used_locally);

void get_serial_num(const Netlist<>& net_list);

void print_route(const Netlist<>& net_list,
                 const char* place_file,
                 const char* route_file,
                 bool is_flat);
void print_route(const Netlist<>& net_list,
                 FILE* fp,
                 const vtr::vector<ParentNetId, t_traceback>& tracebacks,
                 bool is_flat);
