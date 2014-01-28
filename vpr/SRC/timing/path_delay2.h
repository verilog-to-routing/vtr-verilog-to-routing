/*********** Types and defines used by all path_delay modules ****************/

extern t_tnode *tnode; /* [0..num_tnodes - 1] */
extern int num_tnodes; /* Number of nodes in the timing graph */

extern int num_tnode_levels; /* Number of levels in the timing graph. */

extern struct s_ivec *tnodes_at_level;
/* [0..num__tnode_levels - 1].  Count and list of tnodes at each level of    
 * the timing graph, to make topological searches easier. Level-0 nodes are
 * sources to the timing graph (types TN_FF_SOURCE, TN_INPAD_SOURCE
 * and TN_CONSTANT_GEN_SOURCE). Level-N nodes are in the immediate fanout of 
 * nodes with level at most N-1. */

/***************** Subroutines exported by this module ***********************/

int alloc_and_load_timing_graph_levels(void);

void check_timing_graph(int num_sinks);

float print_critical_path_node(FILE * fp, t_linked_int * critical_path_node);

void detect_and_fix_timing_graph_combinational_loops();
