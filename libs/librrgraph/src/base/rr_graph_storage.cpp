#include <climits>
#include "rr_graph_storage.h"
#include "physical_types.h"
#include "rr_graph_fwd.h"
#include "vtr_assert.h"
#include "vtr_error.h"
#include "librrgraph_types.h"
#include "vtr_util.h"

#include <algorithm>
#include <cstddef>
#include <ranges>

void t_rr_graph_storage::reserve_edges(size_t num_edges) {
    edge_src_node_.reserve(num_edges);
    edge_dest_node_.reserve(num_edges);
    edge_switch_.reserve(num_edges);
    edge_remapped_.reserve(num_edges);
    edge_crr_id_.reserve(num_edges);
}

void t_rr_graph_storage::emplace_back_edge(RRNodeId src, RRNodeId dest, short edge_switch, bool remapped, std::string crr_id) {
    // Cannot mutate edges once edges have been read!
    VTR_ASSERT(!edges_read_);
    edge_src_node_.emplace_back(src);
    edge_dest_node_.emplace_back(dest);
    edge_switch_.emplace_back(edge_switch);
    edge_remapped_.emplace_back(remapped);
    edge_crr_id_.emplace_back(crr_id);
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
        edge_remapped_.reserve(new_capacity);
        edge_crr_id_.reserve(new_capacity);
    }

    for (const t_rr_edge_info& new_edge : *rr_edges_to_create) {
        emplace_back_edge(
            new_edge.from_node,
            new_edge.to_node,
            new_edge.switch_type,
            new_edge.remapped,
            new_edge.crr_id);
    }
}

void t_rr_graph_storage::remove_edges(std::vector<RREdgeId>& rr_edges_to_remove) {
    VTR_ASSERT(!edges_read_);

    if (rr_edges_to_remove.empty()) {
        return;
    }

    size_t starting_edge_count = edge_dest_node_.size();

    // Sort and make sure all edge indices are unique
    vtr::uniquify(rr_edges_to_remove);
    VTR_ASSERT_SAFE(std::is_sorted(rr_edges_to_remove.begin(), rr_edges_to_remove.end()));

    // Make sure the edge indices are valid
    VTR_ASSERT(static_cast<size_t>(rr_edges_to_remove.back()) <= edge_dest_node_.size());
    
    // Index of the last edge
    size_t edge_list_end = edge_dest_node_.size() - 1;

    // Iterate backwards through the list of indices we want to remove.
    
    for (RREdgeId erase_idx : std::ranges::reverse_view(rr_edges_to_remove)) {
        // Copy what's at the end of the list to the index we wanted to remove
        edge_dest_node_[erase_idx] = edge_dest_node_[RREdgeId(edge_list_end)];
        edge_src_node_[erase_idx] = edge_src_node_[RREdgeId(edge_list_end)];
        edge_switch_[erase_idx] = edge_switch_[RREdgeId(edge_list_end)];
        edge_remapped_[erase_idx] = edge_remapped_[RREdgeId(edge_list_end)];
        edge_crr_id_[erase_idx] = edge_crr_id_[RREdgeId(edge_list_end)];

        // At this point we have no copies of what was at erase_idx and two copies of
        // what was at the end of the list. If we make the list one element shorter,
        // we end up with a list that has removed the element at erase_idx.
        edge_list_end--;

    }

    // We have a new index to the end of the list, call erase on the elements past that index
    // to update the std::vector and shrink the actual data structures.
    edge_dest_node_.erase(edge_dest_node_.begin() + edge_list_end + 1, edge_dest_node_.end());
    edge_src_node_.erase(edge_src_node_.begin() + edge_list_end + 1, edge_src_node_.end());
    edge_switch_.erase(edge_switch_.begin() + edge_list_end + 1, edge_switch_.end());
    edge_remapped_.erase(edge_remapped_.begin() + edge_list_end + 1, edge_remapped_.end());
    edge_crr_id_.erase(edge_crr_id_.begin() + edge_list_end + 1, edge_crr_id_.end());

    VTR_ASSERT(edge_dest_node_.size() == (starting_edge_count - rr_edges_to_remove.size()));

    partitioned_ = false;
}


