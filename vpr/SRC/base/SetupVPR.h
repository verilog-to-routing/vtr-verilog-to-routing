#ifndef SETUPVPR_H
#define SETUPVPR_H
#include <vector>
#include "logic_types.h"
#include "ReadOptions.h"
#include "physical_types.h"
#include "vpr_types.h"


void SetupVPR(t_options *Options, 
        const bool TimingEnabled,
		const bool readArchFile, 
        struct s_file_name_opts *FileNameOpts,
		t_arch * Arch,
		t_model ** user_models, 
        t_model ** library_models,
		t_netlist_opts* NetlistOpts,
		struct s_packer_opts *PackerOpts,
		struct s_placer_opts *PlacerOpts,
		struct s_annealing_sched *AnnealSched,
		struct s_router_opts *RouterOpts,
		struct s_det_routing_arch *RoutingArch,
		std::vector<t_lb_type_rr_node> **PackerRRGraphs,
		t_segment_inf ** Segments, t_timing_inf * Timing,
		bool * ShowGraphics, int *GraphPause,
		t_power_opts * PowerOpts);
#endif
