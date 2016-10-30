#pragma once
/*
 * Forward declarations for Timing Graph and related types
 */
#include <iosfwd>
#include "tatum_strong_id.hpp"

//The timing graph
class TimingGraph;

//Potential node types in the timing graph
enum class TN_Type;

//Various IDs used by the timing graph

struct node_id_tag;
struct block_id_tag;
struct edge_id_tag;
struct domain_id_tag;
struct level_id_tag;

typedef tatum::StrongId<node_id_tag> NodeId;
typedef tatum::StrongId<block_id_tag> BlockId;
typedef tatum::StrongId<edge_id_tag> EdgeId;
typedef tatum::StrongId<domain_id_tag> DomainId;
typedef tatum::StrongId<level_id_tag> LevelId;

#define INVALID_CLOCK_DOMAIN -1

std::ostream& operator<<(std::ostream& os, NodeId node_id);
std::ostream& operator<<(std::ostream& os, BlockId block_id);
std::ostream& operator<<(std::ostream& os, EdgeId edge_id);
std::ostream& operator<<(std::ostream& os, DomainId domain_id);
std::ostream& operator<<(std::ostream& os, LevelId level_id);