void t_rr_graph_storage::assign_first_edges() {
    VTR_ASSERT(node_first_edge_.empty());

    // Last element is a dummy element
    node_first_edge_.resize(node_storage_.size() + 1);

    VTR_ASSERT(std::is_sorted(
        edge_src_node_.begin(),
        edge_src_node_.end()));

    size_t node_id = 0;
    size_t first_edge_id = 0;
    size_t second_edge_id = 0;

    size_t num_edges = edge_src_node_.size();
    VTR_ASSERT(edge_dest_node_.size() == num_edges);
    VTR_ASSERT(edge_switch_.size() == num_edges);
    VTR_ASSERT(edge_remapped_.size() == num_edges);
    VTR_ASSERT(edge_crr_id_.size() == num_edges);

    while (true) {
        VTR_ASSERT(first_edge_id < num_edges);
        VTR_ASSERT(second_edge_id < num_edges);

        size_t current_node_id = size_t(edge_src_node_[RREdgeId(second_edge_id)]);
        if (node_id < current_node_id) {
            // All edges belonging to node_id are assigned.
            while (node_id < current_node_id) {
                // Store any edges belongs to node_id.
                VTR_ASSERT(node_id < node_first_edge_.size());
                node_first_edge_[RRNodeId(node_id)] = RREdgeId(first_edge_id);
                first_edge_id = second_edge_id;
                node_id += 1;
            }

            VTR_ASSERT(node_id == current_node_id);
            node_first_edge_[RRNodeId(node_id)] = RREdgeId(second_edge_id);
        } else {
            second_edge_id += 1;
            if (second_edge_id == num_edges) {
                break;
            }
        }
    }

    // All remaining nodes have no edges, set as such.
    for (size_t inode = node_id + 1; inode < node_first_edge_.size(); ++inode) {
        node_first_edge_[RRNodeId(inode)] = RREdgeId(second_edge_id);
    }

    VTR_ASSERT_SAFE(verify_first_edges());
}

bool t_rr_graph_storage::verify_first_edges() const {
    size_t num_edges = edge_src_node_.size();
    VTR_ASSERT_MSG(node_first_edge_[RRNodeId(node_storage_.size())] == RREdgeId(num_edges), 
                vtr::string_fmt("node first edge is '%lu' while expected edge id is '%lu'\n", 
                size_t(node_first_edge_[RRNodeId(node_storage_.size())]), 
                num_edges).c_str());

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
    node_fan_in_.clear();
    node_fan_in_.resize(node_storage_.size(), 0);
    node_fan_in_.shrink_to_fit();
    //Walk the graph and increment fanin on all downstream nodes
    for(const auto& [_, dest_node] : edge_dest_node_.pairs()) {
        node_fan_in_[dest_node] += 1;
    }
}

namespace {
/// Functor for sorting edges according to destination node's ID.
class edge_compare_dest_node {
public:
    edge_compare_dest_node(const t_rr_graph_storage& rr_graph_storage) : rr_graph_storage_(rr_graph_storage) {}

    bool operator()(RREdgeId lhs, RREdgeId rhs) const {
        RRNodeId lhs_dest_node = rr_graph_storage_.edge_sink_node(lhs);
        RRNodeId rhs_dest_node = rr_graph_storage_.edge_sink_node(rhs);

        return lhs_dest_node < rhs_dest_node;
    }

private:
    const t_rr_graph_storage& rr_graph_storage_;
};
} // namespace

size_t t_rr_graph_storage::count_rr_switches(const std::vector<t_arch_switch_inf>& arch_switch_inf,
                                             t_arch_switch_fanin& arch_switch_fanins) {
    VTR_ASSERT(!partitioned_);
    VTR_ASSERT(!remapped_edges_);

    edges_read_ = true;
    int num_rr_switches = 0;

    // Sort by destination node to collect per node/per switch fan in values
    // This sort is safe to do because partition_edges() has not been invoked yet.
    sort_edges(edge_compare_dest_node(*this));

    // Collect the fan-in per switch type for each node in the graph
    // Record the unique switch type/fanin combinations
    std::vector<int> arch_switch_counts;
    arch_switch_counts.resize(arch_switch_inf.size(), 0);
    auto first_edge = edge_dest_node_.begin();
    do {
        RRNodeId node = *first_edge;
        auto last_edge = std::find_if(first_edge, edge_dest_node_.end(), [node](const RRNodeId& other) {
            return other != node;
        });

        size_t first_edge_offset = first_edge - edge_dest_node_.begin();
        size_t last_edge_offset = last_edge - edge_dest_node_.begin();

        std::fill(arch_switch_counts.begin(), arch_switch_counts.end(), 0);
        for (short edge_switch : vtr::Range<decltype(edge_switch_)::const_iterator>(
                 edge_switch_.begin() + first_edge_offset,
                 edge_switch_.begin() + last_edge_offset)) {
            arch_switch_counts[edge_switch] += 1;
        }

        for (size_t iswitch = 0; iswitch < arch_switch_counts.size(); ++iswitch) {
            if (arch_switch_counts[iswitch] == 0) {
                continue;
            }

            int fanin = arch_switch_counts[iswitch];
            VTR_ASSERT_SAFE(iswitch < arch_switch_inf.size());

            if (arch_switch_inf[iswitch].fixed_Tdel()) {
                // If delay is independent of fanin drop the unique fanin info
                fanin = LIBRRGRAPH_UNDEFINED_VAL;
            }

            if (arch_switch_fanins[iswitch].count(fanin) == 0) {        // New fanin for this switch
                arch_switch_fanins[iswitch][fanin] = RRSwitchId(num_rr_switches); // Assign it a unique index
                num_rr_switches++;
            }
        }

        first_edge = last_edge;

    } while (first_edge != edge_dest_node_.end());

    // We assign a rr switch for other arch switches. This is done mainly for flat-routing,
    // to avoid allocating switches again. We assume that internal switches' delay are not
    // dependent on their fan-in
    for (size_t iswitch = 0; iswitch < arch_switch_counts.size(); ++iswitch) {
        if (arch_switch_fanins[iswitch].empty()){
            if (arch_switch_inf[iswitch].fixed_Tdel()){
                arch_switch_fanins[iswitch][LIBRRGRAPH_UNDEFINED_VAL] = RRSwitchId(num_rr_switches);
                num_rr_switches++;
            }
        }
    }

    return num_rr_switches;
}

