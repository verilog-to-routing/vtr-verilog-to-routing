/**
 * Jason Luu
 * July 22, 2013
 *
 * Defines core data structures used in packing
 */

#ifndef PACK_TYPES_H
#define PACK_TYPES_H

#include "arch_types.h"
#include <map>
#include <vector>


/**************************************************************************
* Packing Algorithm Enumerations
***************************************************************************/


/* Describes different types of intra-logic block routing resource nodes */
enum e_lb_rr_type {
	LB_SOURCE = 0, LB_SINK, LB_INTERMEDIATE, NUM_LB_RR_TYPES
};
extern const char* lb_rr_type_str[];


/**************************************************************************
* Packing Algorithm Data Structures
***************************************************************************/

/* Stores statistical information for a physical block such as costs and usages */
typedef struct s_pb_stats {
	/* Packing statistics */
	std::map<int, float> gain; /* Attraction (inverse of cost) function */

	std::map<int, float> timinggain; /* [0..num_logical_blocks-1]. The timing criticality score of this logical_block. 
	 Determined by the most critical g_atoms_nlist.net between this logical_block and any logical_block in the current pb */
	std::map<int, float> connectiongain; /* [0..num_logical_blocks-1] Weighted sum of connections to attraction function */
	std::map<int, float> prevconnectiongainincr; /* [0..num_logical_blocks-1] Prev sum to weighted sum of connections to attraction function */
	std::map<int, float> sharinggain; /* [0..num_logical_blocks-1]. How many nets on this logical_block are already in the pb under consideration */

	/* [0..num_logical_blocks-1]. This is the gain used for hill-climbing. It stores*
	 * the reduction in the number of pins that adding this logical_block to the the*
	 * current pb will have. This reflects the fact that sometimes the *
	 * addition of a logical_block to a pb may reduce the number of inputs     *
	 * required if it shares inputs with all other BLEs and it's output is  *
	 * used by all other child pbs in this parent pb.                               */
	std::map<int, float> hillgain;

	/* [0..num_marked_nets] and [0..num_marked_blocks] respectively.  List  *
	 * the indices of the nets and blocks that have had their num_pins_of_  *
	 * net_in_pb and gain entries altered.                             */
	int *marked_nets, *marked_blocks;
	int num_marked_nets, num_marked_blocks;
	int num_child_blocks_in_pb;

	int tie_break_high_fanout_net; /* If no marked candidate atoms, use this high fanout net to determine the next candidate atom */

	/* [0..g_atoms_nlist.net.size()-1].  How many pins of each g_atoms_nlist.net are contained in the *
	 * currently open pb?                                          */
	std::map<int, int> num_pins_of_net_in_pb;

	/* Record of pins of class used TODO: Jason Luu: Should really be using hash table for this for speed, too lazy to write one now, performance isn't too bad since I'm at most iterating over the number of pins of a pb which is effectively a constant for reasonable architectures */
	int **input_pins_used; /* [0..pb_graph_node->num_pin_classes-1][0..pin_class_size] number of input pins of this class that are used */
	int **output_pins_used; /* [0..pb_graph_node->num_pin_classes-1][0..pin_class_size] number of output pins of this class that are used */

	int **lookahead_input_pins_used; /* [0..pb_graph_node->num_pin_classes-1][0..pin_class_size] number of input pins of this class that are speculatively used */
	int **lookahead_output_pins_used; /* [0..pb_graph_node->num_pin_classes-1][0..pin_class_size] number of output pins of this class that are speculatively used */

	/* Array of feasible blocks to select from [0..max_array_size-1] 
	 Sorted in ascending gain order so that the last block is the most desirable (this makes it easy to pop blocks off the list
	 */
	struct s_pack_molecule **feasible_blocks;
	int num_feasible_blocks; /* [0..num_marked_models-1] */
} t_pb_stats;



/**************************************************************************
* Intra-Logic Block Routing Data Structures (by type)
***************************************************************************/

/* Output edges of a t_lb_type_rr_node */
struct t_lb_type_rr_node_edge {
	int node_index;
	float intrinsic_cost;
};

