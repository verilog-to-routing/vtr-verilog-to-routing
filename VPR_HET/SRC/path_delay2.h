/*********** Types and defines used by all path_delay modules ****************/

typedef struct
{
    int to_node;
    float Tdel;
}
t_tedge;

/* to_node: index of node at the sink end of this edge.                      *
 * Tdel: delay to go to to_node along this edge.                             */


typedef struct
{
    t_tedge *out_edges;
    int num_edges;
    float T_arr;
    float T_req;
}
t_tnode;

/* out_edges: [0..num_edges - 1].  Array of the edges leaving this tnode.    *
 * num_edges: Number of edges leaving this node.                             *
 * T_arr:  Arrival time of the last input signal to this node.               *
 * T_req:  Required arrival time of the last input signal to this node if    *
 *         the critical path is not to be lengthened.                        */


/* Info. below is only used to print out and display the critical path.  It  *
 * gives a mapping from each t_node to what circuit element it represents.   *
 * I put this info in a separate structure to maximize cache effectiveness,  *
 * since it's not used much.                                                 */

typedef enum
{ INPAD_SOURCE, INPAD_OPIN, OUTPAD_IPIN, OUTPAD_SINK,
    FB_IPIN, FB_OPIN, SUBBLK_IPIN, SUBBLK_OPIN, FF_SINK, FF_SOURCE,
    CONSTANT_GEN_SOURCE
}
t_tnode_type;

typedef struct
{
    t_tnode_type type;
    short ipin;
    short isubblk;
    int iblk;
}
t_tnode_descript;

/* type:  What is this tnode? (Pad pin, clb pin, subblock pin, etc.)         *
 * ipin:  Number of the FB or subblock pin this tnode represents, if        *
 *        applicable.                                                        *
 * isubblk: Number of the subblock this tnode is part of, if applicable.     *
 * iblk:  Number of the block (FB or PAD) this tnode is part of.            */



/*************** Variables shared only amongst path_delay modules ************/

extern t_tnode *tnode;		/* [0..num_tnodes - 1] */
extern t_tnode_descript *tnode_descript;	/* [0..num_tnodes - 1] */
extern int num_tnodes;		/* Number of nodes in the timing graph */


/* [0..num_nets - 1].  Gives the index of the tnode that drives each net. */

extern int *net_to_driver_tnode;


/* [0..num__tnode_levels - 1].  Count and list of tnodes at each level of    *
 * the timing graph, to make breadth-first searches easier.                  */

extern struct s_ivec *tnodes_at_level;
extern int num_tnode_levels;	/* Number of levels in the timing graph. */



/***************** Subroutines exported by this module ***********************/

int alloc_and_load_timing_graph_levels(void);

void check_timing_graph(int num_const_gen,
			int num_ff,
			int num_sinks);

float print_critical_path_node(FILE * fp,
			       t_linked_int * critical_path_node,
			       t_subblock_data subblock_data);