void t_rr_graph_storage::remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin) {
    edges_read_ = true;

    VTR_ASSERT(!remapped_edges_);
    for (size_t i = 0; i < edge_src_node_.size(); ++i) {
        RREdgeId edge(i);
        if (edge_remapped_[edge]) {
            continue;
        }

        RRNodeId to_node = edge_dest_node_[edge];
        int switch_index = edge_switch_[edge];
        int fanin = node_fan_in_[to_node];

        if (switch_fanin[switch_index].count(LIBRRGRAPH_UNDEFINED_VAL) == 1) {
            fanin = LIBRRGRAPH_UNDEFINED_VAL;
        }

        auto itr = switch_fanin[switch_index].find(fanin);
        VTR_ASSERT(itr != switch_fanin[switch_index].end());

        int rr_switch_index = (int)itr->second;

        edge_switch_[edge] = rr_switch_index;
        edge_remapped_[edge] = true;
    }
    remapped_edges_ = true;
}

void t_rr_graph_storage::mark_edges_as_rr_switch_ids() {
    edges_read_ = true;
    remapped_edges_ = true;
}

namespace{
/// Functor for sorting edges according to source node, with configurable edges coming first
class edge_compare_src_node_and_configurable_first {
  public:
    edge_compare_src_node_and_configurable_first(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf, const t_rr_graph_storage& rr_graph_storage)
        : rr_switch_inf_(rr_switch_inf),
          rr_graph_storage_(rr_graph_storage) {}

    bool operator()(RREdgeId lhs, RREdgeId rhs) const {

        RRNodeId lhs_dest_node = rr_graph_storage_.edge_sink_node(lhs);
        RRNodeId lhs_src_node = rr_graph_storage_.edge_source_node(lhs);
        RRSwitchId lhs_switch_type = RRSwitchId(rr_graph_storage_.edge_switch(lhs));
        bool lhs_is_configurable = rr_switch_inf_[lhs_switch_type].configurable();

        RRNodeId rhs_dest_node = rr_graph_storage_.edge_sink_node(rhs);
        RRNodeId rhs_src_node = rr_graph_storage_.edge_source_node(rhs);
        RRSwitchId rhs_switch_type = RRSwitchId(rr_graph_storage_.edge_switch(rhs));
        bool rhs_is_configurable = rr_switch_inf_[rhs_switch_type].configurable();

        return std::make_tuple(lhs_src_node, !lhs_is_configurable, lhs_dest_node, lhs_switch_type) < std::make_tuple(rhs_src_node, !rhs_is_configurable, rhs_dest_node, rhs_switch_type);
    }

  private:
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf_;
    const t_rr_graph_storage& rr_graph_storage_;
};
} // namespace

void t_rr_graph_storage::partition_edges(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) {
    if (partitioned_) {
        return;
    }

    edges_read_ = true;
    VTR_ASSERT(remapped_edges_);
    // This sort ensures two things:
    //  - Edges are stored in ascending source node order.  This is required
    //    by assign_first_edges()
    //  - Edges within a source node have the configurable edges before the
    //    non-configurable edges.
    sort_edges(edge_compare_src_node_and_configurable_first(rr_switches, *this));

    partitioned_ = true;

    assign_first_edges();

    VTR_ASSERT_SAFE(validate(rr_switches));
}

void t_rr_graph_storage::override_edge_switch(RREdgeId edge_id, RRSwitchId switch_id) {
    VTR_ASSERT_DEBUG(partitioned_);
    VTR_ASSERT_DEBUG(remapped_edges_);
    edge_switch_[edge_id] = (short)((size_t)switch_id);
}

