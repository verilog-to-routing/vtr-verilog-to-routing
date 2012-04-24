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


int get_max_primitives_in_pb_type(t_pb_type *pb_type);
int get_max_depth_of_pb_type(t_pb_type *pb_type);
int get_max_nets_in_pb_type(const t_pb_type *pb_type);
boolean primitive_type_feasible(int iblk, const t_pb_type *cur_pb_type);
float compute_primitive_base_cost(INP t_pb_graph_node *primitive);
int num_ext_inputs_logical_block (int iblk);

#endif

