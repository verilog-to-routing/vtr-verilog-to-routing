#include "rr_graph_storage.h"

#include <algorithm>

#include "globals.h"

void t_rr_graph_storage::reserve_edges(size_t num_edges) {
    edge_src_node_.reserve(num_edges);
    edge_dest_node_.reserve(num_edges);
    edge_switch_.reserve(num_edges);
}

void t_rr_graph_storage::emplace_back_edge(RRNodeId src, RRNodeId dest, short edge_switch) {
    // Cannot mutate edges once edges have been read!
    VTR_ASSERT(!edges_read_);
    edge_src_node_.emplace_back(src);
    edge_dest_node_.emplace_back(dest);
    edge_switch_.emplace_back(edge_switch);
}

// Typical node to edge ratio.  This allows a preallocation guess for the edges
// to avoid repeated reallocation.
constexpr size_t kEdgeToNodeRatio = 10;

void t_rr_graph_storage::alloc_and_load_edges(const t_rr_edge_info_set* rr_edges_to_create) {
    // Cannot mutate edges once edges have been read!
    VTR_ASSERT(!edges_read_);

    size_t required_size = edge_src_node_.size() + rr_edges_to_create->size();
    if (edge_src_node_.capacity() < required_size) {
        size_t new_capacity = std::min(edge_src_node_.capacity(), node_storage_.size() * kEdgeToNodeRatio);
        if (new_capacity < 1) {
            new_capacity = 1;
        }
        while (new_capacity < required_size) {
            new_capacity *= 2;
        }

        edge_src_node_.reserve(new_capacity);
        edge_dest_node_.reserve(new_capacity);
        edge_switch_.reserve(new_capacity);
    }

    for (const auto& new_edge : *rr_edges_to_create) {
        emplace_back_edge(
            RRNodeId(new_edge.from_node),
            RRNodeId(new_edge.to_node),
            new_edge.switch_type);
    }
}

/* edge_swapper / edge_sort_iterator / edge_compare_src_node_and_configurable_first
 * are used to sort the edge data arrays
 * edge_src_node_ / edge_dest_node_ / edge_switch_.
 *
 * edge_sort_iterator is a random access iterator for the edge data arrays.
 *
 * edge_swapper is a reference for the src/dest/switch tuple, and can convert
 * to and from t_rr_edge_info, the value_type for edge_sort_iterator.
 *
 * edge_compare_src_node_and_configurable_first is a comparision operator
 * that first partitions the edge data by source rr node, and then by
 * configurable switches first.  Sorting by this comparision operator means that
 * the edge data is directly usable for each node by simply slicing the arrays.
 *
 * */
struct edge_swapper {
    edge_swapper(t_rr_graph_storage* storage, size_t idx)
        : storage_(storage)
        , idx_(idx) {}
    t_rr_graph_storage* storage_;
    size_t idx_;

    edge_swapper(const edge_swapper&) = delete;
    edge_swapper& operator=(const edge_swapper& other) {
        VTR_ASSERT(idx_ < storage_->edge_src_node_.size());
        VTR_ASSERT(other.idx_ < storage_->edge_src_node_.size());

        RREdgeId edge(idx_);
        RREdgeId other_edge(other.idx_);
        storage_->edge_src_node_[edge] = storage_->edge_src_node_[other_edge];
        storage_->edge_dest_node_[edge] = storage_->edge_dest_node_[other_edge];
        storage_->edge_switch_[edge] = storage_->edge_switch_[other_edge];
        return *this;
    }

    edge_swapper& operator=(const t_rr_edge_info& edge) {
        VTR_ASSERT(idx_ < storage_->edge_src_node_.size());

        storage_->edge_src_node_[RREdgeId(idx_)] = RRNodeId(edge.from_node);
        storage_->edge_dest_node_[RREdgeId(idx_)] = RRNodeId(edge.to_node);
        storage_->edge_switch_[RREdgeId(idx_)] = edge.switch_type;
        return *this;
    }

    operator t_rr_edge_info() const {
        VTR_ASSERT(idx_ < storage_->edge_src_node_.size());
        t_rr_edge_info info(
            size_t(storage_->edge_src_node_[RREdgeId(idx_)]),
            size_t(storage_->edge_dest_node_[RREdgeId(idx_)]),
            storage_->edge_switch_[RREdgeId(idx_)]);

        return info;
    }

