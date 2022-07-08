#ifndef RR_NODE_IMPL_H
#define RR_NODE_IMPL_H
// This file provides the inline proxy implemenation for t_rr_node.
// See the t_rr_node class comment for additional details.

#include "rr_node_types.h"
#include "rr_node.h"
#include "rr_graph_storage.h"

class node_idx_iterator : public std::iterator<std::bidirectional_iterator_tag, const t_rr_node> {
  public:
    node_idx_iterator(t_rr_node value)
        : value_(value) {}

    iterator operator++() {
        value_.next_node();
        return *this;
    }
    iterator operator--() {
        value_.prev_node();
        return *this;
    }
    reference operator*() const { return value_; }
    pointer operator->() const { return &value_; }

    friend bool operator==(const node_idx_iterator lhs, const node_idx_iterator rhs) { return lhs.value_.id() == rhs.value_.id(); }
    friend bool operator!=(const node_idx_iterator lhs, const node_idx_iterator rhs) { return !(lhs == rhs); }

  private:
    t_rr_node value_;
};

inline node_idx_iterator t_rr_graph_storage::begin() const {
    return node_idx_iterator(t_rr_node(const_cast<t_rr_graph_storage*>(this), RRNodeId(0)));
}

inline node_idx_iterator t_rr_graph_storage::end() const {
    return node_idx_iterator(t_rr_node(const_cast<t_rr_graph_storage*>(this), RRNodeId(size())));
}

inline const t_rr_node t_rr_graph_storage::operator[](size_t idx) const {
    return t_rr_node(const_cast<t_rr_graph_storage*>(this), RRNodeId(idx));
}

inline t_rr_node t_rr_graph_storage::operator[](size_t idx) {
    return t_rr_node(this, RRNodeId(idx));
}

inline const t_rr_node t_rr_graph_storage::at(size_t idx) const {
    VTR_ASSERT(idx < node_storage_.size());
    return t_rr_node(const_cast<t_rr_graph_storage*>(this), RRNodeId(idx));
}

inline t_rr_node t_rr_graph_storage::at(size_t idx) {
    VTR_ASSERT(idx < node_storage_.size());
    return t_rr_node(this, RRNodeId(idx));
}

inline const t_rr_node t_rr_graph_storage::front() const {
    return t_rr_node(const_cast<t_rr_graph_storage*>(this), RRNodeId(0));
}
inline t_rr_node t_rr_graph_storage::front() {
    return t_rr_node(this, RRNodeId(0));
}

inline const t_rr_node t_rr_graph_storage::back() const {
    return t_rr_node(const_cast<t_rr_graph_storage*>(this), RRNodeId(size() - 1));
}
inline t_rr_node t_rr_graph_storage::back() {
    return t_rr_node(this, RRNodeId(size() - 1));
}

inline t_edge_size t_rr_node::num_edges() const {
    return storage_->num_edges(id_);
}

inline edge_idx_range t_rr_node::edges() const {
    return storage_->edges(id_);
}

#endif /* _RR_NODE_IMPL_H_ */
