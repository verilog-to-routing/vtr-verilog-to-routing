float **alloc_and_load_timing_graph(t_timing_inf timing_inf);

float **alloc_and_load_pre_packing_timing_graph(float block_delay,
		float inter_cluster_net_delay, t_model *models);

t_linked_int *allocate_and_load_critical_path(void);

void load_timing_graph_net_delays(float **net_delay);

float load_net_slack(float **net_slack);

void free_timing_graph(float **net_slack);

void print_timing_graph(char *fname);

void print_net_slack(char *fname, float **net_slack);

void print_critical_path(char *fname);

void get_tnode_block_and_output_net(int inode, int *iblk_ptr, int *inet_ptr);

void do_constant_net_delay_timing_analysis(t_timing_inf timing_inf,
		float constant_net_delay_value);

void print_timing_graph_as_blif(char *fname, t_model *models);