t_edge_size t_rr_graph_storage::num_configurable_edges(RRNodeId id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
    VTR_ASSERT(!node_first_edge_.empty() && remapped_edges_);

    auto first_id = size_t(node_first_edge_[id]);
    auto last_id = size_t((&node_first_edge_[id])[1]);
    for (size_t idx = first_id; idx < last_id; ++idx) {
        auto switch_idx = edge_switch_[RREdgeId(idx)];
        if (!rr_switches[RRSwitchId(switch_idx)].configurable()) {
            return idx - first_id;
        }
    }

    return last_id - first_id;
}

t_edge_size t_rr_graph_storage::num_non_configurable_edges(RRNodeId node, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
    return num_edges(node) - num_configurable_edges(node, rr_switches);
}

bool t_rr_graph_storage::edge_is_configurable(RREdgeId edge, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
  short iswitch = edge_switch(edge);
  return rr_switches[RRSwitchId(iswitch)].configurable();
}

bool t_rr_graph_storage::edge_is_configurable(RRNodeId id, t_edge_size iedge, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
  short iswitch = edge_switch(id, iedge);
  return rr_switches[RRSwitchId(iswitch)].configurable();
}

bool t_rr_graph_storage::validate_node(RRNodeId node_id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
   t_edge_size iedge = 0;
   for (auto edge : edges(node_id)) {
       if (edge < num_configurable_edges(node_id, rr_switches)) {
           if (!edge_is_configurable(node_id, edge, rr_switches)) {
               VTR_LOG_ERROR("RR Node non-configurable edge found in configurable edge list");
           }
       } else {
           if (edge_is_configurable(node_id, edge, rr_switches)) {
               VTR_LOG_ERROR("RR Node configurable edge found in non-configurable edge list");
           }
       }
       ++iedge;
   }

   if (iedge != num_edges(node_id)) {
       VTR_LOG_ERROR("RR Node Edge iteration does not match edge size");
   }

   return true;
}

bool t_rr_graph_storage::validate(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
    bool all_valid = verify_first_edges();
    for (size_t inode = 0; inode < size(); ++inode) {
        all_valid = validate_node(RRNodeId(inode), rr_switches) || all_valid;
    }
    return all_valid;
}

const char* t_rr_graph_storage::node_type_string(RRNodeId id) const {
    return rr_node_typename[node_type(id)];
}
const char* t_rr_graph_view::node_type_string(RRNodeId id) const {
    return rr_node_typename[node_type(id)];
}

const std::string& t_rr_graph_storage::node_direction_string(RRNodeId id) const {
    Direction direction = node_direction(id);

    int int_direction = static_cast<int>(direction);
    VTR_ASSERT(int_direction >= 0 && int_direction < static_cast<int>(Direction::NUM_DIRECTIONS));
    return CONST_DIRECTION_STRING[int_direction];
}


const std::vector<e_side> t_rr_graph_storage::node_sides(RRNodeId id) const {
    std::vector<e_side> sides;
    for (const e_side& side : TOTAL_2D_SIDES) {
        if (is_node_on_specific_side(id, side)) {
            sides.push_back(side);
        }
    }
    return sides;
}

const char* t_rr_graph_storage::node_side_string(RRNodeId id) const {
    for (const e_side& side : TOTAL_2D_SIDES) {
        if (is_node_on_specific_side(id, side)) {
            return TOTAL_2D_SIDE_STRINGS[side];
        }
    }
    // Not found, return an invalid string
    return TOTAL_2D_SIDE_STRINGS[NUM_2D_SIDES];
}

void t_rr_graph_storage::set_node_layer(RRNodeId id, char layer_low, char layer_high) {
    node_layer_[id] = std::make_pair(layer_low, layer_high);
}

void t_rr_graph_storage::set_node_ptc_num(RRNodeId id, int new_ptc_num) {
    node_ptc_[id].ptc_.pin_num = new_ptc_num; //TODO: eventually remove
}
void t_rr_graph_storage::set_node_pin_num(RRNodeId id, int new_pin_num) {
    if (node_type(id) != e_rr_type::IPIN && node_type(id) != e_rr_type::OPIN) {
        VTR_LOG_ERROR("Attempted to set RR node 'pin_num' for non-IPIN/OPIN type '%s'", node_type_string(id));
    }
    node_ptc_[id].ptc_.pin_num = new_pin_num;
}

void t_rr_graph_storage::set_node_track_num(RRNodeId id, int new_track_num) {
    if (node_type(id) != e_rr_type::CHANX && node_type(id) != e_rr_type::CHANY && node_type(id) != e_rr_type::CHANZ)  {
        VTR_LOG_ERROR("Attempted to set RR node 'track_num' for non-CHANX/CHANY/CHANZ type '%s'", node_type_string(id));
    }
    node_ptc_[id].ptc_.track_num = new_track_num;
}

