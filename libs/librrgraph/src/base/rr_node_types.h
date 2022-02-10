#ifndef RR_NODE_TYPES_H
#define RR_NODE_TYPES_H

/**
 * @brief Type of a routing resource node.
 *
 * x-directed channel segment, y-directed channel segment,
 * input pin to a clb to pad, output from a clb or pad
 * (i.e. output pin of a net) and:
 * - SOURCE
 * - SINK
 */
typedef enum e_rr_type : unsigned char {
    SOURCE = 0, ///<A dummy node that is a logical output within a block -- i.e., the gate that generates a signal.
    SINK,       ///<A dummy node that is a logical input within a block -- i.e. the gate that needs a signal.
    IPIN,
    OPIN,
    CHANX,
    CHANY,
    NUM_RR_TYPES
} t_rr_type;

constexpr std::array<t_rr_type, NUM_RR_TYPES> RR_TYPES = {{SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}};
constexpr std::array<const char*, NUM_RR_TYPES> rr_node_typename{{"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY"}};

#endif
