#ifndef VPR_PLACE_AND_ROUTE_H
#define VPR_PLACE_AND_ROUTE_H

#define INFINITE -1
#define NOT_FOUND 0

#define WNEED 1
#define WL 2
#define PROC_TIME 3

#include "vpr_types.h"
#include "timing_info.h"

struct t_fmap_cell {
	int fs; /* at this fs */
	int fc; /* at this fc */
	int wneed; /* need wneed to route */
	int wirelength; /* corresponding wirelength of successful routing at wneed */
	int proc_time;
	t_fmap_cell *next;
};

bool place_and_route(
		t_placer_opts placer_opts,
		t_file_name_opts  filename_opts,
        const t_arch* arch,
		t_annealing_sched annealing_sched,
		t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf);

int binary_search_place_and_route(t_placer_opts placer_opts,
        t_file_name_opts filename_opts,
        const t_arch* arch,
        bool verify_binary_search, int min_chan_width_hint,
        t_annealing_sched annealing_sched,
        t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, std::vector<t_segment_inf>& segment_inf,
        vtr::vector_map<ClusterNetId, float *> &net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
        const t_timing_inf& timing_inf,
#endif
        std::shared_ptr<SetupHoldTimingInfo> timing_info);

void init_chan(int cfactor, t_chan_width_dist chan_width_dist);

void post_place_sync();

#endif