    friend class edge_compare;

    static void swap(edge_swapper& a, edge_swapper& b) {
        VTR_ASSERT(a.idx_ < a.storage_->edge_src_node_.size());
        VTR_ASSERT(b.idx_ < a.storage_->edge_src_node_.size());
        RREdgeId a_edge(a.idx_);
        RREdgeId b_edge(b.idx_);

        std::swap(a.storage_->edge_src_node_[a_edge], a.storage_->edge_src_node_[b_edge]);
        std::swap(a.storage_->edge_dest_node_[a_edge], a.storage_->edge_dest_node_[b_edge]);
        std::swap(a.storage_->edge_switch_[a_edge], a.storage_->edge_switch_[b_edge]);
    }

    friend void swap(edge_swapper& a, edge_swapper& b) {
        edge_swapper::swap(a, b);
    }
};

class edge_sort_iterator {
  public:
    edge_sort_iterator()
        : swapper_(nullptr, 0) {}
    edge_sort_iterator(t_rr_graph_storage* storage, size_t idx)
        : swapper_(storage, idx) {}

    edge_sort_iterator(const edge_sort_iterator& other)
        : swapper_(
              other.swapper_.storage_,
              other.swapper_.idx_) {
    }

    edge_sort_iterator& operator=(const edge_sort_iterator& other) {
        swapper_.storage_ = other.swapper_.storage_;
        swapper_.idx_ = other.swapper_.idx_;

        return *this;
    }

    using iterator_category = std::random_access_iterator_tag;
    using value_type = t_rr_edge_info;
    using reference = edge_swapper&;
    using pointer = edge_swapper*;
    using difference_type = ssize_t;

    edge_swapper& operator*() {
        return this->swapper_;
    }

    edge_swapper* operator->() {
        return &this->swapper_;
    }

    edge_sort_iterator& operator+=(ssize_t n) {
        swapper_.idx_ += n;
        return *this;
    }

    edge_sort_iterator& operator++() {
        ++swapper_.idx_;
        return *this;
    }

    edge_sort_iterator& operator--() {
        --swapper_.idx_;
        return *this;
    }

    friend edge_sort_iterator operator+(const edge_sort_iterator& lhs, ssize_t n) {
        edge_sort_iterator ret = lhs;
        ret.swapper_.idx_ += n;
        return ret;
    }

    friend edge_sort_iterator operator-(const edge_sort_iterator& lhs, ssize_t n) {
        edge_sort_iterator ret = lhs;
        ret.swapper_.idx_ -= n;
        return ret;
    }

    friend ssize_t operator-(const edge_sort_iterator& lhs, const edge_sort_iterator& rhs) {
        ssize_t diff = lhs.swapper_.idx_;
        diff -= rhs.swapper_.idx_;
        return diff;
    }

    friend bool operator==(const edge_sort_iterator& lhs, const edge_sort_iterator& rhs) {
        return lhs.swapper_.idx_ == rhs.swapper_.idx_;
    }

    friend bool operator!=(const edge_sort_iterator& lhs, const edge_sort_iterator& rhs) {
        return lhs.swapper_.idx_ != rhs.swapper_.idx_;
    }

    friend bool operator<(const edge_sort_iterator& lhs, const edge_sort_iterator& rhs) {
        return lhs.swapper_.idx_ < rhs.swapper_.idx_;
    }

    RREdgeId edge() const {
        return RREdgeId(swapper_.idx_);
    }

  private:
    edge_swapper swapper_;
};

class edge_compare_src_node_and_configurable_first {
  public:
    edge_compare_src_node_and_configurable_first(const std::vector<t_rr_switch_inf>& rr_switch_inf)
        : rr_switch_inf_(rr_switch_inf) {}

    bool operator()(const t_rr_edge_info& lhs, const edge_swapper& rhs) {
        auto lhs_src_node = RRNodeId(lhs.from_node);
        auto lhs_dest_node = RRNodeId(lhs.to_node);
        auto lhs_is_configurable = rr_switch_inf_[lhs.switch_type].configurable();

        auto rhs_edge = RREdgeId(rhs.idx_);
        auto rhs_src_node = rhs.storage_->edge_src_node_[rhs_edge];
        auto rhs_dest_node = rhs.storage_->edge_dest_node_[rhs_edge];
        auto rhs_is_configurable = rr_switch_inf_[rhs.storage_->edge_switch_[rhs_edge]].configurable();

        return std::make_tuple(lhs_src_node, !lhs_is_configurable, lhs_dest_node, lhs.switch_type) < std::make_tuple(rhs_src_node, !rhs_is_configurable, rhs_dest_node, rhs.storage_->edge_switch_[rhs_edge]);
    }