void t_rr_graph_storage::set_node_class_num(RRNodeId id, int new_class_num) {
    if (node_type(id) != e_rr_type::SOURCE && node_type(id) != e_rr_type::SINK) {
        VTR_LOG_ERROR("Attempted to set RR node 'class_num' for non-SOURCE/SINK type '%s'", node_type_string(id));
    }
    node_ptc_[id].ptc_.class_num = new_class_num;
}

void t_rr_graph_storage::set_node_mux_num(RRNodeId id, int new_mux_num) {
    if (node_type(id) != e_rr_type::MUX) {
        VTR_LOG_ERROR("Attempted to set RR node 'mux_num' for non-MUX type '%s'\n", node_type_string(id));
    }
    node_ptc_[id].ptc_.mux_num = new_mux_num;
}

int t_rr_graph_storage::node_ptc_num(RRNodeId id) const {
    return node_ptc_[id].ptc_.pin_num;
}

static int get_node_pin_num(vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
                            vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc,
                            RRNodeId id) {
    e_rr_type node_type = node_storage[id].type_;
    if (node_type != e_rr_type::IPIN && node_type != e_rr_type::OPIN) {
        VTR_LOG_ERROR("Attempted to access RR node 'pin_num' for non-IPIN/OPIN type '%s'", rr_node_typename[node_type]);
    }
    return node_ptc[id].ptc_.pin_num;
}

static int get_node_track_num(vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
                              vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc,
                              RRNodeId id) {
    e_rr_type node_type = node_storage[id].type_;
    if (node_type != e_rr_type::CHANX && node_type != e_rr_type::CHANY && node_type != e_rr_type::CHANZ) {
        VTR_LOG_ERROR("Attempted to access RR node 'track_num' for non-CHANX/CHANY/CHANZ type '%s'", rr_node_typename[node_type]);
    }
    return node_ptc[id].ptc_.track_num;
}

static int get_node_class_num(vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
                              vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc,
                              RRNodeId id) {
    e_rr_type node_type = node_storage[id].type_;
    if (node_type != e_rr_type::SOURCE && node_type != e_rr_type::SINK) {
        VTR_LOG_ERROR("Attempted to access RR node 'class_num' for non-SOURCE/SINK type '%s'", rr_node_typename[node_type]);
    }
    return node_ptc[id].ptc_.class_num;
}

static int get_node_mux_num(
    vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
    vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc,
    RRNodeId id) {
    e_rr_type node_type = node_storage[id].type_;
    if (node_type != e_rr_type::MUX) {
        VTR_LOG_ERROR("Attempted to access RR node 'mux_num' for non-MUX type '%s'\n", rr_node_typename[node_type]);
    }
    return node_ptc[id].ptc_.mux_num;
}

int t_rr_graph_storage::node_pin_num(RRNodeId id) const {
    return get_node_pin_num(
        vtr::make_const_array_view_id(node_storage_),
        vtr::make_const_array_view_id(node_ptc_),
        id);
}
int t_rr_graph_storage::node_track_num(RRNodeId id) const {
    return get_node_track_num(
        vtr::make_const_array_view_id(node_storage_),
        vtr::make_const_array_view_id(node_ptc_),
        id);
}
int t_rr_graph_storage::node_class_num(RRNodeId id) const {
    return get_node_class_num(
        vtr::make_const_array_view_id(node_storage_),
        vtr::make_const_array_view_id(node_ptc_),
        id);
}
int t_rr_graph_storage::node_mux_num(RRNodeId id) const {
    return get_node_mux_num(
        vtr::make_const_array_view_id(node_storage_),
        vtr::make_const_array_view_id(node_ptc_),
        id);
}

void t_rr_graph_storage::set_node_type(RRNodeId id, e_rr_type new_type) {
    node_storage_[id].type_ = new_type;
}

