#ifndef READ_PLACE_H
#define READ_PLACE_H

void read_place(
		const char *net_file,
        const char *place_file,
        bool verify_file_hashes,
		const int L_nx,
		const int L_ny,
		const int L_num_blocks,
		t_block block_list[]);

void print_place(const char* net_file, 
                 const char* net_id, 
                 const char* place_file);

void read_user_pad_loc(const char *pad_loc_file);

#endif