    bool operator()(const t_rr_edge_info& lhs, const t_rr_edge_info& rhs) {
        auto lhs_src_node = lhs.from_node;
        auto lhs_dest_node = lhs.to_node;
        auto lhs_is_configurable = rr_switch_inf_[lhs.switch_type].configurable();

        auto rhs_src_node = rhs.from_node;
        auto rhs_dest_node = rhs.to_node;
        auto rhs_is_configurable = rr_switch_inf_[rhs.switch_type].configurable();

        return std::make_tuple(lhs_src_node, !lhs_is_configurable, lhs_dest_node, lhs.switch_type) < std::make_tuple(rhs_src_node, !rhs_is_configurable, rhs_dest_node, rhs.switch_type);
    }
    bool operator()(const edge_swapper& lhs, const t_rr_edge_info& rhs) {
        auto lhs_edge = RREdgeId(lhs.idx_);
        auto lhs_src_node = lhs.storage_->edge_src_node_[lhs_edge];
        auto lhs_dest_node = lhs.storage_->edge_dest_node_[lhs_edge];
        auto lhs_is_configurable = rr_switch_inf_[lhs.storage_->edge_switch_[lhs_edge]].configurable();

        auto rhs_src_node = RRNodeId(rhs.from_node);
        auto rhs_dest_node = RRNodeId(rhs.to_node);
        auto rhs_is_configurable = rr_switch_inf_[rhs.switch_type].configurable();

        return std::make_tuple(lhs_src_node, !lhs_is_configurable, lhs_dest_node, lhs.storage_->edge_switch_[lhs_edge]) < std::make_tuple(rhs_src_node, !rhs_is_configurable, rhs_dest_node, rhs.switch_type);
    }
    bool operator()(const edge_swapper& lhs, const edge_swapper& rhs) {
        auto lhs_edge = RREdgeId(lhs.idx_);
        auto lhs_src_node = lhs.storage_->edge_src_node_[lhs_edge];
        auto lhs_dest_node = lhs.storage_->edge_dest_node_[lhs_edge];
        auto lhs_is_configurable = rr_switch_inf_[lhs.storage_->edge_switch_[lhs_edge]].configurable();

        auto rhs_edge = RREdgeId(rhs.idx_);
        auto rhs_src_node = rhs.storage_->edge_src_node_[rhs_edge];
        auto rhs_dest_node = rhs.storage_->edge_dest_node_[rhs_edge];
        auto rhs_is_configurable = rr_switch_inf_[rhs.storage_->edge_switch_[rhs_edge]].configurable();

        return std::make_tuple(lhs_src_node, !lhs_is_configurable, lhs_dest_node, lhs.storage_->edge_switch_[lhs_edge]) < std::make_tuple(rhs_src_node, !rhs_is_configurable, rhs_dest_node, rhs.storage_->edge_switch_[rhs_edge]);
    }

  private:
    const std::vector<t_rr_switch_inf>& rr_switch_inf_;
};

class edge_compare_dest_node {
  public:
    bool operator()(const t_rr_edge_info& lhs, const edge_swapper& rhs) {
        auto lhs_dest_node = RRNodeId(lhs.to_node);

        auto rhs_edge = RREdgeId(rhs.idx_);
        auto rhs_dest_node = rhs.storage_->edge_dest_node_[rhs_edge];

        return lhs_dest_node < rhs_dest_node;
    }

    bool operator()(const t_rr_edge_info& lhs, const t_rr_edge_info& rhs) {
        auto lhs_dest_node = lhs.to_node;

        auto rhs_dest_node = rhs.to_node;

        return lhs_dest_node < rhs_dest_node;
    }
    bool operator()(const edge_swapper& lhs, const t_rr_edge_info& rhs) {
        auto lhs_edge = RREdgeId(lhs.idx_);
        auto lhs_dest_node = lhs.storage_->edge_dest_node_[lhs_edge];

        auto rhs_dest_node = RRNodeId(rhs.to_node);

        return lhs_dest_node < rhs_dest_node;
    }
    bool operator()(const edge_swapper& lhs, const edge_swapper& rhs) {
        auto lhs_edge = RREdgeId(lhs.idx_);
        auto lhs_dest_node = lhs.storage_->edge_dest_node_[lhs_edge];

        auto rhs_edge = RREdgeId(rhs.idx_);
        auto rhs_dest_node = rhs.storage_->edge_dest_node_[rhs_edge];

        return lhs_dest_node < rhs_dest_node;
    }
};

