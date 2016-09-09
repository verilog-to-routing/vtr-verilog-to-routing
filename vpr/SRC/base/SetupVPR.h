#ifndef SETUPVPR_H
#define SETUPVPR_H



void SetupVPR(t_options *Options,
		const bool TimingEnabled,
		const bool readArchFile,
		struct s_file_name_opts *FileNameOpts,
		t_arch * Arch,
		enum e_operation *Operation,
		t_model ** user_models,
		t_model ** library_models,
		struct s_packer_opts *PackerOpts,
		struct s_placer_opts *PlacerOpts,
		struct s_annealing_sched *AnnealSched,
		struct s_router_opts *RouterOpts,
		struct s_det_routing_arch *RoutingArch,
		vector <t_lb_type_rr_node> **PackerRRGraph,
		t_segment_inf ** Segments,
		t_timing_inf * Timing,
		bool * ShowGraphics,
		int *GraphPause,
		t_power_opts * power_opts);

void CheckSetup(const enum e_operation Operation,
		const struct s_placer_opts PlacerOpts,
		const struct s_annealing_sched AnnealSched,
		const struct s_router_opts RouterOpts,
		const struct s_det_routing_arch RoutingArch,
		const t_segment_inf * Segments,
		const t_timing_inf Timing,
		const t_chan_width_dist Chans);

void CheckArch(const t_arch Arch,
		const bool TimingEnabled);

void ShowSetup(const t_options options, const t_vpr_setup vpr_setup);
void printClusteredNetlistStats(void);

#endif

