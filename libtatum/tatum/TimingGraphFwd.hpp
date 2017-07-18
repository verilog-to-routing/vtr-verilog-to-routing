#pragma once
/*
 * Forward declarations for Timing Graph and related types
 */
#include <iosfwd>
#include "tatum/util/tatum_strong_id.hpp"

namespace tatum {

//The timing graph
class TimingGraph;

struct GraphIdMaps;

/**
 * Potential types for nodes in the timing graph
 */
enum class NodeType : unsigned char {
    SOURCE, //The start of a clock/data path
    SINK, //The end of a clock/data path
    IPIN, //An intermediate input pin
    OPIN, //An intermediate output pin
    CPIN, //An intermediate clock (input) pin
};

//Stream operators for NodeType

enum class EdgeType : unsigned char {
    PRIMITIVE_COMBINATIONAL,
    PRIMITIVE_CLOCK_LAUNCH,
    PRIMITIVE_CLOCK_CAPTURE,
    INTERCONNECT
};

//Stream operators for Edge/Node Type
std::ostream& operator<<(std::ostream& os, const NodeType type);
std::ostream& operator<<(std::ostream& os, const EdgeType type);

//Various IDs used by the timing graph

struct node_id_tag;
struct edge_id_tag;
struct domain_id_tag;
struct level_id_tag;

typedef tatum::util::StrongId<node_id_tag> NodeId;
typedef tatum::util::StrongId<edge_id_tag> EdgeId;
typedef tatum::util::StrongId<level_id_tag> LevelId;

//We expect far fewer domains than nodes/edges so we use a smaller 
//data type, as this allows for more efficient packing in TimingTag
typedef tatum::util::StrongId<domain_id_tag,unsigned short> DomainId;

std::ostream& operator<<(std::ostream& os, NodeId node_id);
std::ostream& operator<<(std::ostream& os, EdgeId edge_id);
std::ostream& operator<<(std::ostream& os, DomainId domain_id);
std::ostream& operator<<(std::ostream& os, LevelId level_id);

} //namepsace