/* Describes a routing resource node within a logic block type */
struct t_lb_type_rr_node {
	short capacity;			/* Number of nets that can simultaneously use this node */
	short *num_fanout;		/* [0..num_modes - 1] Mode dependant fanout */
	enum e_lb_rr_type type;	/* Type of logic block resource node */	

	t_lb_type_rr_node_edge **outedges;						/* [0..num_modes - 1][0..num_fanout-1] index and cost of out edges */

	struct s_pb_graph_pin *pb_graph_pin;	/* pb_graph_pin associated with this lb_rr_node if exists, NULL otherwise */
	float intrinsic_cost;					/* cost of this node */
	
	t_lb_type_rr_node() {
		capacity = 0;
		num_fanout = NULL;
		type = NUM_LB_RR_TYPES;
		outedges = NULL;
		pb_graph_pin = NULL;
		intrinsic_cost = 0;
	}
};



/**************************************************************************
* Intra-Logic Block Routing Data Structures (by instance)
***************************************************************************/

/* A routing traceback data structure, provides a logic block instance specific trace lookup directly from the t_lb_type_rr_node array index 
   After packing, routing info for each CLB will have an array of t_lb_traceback to store routing info within the CLB
*/
struct t_lb_traceback {
	int	net;				/* net of flat, technology-mapped, netlist using this node */
	int prev_lb_rr_node;	/* index of previous node that drives current node */
	int prev_edge;			/* index of previous edge that drives current node */	
};

/**************************************************************************
* Intra-Logic Block Router Data Structures
***************************************************************************/

/* Describes the status of a logic block routing resource node for a given logic block instance */
struct t_lb_rr_node_stats {
	int occ;								/* Number of nets currently using this lb_rr_node */
	vector<t_lb_traceback> tracebacks;		/* Nets that share this node in their routetree */
	
	int mode;								/* Mode that this rr_node is set to */
	
	float current_cost;		/* Current cost of using this node */
	float historical_cost;	/* Historical cost of using this node */

	t_lb_rr_node_stats() {
		occ = 0;
		mode = 0;
		current_cost = 0;
		historical_cost = 0;
	}
};

/* 
  Data structure forming the route tree of a net within one logic block.  
  
  A net is implemented using routing resource nodes.  The t_lb_trace data structure records one of the nodes used by the net and the connections
  to other nodes
*/
struct t_lb_trace {
	int	current_node;					/* current t_lb_type_rr_node used by net */
	int num_fanout;						/* number of other nodes used by net that are directly driven by the current node*/
	vector<t_lb_trace> next_nodes;		/* index of previous edge that drives current node */	
};

/* Represents a net used inside a logic block and the physical nodes used by the net */
struct t_intra_lb_net {
	int atom_net_index;					/* index of atomic net this intra_lb_net represents */
	vector<int> terminals;				/* endpoints of the intra_lb_net, 0th position is the source, all others are sinks */
	t_lb_trace *rt_tree;				/* Route tree head */
	t_lb_trace *saved_rt_tree;			/* Saved route tree head */
	
	t_intra_lb_net() {
		atom_net_index = OPEN;
		rt_tree = NULL;
		saved_rt_tree = NULL;
	}
};

/* Stores all data needed by intra-logic block router */
struct t_lb_router_data {
	/* Physical Architecture Info */
	vector<t_lb_type_rr_node> *lb_type_graph;	/* Pointer to physical intra-logic block type rr graph */
	
	/* Logical Netlist Info */
	vector <t_intra_lb_net> *intra_lb_nets;		/* Pointer to vector of intra logic block nets and their connections */

	/* Logical-to-physical mapping info */
	t_lb_rr_node_stats *lb_rr_node_stats;		/* [0..lb_type_graph->size()-1] Stats for each logic block instance */
	boolean is_routed;							/* Stores whether or not the current logical-to-physical mapping has a routed solution */

	/* Current type */
	t_type_ptr lb_type;

	t_lb_router_data() {
		lb_type_graph = NULL;	
		lb_rr_node_stats = NULL;	
		intra_lb_nets = NULL;
		is_routed = FALSE;
		lb_type = NULL;
	}
};

#endif
