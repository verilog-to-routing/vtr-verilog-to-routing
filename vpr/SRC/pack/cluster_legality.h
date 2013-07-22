#ifndef CLUSTER_LEGALITY_H
#define CLUSTER_LEGALITY_H

/* Legalizes routing for a cluster
 */
void alloc_and_load_cluster_legality_checker(void);

void alloc_and_load_legalizer_for_cluster(INP t_block* clb, INP int clb_index, INP const t_arch *arch);

void free_legalizer_for_cluster(INP t_block* clb, boolean free_local_rr_graph);

void free_cluster_legality_checker(void);

void reset_legalizer_for_cluster(t_block *clb);

/* order of use: 1. save cluster 2. Add blocks.  3. route 4. save if successful, undo if not successful */
void save_and_reset_routing_cluster(void);
void setup_intracluster_routing_for_molecule(INP t_pack_molecule *molecule, INOUTP t_pb_graph_node **primitives_list);
boolean try_breadth_first_route_cluster(void);
void restore_routing_cluster(void);
void save_cluster_solution(void);

boolean is_pin_open(int i);

void set_pb_graph_mode(t_pb_graph_node *pb_graph_node, int mode, int isOn);

void alloc_and_load_rr_graph_for_pb_graph_node(INP t_pb_graph_node *pb_graph_node, INP const t_arch* arch, int mode);


/* Power user options */
void reload_ext_net_rr_terminal_cluster(void);
boolean force_post_place_route_cb_input_pins(int iblock);
void setup_intracluster_routing_for_logical_block(INP int iblock,
		INP t_pb_graph_node *primitive);


#endif
