#ifndef RR_GRAPH_H
#define RR_GRAPH_H


/* Include track buffers or not. Track buffers isolate the tracks from the
   input connection block. However, they are difficult to lay out in practice,
   and so are not currently used in commercial architectures. */
#define INCLUDE_TRACK_BUFFERS false


enum e_graph_type {
	GRAPH_GLOBAL, /* One node per channel with wire capacity > 1 and full connectivity */
	GRAPH_BIDIR, /* Detailed bidirectional graph */
	GRAPH_UNIDIR, /* Detailed unidir graph, untilable */
	/* RESEARCH TODO: Get this option debugged */
	GRAPH_UNIDIR_TILEABLE /* Detail unidir graph with wire groups multiples of 2*L */
};
typedef enum e_graph_type t_graph_type;

/* Warnings about the routing graph that can be returned.
 * This is to avoid output messages during a value sweep */
enum {
	RR_GRAPH_NO_WARN = 0x00,
	RR_GRAPH_WARN_FC_CLIPPED = 0x01,
	RR_GRAPH_WARN_CHAN_WIDTH_CHANGED = 0x02
};

void build_rr_graph(
		INP t_graph_type graph_type, 
		INP int L_num_types,
		INP t_type_ptr types, 
		INP int L_nx, 
		INP int L_ny,
		INP struct s_grid_tile **L_grid, 
		INP struct s_chan_width *nodes_per_chan,
		INP struct s_chan_width_dist *chan_capacity_inf,
		INP enum e_switch_block_type sb_type, 
		INP int Fs,
		INP std::vector<t_switchblock_inf> switchblocks,
		INP int num_seg_types,
		INP int num_arch_switches, 
		INP t_segment_inf * segment_inf,
		INP int global_route_switch, 
		INP int delayless_switch,
		INP t_timing_inf timing_inf, 
		INP int wire_to_ipin_switch,
		INP enum e_base_cost_type base_cost_type, 
		INP bool trim_empty_channels,
		INP bool trim_obs_channels,
		INP t_direct_inf *directs, 
		INP int num_directs,
		INP bool ignore_Fc_0,
		INP const char *dump_rr_structs_file,
		OUTP int *wire_to_rr_ipin_switch,
		OUTP int *num_rr_switches,
		OUTP int *Warnings);

void free_rr_graph(void);

void dump_rr_graph(INP const char *file_name);
void print_rr_indexed_data(FILE * fp, int index); /* For debugging only */
void load_net_rr_terminals(t_ivec *** L_rr_node_indices);

void print_rr_node(FILE *fp, t_rr_node *L_rr_node, int inode);

#endif