void t_rr_graph_storage::set_node_name(RRNodeId id, const std::string& new_name) {
    node_name_.insert(std::make_pair(id, new_name));
}
void t_rr_graph_storage::set_node_coordinates(RRNodeId id, short x1, short y1, short x2, short y2) {
    t_rr_node_data& node = node_storage_[id];
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

void t_rr_graph_storage::set_node_bend_start(RRNodeId id, size_t bend_start) {
    node_bend_start_[id] = bend_start;
}

void t_rr_graph_storage::set_node_bend_end(RRNodeId id, size_t bend_end) {
    node_bend_end_[id] = bend_end;
}

void t_rr_graph_storage::set_node_cost_index(RRNodeId id, RRIndexedDataId new_cost_index) {
    t_rr_node_data& node = node_storage_[id];
    if ((size_t)new_cost_index >= std::numeric_limits<decltype(node.cost_index_)>::max()) {
        VTR_LOG_ERROR("Attempted to set cost_index_ %zu above cost_index storage max value.\n",
                      new_cost_index);
    }
    node.cost_index_ = (size_t)new_cost_index;
}

void t_rr_graph_storage::set_node_rc_index(RRNodeId id, NodeRCIndex new_rc_index) {
    node_storage_[id].rc_index_ = (size_t)new_rc_index;
}

void t_rr_graph_storage::set_node_capacity(RRNodeId id, short new_capacity) {
    VTR_ASSERT(new_capacity >= 0);
    node_storage_[id].capacity_ = new_capacity;
}

void t_rr_graph_storage::set_node_direction(RRNodeId id, Direction new_direction) {
    if (node_type(id) != e_rr_type::CHANX && node_type(id) != e_rr_type::CHANY && node_type(id) != e_rr_type::CHANZ) {
        VTR_LOG_ERROR("Attempted to set RR node 'direction' for non-channel type '%s'", node_type_string(id));
    }
    node_storage_[id].dir_side_.direction = new_direction;
}

void t_rr_graph_storage::set_node_ptc_nums(RRNodeId node, const std::vector<int>& ptc_numbers) {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    VTR_ASSERT(!ptc_numbers.empty());
    // The default VTR RR graph generator assigns only one PTC number per node, which is
    // stored in the node_ptc_ field of rr_graph_storage. However, when the tileable RR
    // graph is used, CHANX/CHANY nodes can have multiple PTC numbers.
    // 
    // To satisfy VPR's requirements, we store the PTC number for offset = 0 in the
    // node_ptc_ field, and store all PTC numbers assigned to the node in the
    // node_tileable_track_nums_ field.
    set_node_ptc_num(node, ptc_numbers[0]);
    if (ptc_numbers.size() > 1) {
        VTR_ASSERT(size_t(node) < node_tilable_track_nums_.size());
        node_tilable_track_nums_[node].resize(ptc_numbers.size());
        for (size_t iptc = 0; iptc < ptc_numbers.size(); iptc++) {
            node_tilable_track_nums_[node][iptc] = ptc_numbers[iptc];
        }
    }
}

void t_rr_graph_storage::add_node_tilable_track_num(RRNodeId node, size_t node_offset, short track_id) {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    VTR_ASSERT(size_t(node) < node_tilable_track_nums_.size());
    VTR_ASSERT_MSG(node_type(node) == e_rr_type::CHANX || node_type(node) == e_rr_type::CHANY,
                   "Track number valid only for CHANX/CHANY RR nodes");

    size_t node_length = std::abs(node_xhigh(node) - node_xlow(node))
                       + std::abs(node_yhigh(node) - node_ylow(node))
                       + 1;
    VTR_ASSERT(node_offset < node_length);

    if (node_length != node_tilable_track_nums_[node].size()) {
        node_tilable_track_nums_[node].resize(node_length);
    }

    node_tilable_track_nums_[node][node_offset] = track_id;
}

void t_rr_graph_storage::add_node_side(RRNodeId id, e_side new_side) {
    if (node_type(id) != e_rr_type::IPIN && node_type(id) != e_rr_type::OPIN) {
        VTR_LOG_ERROR("Attempted to set RR node 'side' for non-pin type '%s'", node_type_string(id));
    }
    std::bitset<NUM_2D_SIDES> side_bits = node_storage_[id].dir_side_.sides;
    side_bits[size_t(new_side)] = true;
    if (side_bits.to_ulong() > CHAR_MAX) {
        VTR_LOG_ERROR("Invalid side '%s' to be added to rr node %u", TOTAL_2D_SIDE_STRINGS[new_side], size_t(id));
    }
    node_storage_[id].dir_side_.sides = static_cast<unsigned char>(side_bits.to_ulong());
}

void t_rr_graph_storage::set_virtual_clock_network_root_idx(RRNodeId virtual_clock_network_root_idx) {
    // Retrieve the name string for the specified RRNodeId.
    auto clock_network_name_str = node_name(virtual_clock_network_root_idx);

    // If the name is available, associate it with the given node id for the clock network virtual sink.
    if(clock_network_name_str) {
        virtual_clock_network_root_idx_.insert(std::make_pair(*(clock_network_name_str.value()), virtual_clock_network_root_idx));
    }
    else {
        // If no name is available, throw a VtrError indicating the absence of the attribute name for the virtual sink node.
        throw vtr::VtrError(vtr::string_fmt("Attribute name is not specified for virtual sink node '%u'\n", size_t(virtual_clock_network_root_idx)), __FILE__, __LINE__);
    }
}

void t_rr_graph_storage::remove_nodes(std::vector<RRNodeId> nodes_to_remove) {
    VTR_ASSERT(!edges_read_);
    VTR_ASSERT(!partitioned_);
    // To remove the nodes, we first sort them in ascending order. This makes it easy 
    // to calculate the offset by which other node IDs need to be adjusted. 
    // For example, after sorting the nodes to be removed, if a node ID falls between 
    // the first and second element, its ID should be reduced by 1. 
    // If a node ID is larger than the last element, its ID should be reduced by 
    // the total number of nodes being removed.
    std::sort(nodes_to_remove.begin(), nodes_to_remove.end());
    
    // Iterate over the nodes to be removed and adjust the IDs of nodes 
    // that fall between them. 
    for (size_t removal_idx = 0; removal_idx < nodes_to_remove.size(); ++removal_idx) {
        size_t start_rr_node_index = size_t(nodes_to_remove[removal_idx]) + 1;
        size_t end_rr_node_index = (removal_idx == nodes_to_remove.size() - 1) ? node_storage_.size() : size_t(nodes_to_remove[removal_idx + 1]);
        for (size_t node_idx = start_rr_node_index; node_idx < end_rr_node_index; ++node_idx) {
            RRNodeId old_node = RRNodeId(node_idx);
            // New node index is equal to the old nodex index minus the number of nodes being removed before it.
            RRNodeId new_node = RRNodeId(node_idx-(removal_idx+1));
            node_storage_[new_node] = node_storage_[old_node];
            node_ptc_[new_node] = node_ptc_[old_node];
            node_layer_[new_node] = node_layer_[old_node];
            node_name_[new_node] = node_name_[old_node];
            if (is_tileable_) {
                node_bend_start_[new_node] = node_bend_start_[old_node];
                node_bend_end_[new_node] = node_bend_end_[old_node];
                node_tilable_track_nums_[new_node] = node_tilable_track_nums_[old_node];
            }
        }
    }

    // Now that the data structures are adjusted, we can shrink the size of them
    size_t num_nodes_to_remove = nodes_to_remove.size();
    VTR_ASSERT(num_nodes_to_remove <= node_storage_.size());
    node_storage_.erase(node_storage_.end()-num_nodes_to_remove, node_storage_.end());
    node_ptc_.erase(node_ptc_.end()-num_nodes_to_remove, node_ptc_.end());
    node_layer_.erase(node_layer_.end()-num_nodes_to_remove, node_layer_.end());
    // After shifting the IDs of nodes that are not removed to the left, the last
    // `num_nodes_to_remove` node IDs become invalid (their names have already been
    // updated for other nodes). Therefore, the corresponding entries in `node_name_`
    // must be removed.
    for (size_t node_index = node_name_.size()-num_nodes_to_remove; node_index < node_name_.size(); ++node_index) {
        RRNodeId node = RRNodeId(node_index);
        node_name_.erase(node);
    }
    if (is_tileable_) {
        node_bend_start_.erase(node_bend_start_.end()-num_nodes_to_remove, node_bend_start_.end());
        node_bend_end_.erase(node_bend_end_.end()-num_nodes_to_remove, node_bend_end_.end());
        node_tilable_track_nums_.erase(node_tilable_track_nums_.end()-num_nodes_to_remove, node_tilable_track_nums_.end());
    }

    std::vector<RREdgeId> removed_edges;
    // This function iterates over edge_src_node_ and edge_dest_node_ to remove
    // entries where either endpoint of an edge is in the nodes_to_remove list.
    // It also updates the node IDs of the remaining edges, since node IDs have
    // been shifted.
    auto adjust_edges = [&](vtr::vector<RREdgeId, RRNodeId>& edge_nodes) {
        for (size_t edge_index = 0; edge_index < edge_nodes.size(); ++edge_index) {
            RREdgeId edge_id = RREdgeId(edge_index);
            RRNodeId node = edge_nodes[edge_id];
    
            // Find insertion point in the sorted vector
            auto node_it = std::lower_bound(nodes_to_remove.begin(), nodes_to_remove.end(), node);
    
            if (node_it != nodes_to_remove.end() && *node_it == node) {
                // Node exists in nodes_to_remove, mark edge for removal
                removed_edges.push_back(edge_id);
            } else {
                // If the node is not in the nodes_to_remove list, update the node ID of the edge
                // by finding how many nodes are there in nodes_to_remove before the node.
                size_t node_offset = std::distance(nodes_to_remove.begin(), node_it);
                size_t new_node_index = size_t(node) - node_offset;
                edge_nodes[edge_id] = RRNodeId(new_node_index);
            }
        }
    };

    adjust_edges(edge_src_node_);
    adjust_edges(edge_dest_node_);

    remove_edges(removed_edges);
}

int t_rr_graph_view::node_ptc_num(RRNodeId id) const {
    return node_ptc_[id].ptc_.pin_num;
}

int t_rr_graph_view::node_pin_num(RRNodeId id) const {
    return get_node_pin_num(node_storage_, node_ptc_, id);
}

int t_rr_graph_view::node_track_num(RRNodeId id) const {
    return get_node_track_num(node_storage_, node_ptc_, id);
}

int t_rr_graph_view::node_class_num(RRNodeId id) const {
    return get_node_class_num(node_storage_, node_ptc_, id);
}

int t_rr_graph_view::node_mux_num(RRNodeId id) const {
    return get_node_mux_num(node_storage_, node_ptc_, id);
}


t_rr_graph_view t_rr_graph_storage::view() const {
    VTR_ASSERT(partitioned_);
    VTR_ASSERT(node_storage_.size() == node_fan_in_.size());
    return t_rr_graph_view(
        vtr::make_const_array_view_id(node_storage_),
        vtr::make_const_array_view_id(node_ptc_),
        vtr::make_const_array_view_id(node_first_edge_),
        vtr::make_const_array_view_id(node_fan_in_),
        vtr::make_const_array_view_id(node_layer_),
        node_name_,
        vtr::make_const_array_view_id(edge_src_node_),
        vtr::make_const_array_view_id(edge_dest_node_),
        vtr::make_const_array_view_id(edge_switch_),
        vtr::make_const_array_view_id(edge_crr_id_),
        virtual_clock_network_root_idx_,
        vtr::make_const_array_view_id(node_bend_start_),
        vtr::make_const_array_view_id(node_bend_end_));
}

// Given `order`, a vector mapping each RRNodeId to a new one (old -> new),
// and `inverse_order`, its inverse (new -> old), update the t_rr_graph_storage
// data structure to an isomorphic graph using the new RRNodeId's.
//
// Because the RRNodeId's affect the memory layout, this can be used to
// optimize cache locality.
//
// Preconditions:
//   order[inverse_order[x]] == x
//   inverse_order[order[x]] == x
//   x != y <===> order[x] != order[y]
//
// NOTE: Re-ordering will invalidate any external references, so this
//       should generally be called before creating such references.
void t_rr_graph_storage::reorder(const vtr::vector<RRNodeId, RRNodeId>& order,
                                 const vtr::vector<RRNodeId, RRNodeId>& inverse_order) {
    VTR_ASSERT(order.size() == inverse_order.size());
    {
        auto old_node_storage = node_storage_;

        // Reorder nodes
        for (size_t i = 0; i < node_storage_.size(); i++) {
            RRNodeId n = RRNodeId(i);
            VTR_ASSERT(n == inverse_order[order[n]]);
            node_storage_[order[n]] = old_node_storage[n];
        }
    }
    {
        auto old_node_first_edge = node_first_edge_;
        auto old_edge_src_node = edge_src_node_;
        auto old_edge_dest_node = edge_dest_node_;
        auto old_edge_switch = edge_switch_;
        auto old_edge_remapped = edge_remapped_;
        auto old_edge_crr_id = edge_crr_id_;
        RREdgeId cur_edge(0);

        // Reorder edges by source node
        for (size_t i = 0; i < node_storage_.size(); i++) {
            node_first_edge_[RRNodeId(i)] = cur_edge;
            RRNodeId n = inverse_order[RRNodeId(i)];
            for (RREdgeId e = old_node_first_edge[n];
                 e < old_node_first_edge[RRNodeId(size_t(n) + 1)];
                 e = RREdgeId(size_t(e) + 1)) {
                edge_src_node_[cur_edge] = order[old_edge_src_node[e]]; // == n?
                edge_dest_node_[cur_edge] = order[old_edge_dest_node[e]];
                edge_switch_[cur_edge] = old_edge_switch[e];
                edge_remapped_[cur_edge] = old_edge_remapped[e];
                edge_crr_id_[cur_edge] = old_edge_crr_id[e];
                cur_edge = RREdgeId(size_t(cur_edge) + 1);
            }
        }
    }
    {
        auto old_node_ptc = node_ptc_;
        for (size_t i = 0; i < node_ptc_.size(); i++) {
            node_ptc_[order[RRNodeId(i)]] = old_node_ptc[RRNodeId(i)];
        }
    }
    {
        auto old_node_fan_in = node_fan_in_;
        for (size_t i = 0; i < node_fan_in_.size(); i++) {
            node_fan_in_[order[RRNodeId(i)]] = old_node_fan_in[RRNodeId(i)];
        }
    }
}
