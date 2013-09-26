#ifndef VPR_UTILS_H
#define VPR_UTILS_H

#include "util.h"

void print_tabs(FILE * fpout, int num_tab);

boolean is_opin(int ipin, t_type_ptr type);
int get_unique_pb_graph_node_id(t_pb_graph_node *pb_graph_node);

void get_class_range_for_block(INP int iblk, OUTP int *class_low,
		OUTP int *class_high);

void sync_grid_to_blocks(INP int L_num_blocks,
		INP const struct s_block block_list[], INP int L_nx, INP int L_ny,
		INOUTP struct s_grid_tile **L_grid);

/**************************************************************
* Intra-Logic Block Utility Functions
**************************************************************/
int get_max_primitives_in_pb_type(t_pb_type *pb_type);
int get_max_depth_of_pb_type(t_pb_type *pb_type);
int get_max_nets_in_pb_type(const t_pb_type *pb_type);
boolean primitive_type_feasible(int iblk, const t_pb_type *cur_pb_type);
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(t_model_ports *model_port, int model_pin, t_pb_graph_node *pb_graph_node);
t_pb_graph_pin* get_pb_graph_node_pin_from_g_atoms_nlist_net(int inet, int ipin);
t_pb_graph_pin* get_pb_graph_node_pin_from_g_clbs_nlist_net(int inet, int ipin);
t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(int iblock, int ipin);
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_type_ptr type);
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type);


float compute_primitive_base_cost(INP t_pb_graph_node *primitive);
int num_ext_inputs_logical_block(int iblk);

int ** alloc_and_load_net_pin_index();

void get_port_pin_from_blk_pin(int blk_type_index, int blk_pin, int * port,
		int * port_pin);
void free_port_pin_from_blk_pin(void);

void get_blk_pin_from_port_pin(int blk_type_index, int port,int port_pin, 
		int * blk_pin);
void free_blk_pin_from_port_pin(void);

void alloc_and_load_idirect_from_blk_pin(t_direct_inf* directs, int num_directs, 
		int *** idirect_from_blk_pin, int *** direct_type_from_blk_pin);

void parse_direct_pin_name(char * src_string, int line, int * start_pin_index, 
		int * end_pin_index, char * pb_type_name, char * port_name);


void free_cb(t_pb *pb);
void free_pb_stats(t_pb *pb);
void free_pb(t_pb *pb);


#endif

