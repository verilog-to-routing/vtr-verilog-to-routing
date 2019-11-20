#define INFINITE -1
#define NOT_FOUND 0

#define WNEED 1
#define WL 2
#define PROC_TIME 3

typedef struct s_fmap_cell {
	int fs; /* at this fs */
	int fc; /* at this fc */
	int wneed; /* need wneed to route */
	int wirelength; /* corresponding wirelength of successful routing at wneed */
	int proc_time;
	struct s_fmap_cell *next;
} t_fmap_cell;

void place_and_route(enum e_operation operation,
		struct s_placer_opts placer_opts, char *place_file, char *net_file,
		char *arch_file, char *route_file,
		struct s_annealing_sched annealing_sched,
		struct s_router_opts router_opts,
		struct s_det_routing_arch det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, t_chan_width_dist chan_width_dist,
		struct s_model *models, t_direct_inf *directs, int num_directs);

void init_chan(int cfactor, t_chan_width_dist chan_width_dist);
