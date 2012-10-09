void routing_stats(boolean full_stats, enum e_route_type route_type,
		int num_switch, t_segment_inf * segment_inf, int num_segment,
		float R_minW_nmos, float R_minW_pmos,
		enum e_directionality directionality, boolean timing_analysis_enabled,
		float **net_delay, t_slack * slacks);

void print_wirelen_prob_dist(void);

void print_lambda(void);

void get_num_bends_and_length(int inet, int *bends, int *length, int *segments);

int count_netlist_clocks(void);
