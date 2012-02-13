#ifndef CLUSTER_LEGALITY_H
#define CLUSTER_LEGALITY_H

/* Legalizes routing for a cluster
 */
void alloc_and_load_cluster_legality_checker();

void alloc_and_load_legalizer_for_cluster(INP t_block* clb, INP int clb_index, INP const t_arch *arch);

void free_legalizer_for_cluster(INP t_block* clb);

void free_cluster_legality_checker();

void reset_legalizer_for_cluster(t_block *clb);

enum e_block_pack_status try_add_block_to_current_cluster_and_primitive(INP int logical_block, INP t_pb *primitive);

void save_cluster_solution();

boolean is_pin_open(int i);

void set_pb_mode(t_pb *pb, int mode, int isOn);

void alloc_and_load_rr_graph_for_pb_graph_node(INP t_pb_graph_node *pb_graph_node, INP const t_arch* arch, int mode);


#endif
