#ifndef RR_GRAPH_NODE_TYPES_H
#define RR_GRAPH_NODE_TYPES_H
/**
 * Header file for critical types of nodes in rr_graph
 */

/* Uncomment lines below to save some memory, at the cost of debugging ease. */
/*enum e_rr_type {SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}; */
/* typedef short t_rr_type */

/* Type of a routing resource node.  x-directed channel segment,   *
 * y-directed channel segment, input pin to a clb to pad, output   *
 * from a clb or pad (i.e. output pin of a net) and:               *
 * SOURCE:  A dummy node that is a logical output within a block   *
 *          -- i.e., the gate that generates a signal.             *
 * SINK:    A dummy node that is a logical input within a block    *
 *          -- i.e. the gate that needs a signal.                  */
typedef enum e_rr_graph_node_type : unsigned char {
	SOURCE = 0, SINK, IPIN, OPIN, CHANX, CHANY, INTRA_CLUSTER_EDGE, NUM_RR_TYPES
} t_rr_graph_node_type;

constexpr std::array<const char*, NUM_RR_TYPES> rr_graph_node_typename { {
	"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY", "INTRA_CLUSTER_EDGE"
} };

/* Directionality of a Routing Resource Node (rr_node)
 * Directionality is only applicable to rr_node whose type is 
 * CHANX or CHANY in e_rr_type
 * i.e., routing tracks
 * 1. INC_DIRECTION or DEC_DIRECTION
 *    applicable to rr_node in uni-directional routing architectures:
 *    INC_DIRECTION:
 *    (a) a CHANX goes from left to right
 *    (b) a CHANY goes from bottom to top 
 *    DEC_DIRECTION:
 *    (a) a CHANX goes from right to left
 *    (b) a CHANY goes from top to bottom
 * 2. BI_DIRECTION: 
 *    applicable to rr_node in bi-directional routing architectures
 * 3. NO_DIRECTION:
 *    FIXME
 * 4. NUM_DIRECTIONS:
 *    A quick counter for directionality types
 */
enum e_rr_graph_node_direction : unsigned char {
	INC_DIRECTION = 0,
    DEC_DIRECTION = 1,
    BI_DIRECTION = 2,
    NO_DIRECTION = 3,
    NUM_DIRECTIONS
};

#endif
