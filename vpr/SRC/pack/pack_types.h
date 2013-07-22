/**
 * Jason Luu
 * July 22, 2013
 *
 * Defines core data structures used in packing
 */


/**************************************************************************
* Intra-Logic Block Routing Data Structures
***************************************************************************

/* Describes different types of intra-logic block routing resource nodes */
enum e_lb_rr_type {
	LB_SOURCE = 0, LB_SINK, LB_INTERMEDIATE, NUM_LB_RR_TYPES
};

/* Describes a routing resource node within a logic block type */
typedef struct s_lb_type_rr_node {
	short capacity;			/* Number of nets that can simultaneously use this node */
	short *num_fanout;		/* [0..num_modes - 1] Mode dependant fanout */
	enum e_lb_rr_type type;	/* Type of logic block resource node */	

	int **fanout;					/* [0..num_modes - 1][0..num_fanout-1] index of fanout lb_rr_node */
	float **fanout_intrinsic_cost;	/* [0..num_modes - 1][0..num_fanout-1] cost of fanout lb_rr_node */

	t_pb_graph_pin *pb_graph_pin;	/* pb_graph_pin associated with this lb_rr_node if exists */
	float pack_intrinsic_cost;		/* cost of this node */
} t_lb_type_rr_node;

/* The routing traceback for a net */
typedef struct s_lb_traceback {
	int	net;				/* net using this node */
	int prev_edge;			/* index of previous edge that drives current node */
	int prev_lb_rr_node;	/* index of previous node that drives current node */
	boolean is_valid;		/* whether or not this traceback is valid */
} t_lb_traceback;


/* Describes the status of a logic block routing resource node for a given logic block instance */
typedef struct s_lb_rr_node {
	int occ;				/* Number of nets currently using this lb_rr_node */
	int max_occ;			/* Maximium number of nets allowed to use this lb_rr_node in any intermediate stage of routing, this value should be higher than capacity to allow hill-climbing */
	int mode;				/* Mode that this node is set to */

	t_lb_traceback	*traceback;	/* [0..max_occ-1] traceback of nets that use this node */

	float current_cost;		/* Current cost of using this node */
	float historical_cost;	/* Historical cost of using this node */

	t_lb_type_rr_node *lb_type_rr_node; /* corresponding lb_type_rr_node */
} t_lb_rr_node;