void t_rr_graph_storage::assign_first_edges() {
    VTR_ASSERT(node_first_edge_.empty());

    // Last element is a dummy element
    node_first_edge_.resize(node_storage_.size() + 1);

    VTR_ASSERT(std::is_sorted(
        edge_src_node_.begin(),
        edge_src_node_.end()));

    size_t node_id = 0;
    size_t first_id = 0;
    size_t second_id = 0;
    size_t num_edges = edge_src_node_.size();
    VTR_ASSERT(edge_dest_node_.size() == num_edges);
    VTR_ASSERT(edge_switch_.size() == num_edges);
    while (true) {
        VTR_ASSERT(first_id < num_edges);
        VTR_ASSERT(second_id < num_edges);
        size_t current_node_id = size_t(edge_src_node_[RREdgeId(second_id)]);
        if (node_id < current_node_id) {
            // All edges belonging to node_id are assigned.
            while (node_id < current_node_id) {
                // Store any edges belongs to node_id.
                node_first_edge_[RRNodeId(node_id)] = RREdgeId(first_id);
                first_id = second_id;
                node_id += 1;
                VTR_ASSERT(node_first_edge_.size());
            }

            VTR_ASSERT(node_id == current_node_id);
            node_first_edge_[RRNodeId(node_id)] = RREdgeId(second_id);
        } else {
            second_id += 1;
            if (second_id == num_edges) {
                break;
            }
        }
    }

    // All remaining nodes have no edges, set as such.
    for (size_t inode = node_id + 1; inode < node_first_edge_.size(); ++inode) {
        node_first_edge_[RRNodeId(inode)] = RREdgeId(second_id);
    }

    VTR_ASSERT_SAFE(verify_first_edges());
}

bool t_rr_graph_storage::verify_first_edges() const {
    size_t num_edges = edge_src_node_.size();
    VTR_ASSERT(node_first_edge_[RRNodeId(node_storage_.size())] == RREdgeId(num_edges));

    // Each edge should belong with the edge range defined by
    // [node_first_edge_[src_node], node_first_edge_[src_node+1]).
    for (size_t iedge = 0; iedge < num_edges; ++iedge) {
        RRNodeId src_node = edge_src_node_.at(RREdgeId(iedge));
        RREdgeId first_edge = node_first_edge_.at(src_node);
        RREdgeId second_edge = node_first_edge_.at(RRNodeId(size_t(src_node) + 1));
        VTR_ASSERT(iedge >= size_t(first_edge));
        VTR_ASSERT(iedge < size_t(second_edge));
    }

    return true;
}

void t_rr_graph_storage::init_fan_in() {
    //Reset all fan-ins to zero
    edges_read_ = true;
    node_fan_in_.resize(node_storage_.size(), 0);
    node_fan_in_.shrink_to_fit();

    //Walk the graph and increment fanin on all downstream nodes
    for (const auto& dest_node : edge_dest_node_) {
        node_fan_in_[dest_node] += 1;
    }
}

