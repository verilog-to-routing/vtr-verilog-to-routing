#ifndef READ_PLACE_H
#define READ_PLACE_H

void read_place(INP const char *place_file,
		INP const char *arch_file,
		INP const char *net_file,
		INP int L_nx,
		INP int L_ny,
		INP int L_num_blocks,
		INOUTP struct s_block block_list[]);

void print_place(INP char *place_file,
		INP char *net_file,
		INP char *arch_file);

void read_user_pad_loc(INP char *pad_loc_file);

#endif

