boolean is_opin(int ipin,
		t_type_ptr type);

void get_class_range_for_block(IN int iblk,
			       OUT int *class_low,
			       OUT int *class_high);

void load_one_fb_fanout_count(t_subblock * subblock_inf,
			      int num_subblocks,
			      int *num_uses_of_fb_ipin,
			      int **num_uses_of_sblk_opin,
			      int iblk);

void sync_nets_to_blocks(IN int num_blocks,
			 IN const struct s_block block_list[],
			 IN int num_nets,
			 INOUT struct s_net net_list[]);

void sync_grid_to_blocks(IN int num_blocks,
			 IN const struct s_block block_list[],
			 IN int nx,
			 IN int ny,
			 INOUT struct s_grid_tile **grid);
