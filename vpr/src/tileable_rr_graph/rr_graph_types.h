#ifndef RR_GRAPH_TYPES_H
#define RR_GRAPH_TYPES_H

/********************************************************************
 * Data types required by routing resource graph (RRGraph) definition
 *******************************************************************/

/********************************************************************
 * Directionality of a routing track (node type CHANX and CHANY) in
 * a routing resource graph
 *******************************************************************/
enum e_direction : unsigned char {
    INC_DIRECTION = 0,
    DEC_DIRECTION = 1,
    BI_DIRECTION = 2,
    NO_DIRECTION = 3,
    NUM_DIRECTIONS
};

/* Xifan Tang - string used in describe_rr_node() and write_xml_rr_graph_obj() */
constexpr std::array<const char*, NUM_DIRECTIONS> DIRECTION_STRING_WRITE_XML = {{"INC_DIR", "DEC_DIR", "BI_DIR", "NO_DIR"}};

#if 0
/* Type of a routing resource node.  x-directed channel segment,   *
 * y-directed channel segment, input pin to a clb to pad, output   *
 * from a clb or pad (i.e. output pin of a net) and:               *
 * SOURCE:  A dummy node that is a logical output within a block   *
 *          -- i.e., the gate that generates a signal.             *
 * SINK:    A dummy node that is a logical input within a block    *
 *          -- i.e. the gate that needs a signal.                  */
typedef enum e_rr_type : unsigned char {
    SOURCE = 0,
    SINK,
    IPIN,
    OPIN,
    CHANX,
    CHANY,
    NUM_RR_TYPES
} t_rr_type;
#endif

// constexpr std::array<t_rr_type, NUM_RR_TYPES> RR_TYPES = {{SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}};
// constexpr std::array<const char*, NUM_RR_TYPES> rr_node_typename{{"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY"}};

#endif
