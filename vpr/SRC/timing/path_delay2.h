/*********** Types and defines used by all path_delay modules ****************/

extern t_tnode *tnode; /* [0..num_tnodes - 1] */
extern int num_tnodes; /* Number of nodes in the timing graph */

/* [0..num_nets - 1].  Gives the index of the tnode that drives each net. */

extern int *net_to_driver_tnode;

/* [0..num__tnode_levels - 1].  Count and list of tnodes at each level of    *
 * the timing graph, to make breadth-first searches easier.                  */

extern struct s_ivec *tnodes_at_level;
extern int num_tnode_levels; /* Number of levels in the timing graph. */

/***************** Subroutines exported by this module ***********************/

int alloc_and_load_timing_graph_levels(void);

void check_timing_graph(int num_sinks);

float print_critical_path_node(FILE * fp, t_linked_int * critical_path_node);
