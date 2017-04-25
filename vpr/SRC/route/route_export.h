/******** Function prototypes for functions in route_common.c that ***********
 ******** are used outside the router modules.                     ***********/
#include "vpr_types.h"
#include <memory>
#include "timing_info_fwd.h"

void try_graph(int width_fac, struct s_router_opts router_opts,
		struct s_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_chan_width_dist chan_width_dist,
		t_direct_inf *directs, int num_directs);

bool try_route(int width_fac, struct s_router_opts router_opts,
		struct s_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		float **net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack * slacks,
#endif
        std::shared_ptr<SetupTimingInfo> timing_info,
		t_chan_width_dist chan_width_dist, vtr::t_ivec ** clb_opins_used_locally,
		t_direct_inf *directs, int num_directs);

bool feasible_routing(void);

vtr::t_ivec **alloc_route_structs(void);

void free_route_structs();

struct s_trace **alloc_saved_routing(vtr::t_ivec ** clb_opins_used_locally,
		vtr::t_ivec *** saved_clb_opins_used_locally_ptr);

void free_saved_routing(struct s_trace **best_routing,
		vtr::t_ivec ** saved_clb_opins_used_locally);

void save_routing(struct s_trace **best_routing,
		vtr::t_ivec ** clb_opins_used_locally,
		vtr::t_ivec ** saved_clb_opins_used_locally);

void restore_routing(struct s_trace **best_routing,
		vtr::t_ivec ** clb_opins_used_locally,
		vtr::t_ivec ** saved_clb_opins_used_locally);

void get_serial_num(void);

void print_route(const char* place_file, const char* route_file);
