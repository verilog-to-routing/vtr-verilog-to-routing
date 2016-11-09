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
    SOURCE, //The start of a clock/data path
    SINK, //The end of a clock/data path
    IPIN, //An intermediate input pin
    OPIN //An intermediate output pin
};

//Stream operators for NodeType
std::ostream& operator<<(std::ostream& os, const NodeType type);

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
