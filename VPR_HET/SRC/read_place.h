void read_place(IN const char *place_file,
		IN const char *arch_file,
		IN const char *net_file,
		IN int nx,
		IN int ny,
		IN int num_blocks,
		INOUT struct s_block block_list[]);

void print_place(IN char *place_file,
		 IN char *net_file,
		 IN char *arch_file);

void read_user_pad_loc(IN char *pad_loc_file);
