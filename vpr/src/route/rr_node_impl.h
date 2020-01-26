#ifndef _RR_NODE_IMPL_H_
#define _RR_NODE_IMPL_H_

#include "rr_node.h"
#include "rr_node_storage.h"

#include "vpr_error.h"

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

inline node_idx_iterator t_rr_node_storage::begin() const {
    return node_idx_iterator(t_rr_node(const_cast<t_rr_node_storage*>(this), RRNodeId(0)));
}

inline node_idx_iterator t_rr_node_storage::end() const {
    return node_idx_iterator(t_rr_node(const_cast<t_rr_node_storage*>(this), RRNodeId(size())));
}

inline const t_rr_node t_rr_node_storage::operator[](size_t idx) const {
    return t_rr_node(const_cast<t_rr_node_storage*>(this), RRNodeId(idx));
}

inline t_rr_node t_rr_node_storage::operator[](size_t idx) {
    return t_rr_node(this, RRNodeId(idx));
}

inline const t_rr_node t_rr_node_storage::at(size_t idx) const {
    VTR_ASSERT(idx < storage_.size());
    return t_rr_node(const_cast<t_rr_node_storage*>(this), RRNodeId(idx));
}

inline t_rr_node t_rr_node_storage::at(size_t idx) {
    VTR_ASSERT(idx < storage_.size());
    return t_rr_node(this, RRNodeId(idx));
}

inline const t_rr_node t_rr_node_storage::front() const {
    return t_rr_node(const_cast<t_rr_node_storage*>(this), RRNodeId(0));
}
inline t_rr_node t_rr_node_storage::front() {
    return t_rr_node(this, RRNodeId(0));
}

inline const t_rr_node t_rr_node_storage::back() const {
    return t_rr_node(const_cast<t_rr_node_storage*>(this), RRNodeId(size() - 1));
}
inline t_rr_node t_rr_node_storage::back() {
    return t_rr_node(this, RRNodeId(size() - 1));
}

inline t_rr_type t_rr_node::type() const {
    return storage_->get(id_).type_;
}

inline t_edge_size t_rr_node::num_edges() const {
    return storage_->num_edges(id_);
}

inline edge_idx_range t_rr_node::edges() const {
    return storage_->edges(id_);
}

inline edge_idx_range t_rr_node::configurable_edges() const {
    return storage_->configurable_edges(id_);
}
inline edge_idx_range t_rr_node::non_configurable_edges() const {
    return storage_->non_configurable_edges(id_);
}

inline t_edge_size t_rr_node::num_non_configurable_edges() const {
    return storage_->num_non_configurable_edges(id_);
}

inline t_edge_size t_rr_node::num_configurable_edges() const {
    return storage_->num_configurable_edges(id_);
}

inline int t_rr_node::edge_sink_node(t_edge_size iedge) const {
    size_t inode = (size_t)storage_->edge_sink_node(id_, iedge);
    return inode;
}
inline short t_rr_node::edge_switch(t_edge_size iedge) const {
    return storage_->edge_switch(id_, iedge);
}

inline t_edge_size t_rr_node::fan_in() const {
    return storage_->fan_in(id_);
}

inline short t_rr_node::xlow() const {
    return storage_->get(id_).xlow_;
}
inline short t_rr_node::ylow() const {
    return storage_->get(id_).ylow_;
}
inline short t_rr_node::xhigh() const {
    return storage_->get(id_).xhigh_;
}
inline short t_rr_node::yhigh() const {
    return storage_->get(id_).yhigh_;
}

inline short t_rr_node::capacity() const {
    return storage_->get(id_).capacity_;
}

inline short t_rr_node::ptc_num() const {
    return storage_->get(id_).ptc_.pin_num;
}

inline short t_rr_node::pin_num() const {
    if (type() != IPIN && type() != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'pin_num' for non-IPIN/OPIN type '%s'", type_string());
    }
    return storage_->get(id_).ptc_.pin_num;
}

inline short t_rr_node::track_num() const {
    if (type() != CHANX && type() != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'track_num' for non-CHANX/CHANY type '%s'", type_string());
    }
    return storage_->get(id_).ptc_.track_num;
}

inline short t_rr_node::class_num() const {
    if (type() != SOURCE && type() != SINK) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'class_num' for non-SOURCE/SINK type '%s'", type_string());
    }
    return storage_->get(id_).ptc_.class_num;
}

inline short t_rr_node::cost_index() const {
    return storage_->get(id_).cost_index_;
}

inline short t_rr_node::rc_index() const {
    return storage_->get(id_).rc_index_;
}

inline e_direction t_rr_node::direction() const {
    if (type() != CHANX && type() != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'direction' for non-channel type '%s'", type_string());
    }
    return storage_->get(id_).dir_side_.direction;
}

inline e_side t_rr_node::side() const {
    if (type() != IPIN && type() != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'side' for non-IPIN/OPIN type '%s'", type_string());
    }
    return storage_->get(id_).dir_side_.side;
}

#endif /* _RR_NODE_IMPL_H_ */
