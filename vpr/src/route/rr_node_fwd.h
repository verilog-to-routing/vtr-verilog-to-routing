#ifndef RR_NODE_FWD_H
#define RR_NODE_FWD_H

#include <iterator>
#include "vpr_types.h"
#include "vtr_strong_id.h"

//Forward declaration
class t_rr_node;
class t_rr_graph_storage;
class node_idx_iterator;

/*
 * StrongId's for the t_rr_node class
 */

//Type tags for Ids
struct rr_node_id_tag;
struct rr_edge_id_tag;

//A unique identifier for a node in the rr graph
typedef vtr::StrongId<rr_node_id_tag> RRNodeId;

//A unique identifier for an edge in the rr graph
typedef vtr::StrongId<rr_edge_id_tag> RREdgeId;

//An iterator that dereferences to an edge index
//
//Used inconjunction with vtr::Range to return ranges of edge indices
class edge_idx_iterator : public std::iterator<std::bidirectional_iterator_tag, t_edge_size> {
  public:
    edge_idx_iterator(value_type init)
        : value_(init) {}
    iterator operator++() {
        value_ += 1;
        return *this;
    }
    iterator operator--() {
        value_ -= 1;
        return *this;
    }
    reference operator*() { return value_; }
    pointer operator->() { return &value_; }

    friend bool operator==(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return lhs.value_ == rhs.value_; }
    friend bool operator!=(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return !(lhs == rhs); }

  private:
    value_type value_;
};

typedef vtr::Range<edge_idx_iterator> edge_idx_range;

#endif