size_t t_rr_graph_storage::count_rr_switches(
    size_t num_arch_switches,
    t_arch_switch_inf* arch_switch_inf,
    t_arch_switch_fanin& arch_switch_fanins) {
    VTR_ASSERT(!partitioned_);
    VTR_ASSERT(!remapped_edges_);

    edges_read_ = true;
    int num_rr_switches = 0;

    // Sort by destination node to collect per node/per switch fan in
    // values.
    //
    // This sort is safe to do because partition_edges() has not been invoked yet.
    std::sort(
        edge_sort_iterator(this, 0),
        edge_sort_iterator(this, edge_dest_node_.size()),
        edge_compare_dest_node());

    //Collect the fan-in per switch type for each node in the graph
    //Record the unique switch type/fanin combinations
    std::vector<int> arch_switch_counts;
    arch_switch_counts.resize(num_arch_switches, 0);
    auto first_edge = edge_dest_node_.begin();
    do {
        RRNodeId node = *first_edge;
        auto last_edge = std::find_if(first_edge, edge_dest_node_.end(), [node](const RRNodeId& other) {
            return other != node;
        });

        size_t first_edge_offset = first_edge - edge_dest_node_.begin();
        size_t last_edge_offset = last_edge - edge_dest_node_.begin();

        std::fill(arch_switch_counts.begin(), arch_switch_counts.end(), 0);
        for (auto edge_switch : vtr::Range<decltype(edge_switch_)::const_iterator>(
                 edge_switch_.begin() + first_edge_offset,
                 edge_switch_.begin() + last_edge_offset)) {
            arch_switch_counts[edge_switch] += 1;
        }

        for (size_t iswitch = 0; iswitch < arch_switch_counts.size(); ++iswitch) {
            if (arch_switch_counts[iswitch] == 0) {
                continue;
            }

            auto fanin = arch_switch_counts[iswitch];
            VTR_ASSERT_SAFE(iswitch < num_arch_switches);

            if (arch_switch_inf[iswitch].fixed_Tdel()) {
                //If delay is independent of fanin drop the unique fanin info
                fanin = UNDEFINED;
            }

            if (arch_switch_fanins[iswitch].count(fanin) == 0) {        //New fanin for this switch
                arch_switch_fanins[iswitch][fanin] = num_rr_switches++; //Assign it a unique index
            }
        }

        first_edge = last_edge;

    } while (first_edge != edge_dest_node_.end());

    return num_rr_switches;
}

void t_rr_graph_storage::remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin) {
    edges_read_ = true;

    VTR_ASSERT(!remapped_edges_);
    for (size_t i = 0; i < edge_src_node_.size(); ++i) {
        RREdgeId edge(i);

        RRNodeId to_node = edge_dest_node_[edge];
        int switch_index = edge_switch_[edge];
        int fanin = node_fan_in_[to_node];

        if (switch_fanin[switch_index].count(UNDEFINED) == 1) {
            fanin = UNDEFINED;
        }

        auto itr = switch_fanin[switch_index].find(fanin);
        VTR_ASSERT(itr != switch_fanin[switch_index].end());

        int rr_switch_index = itr->second;

        edge_switch_[edge] = rr_switch_index;
    }
    remapped_edges_ = true;
}

void t_rr_graph_storage::mark_edges_as_rr_switch_ids() {
    edges_read_ = true;
    remapped_edges_ = true;
}

void t_rr_graph_storage::partition_edges() {
    if (partitioned_) {
        return;
    }

    edges_read_ = true;
    VTR_ASSERT(remapped_edges_);

    const auto& device_ctx = g_vpr_ctx.device();
    // This sort ensures two things:
    //  - Edges are stored in ascending source node order.  This is required
    //    by assign_first_edges()
    //  - Edges within a source node have the configurable edges before the
    //    non-configurable edges.
    std::sort(
        edge_sort_iterator(this, 0),
        edge_sort_iterator(this, edge_src_node_.size()),
        edge_compare_src_node_and_configurable_first(device_ctx.rr_switch_inf));

    partitioned_ = true;

    assign_first_edges();

    VTR_ASSERT_SAFE(validate());
}

t_edge_size t_rr_graph_storage::num_configurable_edges(const RRNodeId& id) const {
    VTR_ASSERT(!node_first_edge_.empty() && remapped_edges_);

    const auto& device_ctx = g_vpr_ctx.device();
    auto first_id = size_t(node_first_edge_[id]);
    auto last_id = size_t((&node_first_edge_[id])[1]);
    for (size_t idx = first_id; idx < last_id; ++idx) {
        auto switch_idx = edge_switch_[RREdgeId(idx)];
        if (!device_ctx.rr_switch_inf[switch_idx].configurable()) {
            return idx - first_id;
        }
    }

    return last_id - first_id;
}

t_edge_size t_rr_graph_storage::num_non_configurable_edges(const RRNodeId& id) const {
    return num_edges(id) - num_configurable_edges(id);
}

bool t_rr_graph_storage::validate() const {
    bool all_valid = verify_first_edges();
    for (size_t inode = 0; inode < size(); ++inode) {
        all_valid = (*this)[inode].validate() || all_valid;
    }
    return all_valid;
}

