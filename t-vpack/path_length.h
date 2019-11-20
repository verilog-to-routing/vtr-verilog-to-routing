void  update_blocks_in_cluster_delays(int *block_in_cluster, 
		             int next_empty_location_in_cluster);
boolean report_critical_path(float block_delay, float inter_cluster_net_delay, 
			float intra_cluster_net_delay, int farthest_block,
			int cluster_size, boolean print_data);
void calculate_timing_information(float block_delay, float inter_cluster_net_delay, 
			          float intra_cluster_net_delay,
                                  int num_clusters,
                                  int num_blocks_clustered,
				  int *farthest_block_discovered);
int get_most_critical_unclustered_block(void); 
void alloc_and_init_path_length(int lut_sz, boolean *is_clk);
void free_path_length_memory(void);
int get_net_pin_number(int blk, int blkpin); 
float get_net_pin_forward_criticality(int netnum, int pinnum);
float get_net_pin_backward_criticality(int netnum, int pinnum);
