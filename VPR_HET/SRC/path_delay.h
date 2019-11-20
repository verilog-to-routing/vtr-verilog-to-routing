float **alloc_and_load_timing_graph(t_timing_inf timing_inf,
				    t_subblock_data subblock_data);

t_linked_int *allocate_and_load_critical_path(void);

void load_timing_graph_net_delays(float **net_delay);

float load_net_slack(float **net_slack,
		     float target_cycle_time);

void free_timing_graph(float **net_slack);

void print_timing_graph(char *fname);

void print_net_slack(char *fname,
		     float **net_slack);

void print_critical_path(char *fname,
			 t_subblock_data subblock_data);

void get_tnode_block_and_output_net(int inode,
				    int *iblk_ptr,
				    int *inet_ptr);

void do_constant_net_delay_timing_analysis(t_timing_inf timing_inf,
					   t_subblock_data subblock_data,
					   float constant_net_delay_value);
