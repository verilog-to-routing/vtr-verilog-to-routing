#ifndef RR_NODE_FWD_H
#define RR_NODE_FWD_H

#include "vtr_strong_id.h"

/*
 * StrongId's for the t_rr_node class
 */

//Forward declaration
class t_rr_node;
class t_rr_node_storage;
class node_idx_iterator;

//Type tags for Ids
struct rr_node_id_tag;

//A unique identifier for a node in the rr graph
typedef vtr::StrongId<rr_node_id_tag> RRNodeId;

#endif
