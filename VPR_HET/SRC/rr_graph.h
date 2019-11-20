enum e_graph_type
{
    GRAPH_GLOBAL,		/* One node per channel with wire capacity > 1 and full connectivity */
    GRAPH_BIDIR,		/* Detailed bidirectional graph */
    GRAPH_UNIDIR,		/* Detailed unidir graph, untilable */
    /* RESEARCH TODO: Get this option debugged */
    GRAPH_UNIDIR_TILEABLE	/* Detail unidir graph with wire groups multiples of 2*L */
};
typedef enum e_graph_type t_graph_type;

/* Warnings about the routing graph that can be returned.
 * This is to avoid output messages during a value sweep */
enum
{
    RR_GRAPH_NO_WARN = 0x00,
    RR_GRAPH_WARN_FC_CLIPPED = 0x01,
    RR_GRAPH_WARN_CHAN_WIDTH_CHANGED = 0x02
};

void build_rr_graph(IN t_graph_type graph_type,
			   IN int num_types,
			   IN t_type_ptr types,
			   IN int nx,
			   IN int ny,
			   IN struct s_grid_tile **grid,
			   IN int chan_width,
			   IN struct s_chan_width_dist *chan_capacity_inf,
			   IN enum e_switch_block_type sb_type,
			   IN int Fs,
			   IN int num_seg_types,
			   IN int num_switches,
			   IN t_segment_inf * segment_inf,
			   IN int global_route_switch,
			   IN int delayless_switch,
			   IN t_timing_inf timing_inf,
			   IN int wire_to_ipin_switch,
			   IN enum e_base_cost_type base_cost_type,
			   OUT int *Warnings);

void free_rr_graph(void);

void dump_rr_graph(IN const char *file_name);
void print_rr_indexed_data(FILE * fp,
			   int index);	/* For debugging only */
void load_net_rr_terminals(t_ivec *** rr_node_indices);