const char* t_rr_graph_storage::node_type_string(RRNodeId id) const {
    return rr_node_typename[node_type(id)];
}

void t_rr_graph_storage::set_node_ptc_num(RRNodeId id, short new_ptc_num) {
    node_ptc_[id].ptc_.pin_num = new_ptc_num; //TODO: eventually remove
}
void t_rr_graph_storage::set_node_pin_num(RRNodeId id, short new_pin_num) {
    if (node_type(id) != IPIN && node_type(id) != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'pin_num' for non-IPIN/OPIN type '%s'", node_type_string(id));
    }
    node_ptc_[id].ptc_.pin_num = new_pin_num;
}

void t_rr_graph_storage::set_node_track_num(RRNodeId id, short new_track_num) {
    if (node_type(id) != CHANX && node_type(id) != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'track_num' for non-CHANX/CHANY type '%s'", node_type_string(id));
    }
    node_ptc_[id].ptc_.track_num = new_track_num;
}

void t_rr_graph_storage::set_node_class_num(RRNodeId id, short new_class_num) {
    if (node_type(id) != SOURCE && node_type(id) != SINK) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'class_num' for non-SOURCE/SINK type '%s'", node_type_string(id));
    }
    node_ptc_[id].ptc_.class_num = new_class_num;
}

short t_rr_graph_storage::node_ptc_num(RRNodeId id) const {
    return node_ptc_[id].ptc_.pin_num;
}
short t_rr_graph_storage::node_pin_num(RRNodeId id) const {
    if (node_type(id) != IPIN && node_type(id) != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'pin_num' for non-IPIN/OPIN type '%s'", node_type_string(id));
    }
    return node_ptc_[id].ptc_.pin_num;
}
short t_rr_graph_storage::node_track_num(RRNodeId id) const {
    if (node_type(id) != CHANX && node_type(id) != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'track_num' for non-CHANX/CHANY type '%s'", node_type_string(id));
    }
    return node_ptc_[id].ptc_.track_num;
}
short t_rr_graph_storage::node_class_num(RRNodeId id) const {
    if (node_type(id) != SOURCE && node_type(id) != SINK) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to access RR node 'class_num' for non-SOURCE/SINK type '%s'", node_type_string(id));
    }
    return node_ptc_[id].ptc_.class_num;
}

void t_rr_graph_storage::set_node_type(RRNodeId id, t_rr_type new_type) {
    node_storage_[id].type_ = new_type;
}

void t_rr_graph_storage::set_node_coordinates(RRNodeId id, short x1, short y1, short x2, short y2) {
    auto& node = node_storage_[id];
    if (x1 < x2) {
        node.xlow_ = x1;
        node.xhigh_ = x2;
    } else {
        node.xlow_ = x2;
        node.xhigh_ = x1;
    }

    if (y1 < y2) {
        node.ylow_ = y1;
        node.yhigh_ = y2;
    } else {
        node.ylow_ = y2;
        node.yhigh_ = y1;
    }
}

void t_rr_graph_storage::set_node_cost_index(RRNodeId id, size_t new_cost_index) {
    auto& node = node_storage_[id];
    if (new_cost_index >= std::numeric_limits<decltype(node.cost_index_)>::max()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set cost_index_ %zu above cost_index storage max value.",
                        new_cost_index);
    }
    node.cost_index_ = new_cost_index;
}

void t_rr_graph_storage::set_node_rc_index(RRNodeId id, short new_rc_index) {
    node_storage_[id].rc_index_ = new_rc_index;
}

void t_rr_graph_storage::set_node_capacity(RRNodeId id, short new_capacity) {
    VTR_ASSERT(new_capacity >= 0);
    node_storage_[id].capacity_ = new_capacity;
}

void t_rr_graph_storage::set_node_direction(RRNodeId id, e_direction new_direction) {
    if (node_type(id) != CHANX && node_type(id) != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'direction' for non-channel type '%s'", node_type_string(id));
    }
    node_storage_[id].dir_side_.direction = new_direction;
}

void t_rr_graph_storage::set_node_side(RRNodeId id, e_side new_side) {
    if (node_type(id) != IPIN && node_type(id) != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'side' for non-channel type '%s'", node_type_string(id));
    }
    node_storage_[id].dir_side_.side = new_side;
}
