#ifndef READ_PLACE_H
#define READ_PLACE_H

void read_place(const char *place_file,
		const char *arch_file,
		const char *net_file,
		const int L_nx,
		const int L_ny,
		const int L_num_blocks,
		struct s_block block_list[]);

void print_place(const char *place_file,
		const char *net_file,
		const char *arch_file);

void read_user_pad_loc(const char *pad_loc_file);

#endif

