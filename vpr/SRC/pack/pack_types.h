/**
 * Jason Luu
 * July 22, 2013
 *
 * Defines core data structures used in packing
 */

#ifndef PACK_TYPES_H
#define PACK_TYPES_H

#include "arch_types.h"


/**************************************************************************
* Packing Algorithm Enumerations
***************************************************************************/


/* Describes different types of intra-logic block routing resource nodes */
enum e_lb_rr_type {
	LB_SOURCE = 0, LB_SINK, LB_INTERMEDIATE, NUM_LB_RR_TYPES
};


/**************************************************************************
* Packing Algorithm Data Structures
***************************************************************************/

/* Stores statistical information for a physical block such as costs and usages */
typedef struct s_pb_stats {
	/* Packing statistics */
	std::map<int, float> gain; /* Attraction (inverse of cost) function */

	std::map<int, float> timinggain; /* [0..num_logical_blocks-1]. The timing criticality score of this logical_block. 
	 Determined by the most critical vpack_net between this logical_block and any logical_block in the current pb */
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

	/* [0..num_logical_nets-1].  How many pins of each vpack_net are contained in the *
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
* Intra-Logic Block Routing Data Structures
***************************************************************************/


/* Describes a routing resource node within a logic block type */
typedef struct s_lb_type_rr_node {
	short capacity;			/* Number of nets that can simultaneously use this node */
	short *num_fanout;		/* [0..num_modes - 1] Mode dependant fanout */
	enum e_lb_rr_type type;	/* Type of logic block resource node */	

	int **fanout;					/* [0..num_modes - 1][0..num_fanout-1] index of fanout lb_rr_node */
	float **fanout_intrinsic_cost;	/* [0..num_modes - 1][0..num_fanout-1] cost of fanout lb_rr_node */

	struct s_pb_graph_pin *pb_graph_pin;	/* pb_graph_pin associated with this lb_rr_node if exists */
	float pack_intrinsic_cost;		/* cost of this node */
	
	s_lb_type_rr_node() {
		capacity = 0;
		num_fanout = NULL;
		type = NUM_LB_RR_TYPES;
		fanout = NULL;
		fanout_intrinsic_cost = NULL;
		pb_graph_pin = NULL;
		pack_intrinsic_cost = 0;
	}
} t_lb_type_rr_node;


/* The routing traceback for a net */
typedef struct s_lb_traceback {
	int	net;				/* net using this node */
	int prev_edge;			/* index of previous edge that drives current node */
	int prev_lb_rr_node;	/* index of previous node that drives current node */
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


	/* Default values */
	s_lb_rr_node() {
		occ = 0;
		max_occ = 0;
		mode = 0;
		traceback = NULL;
		current_cost = 0;		
		historical_cost = 0;	
		lb_type_rr_node = NULL; 
	}
} t_lb_rr_node;

#endif
