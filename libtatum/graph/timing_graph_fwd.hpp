#pragma once
/*
 * Forward declarations for Timing Graph and related types
 */
#include <iosfwd>
#include "tatum_strong_id.hpp"

namespace tatum {

//The timing graph
class TimingGraph;

/**
 * Potential types for nodes in the timing graph
 */
enum class NodeType {
	INPAD_SOURCE, //Driver of an input I/O pad
	INPAD_OPIN, //Output pin of an input I/O pad
	OUTPAD_IPIN, //Input pin of an output I/O pad
	OUTPAD_SINK, //Sink of an output I/O pad
	PRIMITIVE_IPIN, //Input pin to a primitive (e.g. LUT)
	PRIMITIVE_OPIN, //Output pin from a primitive (e.g. LUT)
	FF_IPIN, //Input pin to a flip-flop - goes to FF_SINK
	FF_OPIN, //Output pin from a flip-flop - comes from FF_SOURCE
	FF_SINK, //Sink (D) pin of flip-flop
	FF_SOURCE, //Source (Q) pin of flip-flop
	FF_CLOCK, //Clock pin of flip-flop
    CLOCK_SOURCE, //A clock generator such as a PLL
    CLOCK_OPIN, //Output pin from an on-chip clock source - comes from CLOCK_SOURCE
	CONSTANT_GEN_SOURCE, //Source of a constant logic 1 or 0
    SOURCE,
    SINK,
    CLOCK_SINK,
    IPIN,
    OPIN,
    UNKOWN //Unrecognized type, if encountered this is almost certainly an error
};

//Stream operators for NodeType
std::ostream& operator<<(std::ostream& os, const NodeType type);
std::istream& operator>>(std::istream& os, NodeType& type);

//Various IDs used by the timing graph

struct node_id_tag;
struct edge_id_tag;
struct domain_id_tag;
struct level_id_tag;

typedef tatum::util::StrongId<node_id_tag> NodeId;
typedef tatum::util::StrongId<edge_id_tag> EdgeId;
typedef tatum::util::StrongId<domain_id_tag> DomainId;
typedef tatum::util::StrongId<level_id_tag> LevelId;

std::ostream& operator<<(std::ostream& os, NodeId node_id);
std::ostream& operator<<(std::ostream& os, EdgeId edge_id);
std::ostream& operator<<(std::ostream& os, DomainId domain_id);
std::ostream& operator<<(std::ostream& os, LevelId level_id);

} //namepsace
