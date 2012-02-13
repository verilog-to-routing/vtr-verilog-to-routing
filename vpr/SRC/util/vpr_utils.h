#ifndef VPR_UTILS_H
#define VPR_UTILS_H

#include "util.h"

void print_tabs(FILE * fpout, int num_tab);

boolean is_opin(int ipin,
		t_type_ptr type);

void get_class_range_for_block(INP int iblk,
			       OUTP int *class_low,
			       OUTP int *class_high);

void sync_grid_to_blocks(INP int num_blocks,
			 INP const struct s_block block_list[],
			 INP int nx,
			 INP int ny,
			 INOUTP struct s_grid_tile **grid);

#endif

