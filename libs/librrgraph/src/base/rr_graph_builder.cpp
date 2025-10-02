#include "vtr_assert.h"
#include "vtr_log.h"
#include "rr_graph_builder.h"
#include "vtr_time.h"
#include "vtr_util.h"
#include <queue>
#include <random>

RRGraphBuilder::RRGraphBuilder() {
    is_edge_dirty_ = true;
    is_incoming_edge_dirty_ = true;
}

t_rr_graph_storage& RRGraphBuilder::rr_nodes() {
    return node_storage_;
}

RRSpatialLookup& RRGraphBuilder::node_lookup() {
    return node_lookup_;
}

MetadataStorage<int>& RRGraphBuilder::rr_node_metadata() {
    return rr_node_metadata_;
}

MetadataStorage<std::tuple<int, int, short>>& RRGraphBuilder::rr_edge_metadata() {
    return rr_edge_metadata_;
}

vtr::vector<RRNodeId, std::vector<RREdgeId>>& RRGraphBuilder::node_in_edge_storage() {
    return node_in_edges_;
}

vtr::vector<RRNodeId, std::vector<short>>& RRGraphBuilder::node_ptc_storage() {
    return node_tilable_track_nums_;
}

void RRGraphBuilder::add_node_to_all_locs(RRNodeId node) {
    e_rr_type node_type = node_storage_.node_type(node);
    short node_ptc_num = node_storage_.node_ptc_num(node);

    for (int ix = node_storage_.node_xlow(node); ix <= node_storage_.node_xhigh(node); ix++) {
        for (int iy = node_storage_.node_ylow(node); iy <= node_storage_.node_yhigh(node); iy++) {
            for (int iz = node_storage_.node_layer_low(node); iz <= node_storage_.node_layer_high(node); iz++) {
                switch (node_type) {
                    case e_rr_type::SOURCE:
                    case e_rr_type::SINK:
                    case e_rr_type::CHANZ:
                    case e_rr_type::CHANY:
                    case e_rr_type::CHANX:
                        node_lookup_.add_node(node, iz, ix, iy, node_type, node_ptc_num, TOTAL_2D_SIDES[0]);
                        break;

                    case e_rr_type::OPIN:
                    case e_rr_type::IPIN:
                        for (const e_side side : TOTAL_2D_SIDES) {
                            if (node_storage_.is_node_on_specific_side(node, side)) {
                                node_lookup_.add_node(node,iz, ix, iy, node_type, node_ptc_num, side);
                            }
                        }
                        break;

                    default:
                        VTR_LOG_ERROR("Invalid node type for node '%lu' in the routing resource graph file", size_t(node));
                        break;
                }
            }
        }
    }
}

RRNodeId RRGraphBuilder::create_node(int layer, int x, int y, e_rr_type type, int ptc, e_side side) {
    e_side node_side = TOTAL_2D_SIDES[0];
    // Only OPIN and IPIN nodes have sides, otherwise force to use a default side
    if (e_rr_type::OPIN == type || e_rr_type::IPIN == type) {
        node_side = side;
    }
    node_storage_.emplace_back();
    node_tilable_track_nums_.emplace_back();
    RRNodeId new_node = RRNodeId(node_storage_.size() - 1);
    node_storage_.set_node_layer(new_node, layer, layer);
    node_storage_.set_node_type(new_node, type);
    node_storage_.set_node_coordinates(new_node, x, y, x, y);
    node_storage_.set_node_ptc_num(new_node, ptc);
    if (e_rr_type::OPIN == type || e_rr_type::IPIN == type) {
        node_storage_.add_node_side(new_node, node_side);
    }
    node_lookup_.add_node(new_node, layer, x, y, type, ptc, node_side);

    return new_node;
}

void RRGraphBuilder::init_edge_remap(bool val) {
    node_storage_.init_edge_remap(val);
}

void RRGraphBuilder::clear_temp_storage() {
    node_storage_.clear_temp_storage();
}

void RRGraphBuilder::clear() {
    node_lookup_.clear();
    node_storage_.clear();
    node_in_edges_.clear();
    node_tilable_track_nums_.clear();
    rr_node_metadata_.clear();
    rr_edge_metadata_.clear();
    rr_segments_.clear();
    rr_switch_inf_.clear();
    edges_to_build_.clear();
    is_edge_dirty_ = true;
    is_incoming_edge_dirty_ = true;
}

void RRGraphBuilder::reorder_nodes(e_rr_node_reorder_algorithm reorder_rr_graph_nodes_algorithm,
                                   int reorder_rr_graph_nodes_threshold,
                                   int reorder_rr_graph_nodes_seed) {
    size_t v_num = node_storage_.size();
    if (reorder_rr_graph_nodes_threshold < 0 || v_num < (size_t)reorder_rr_graph_nodes_threshold) return;
    vtr::ScopedStartFinishTimer timer("Reordering rr_graph nodes");
    vtr::vector<RRNodeId, RRNodeId> src_order(v_num); // new id -> old id
    size_t cur_idx = 0;
    for (RRNodeId& n : src_order) { // Initialize to [0, 1, 2 ...]
        n = RRNodeId(cur_idx++);
    }

    // This method works well. The intution is that highly connected nodes are enumerated first (together),
    // and since there will be a lot of nodes with the same degree, they are then ordered based on some
    // distance from the starting node.
    if (reorder_rr_graph_nodes_algorithm == DEGREE_BFS) {
        vtr::vector<RRNodeId, size_t> bfs_idx(v_num);
        vtr::vector<RRNodeId, size_t> degree(v_num);
        std::queue<RRNodeId> que;

        // Compute both degree (in + out) and an index based on the BFS traversal
        cur_idx = 0;
        for (size_t i = 0; i < v_num; ++i) {
            if (bfs_idx[RRNodeId(i)]) continue;
            que.push(RRNodeId(i));
            bfs_idx[RRNodeId(i)] = cur_idx++;
            while (!que.empty()) {
                RRNodeId u = que.front();
                que.pop();
                degree[u] += node_storage_.num_edges(u);
                for (RREdgeId edge = node_storage_.first_edge(u); edge < node_storage_.last_edge(u); edge = RREdgeId(size_t(edge) + 1)) {
                    RRNodeId v = node_storage_.edge_sink_node(edge);
                    degree[v]++;
                    if (bfs_idx[v]) continue;
                    bfs_idx[v] = cur_idx++;
                    que.push(v);
                }
            }
        }

        // Sort by degree primarily, and BFS order secondarily
        sort(src_order.begin(), src_order.end(),
             [&](auto a, auto b) -> bool {
                 auto deg_a = degree[a];
                 auto deg_b = degree[b];
                 return deg_a > deg_b || (deg_a == deg_b && bfs_idx[a] < bfs_idx[b]);
             });
    } else if (reorder_rr_graph_nodes_algorithm == RANDOM_SHUFFLE) {
        std::mt19937 g(reorder_rr_graph_nodes_seed);
        std::shuffle(src_order.begin(), src_order.end(), g);
    }
    vtr::vector<RRNodeId, RRNodeId> dest_order(v_num);
    cur_idx = 0;
    for (RRNodeId u : src_order)
        dest_order[u] = RRNodeId(cur_idx++);

    VTR_ASSERT_SAFE(node_storage_.validate(rr_switch_inf_));
    node_storage_.reorder(dest_order, src_order);
    VTR_ASSERT_SAFE(node_storage_.validate(rr_switch_inf_));

    node_lookup().reorder(dest_order);

    rr_node_metadata().remap_keys([&](int node) { return size_t(dest_order[RRNodeId(node)]); });
    rr_edge_metadata().remap_keys([&](std::tuple<int, int, short> edge) {
        return std::make_tuple(size_t(dest_order[RRNodeId(std::get<0>(edge))]),
                               size_t(dest_order[RRNodeId(std::get<1>(edge))]),
                               std::get<2>(edge));
    });
}

void RRGraphBuilder::create_edge_in_cache(RRNodeId src, RRNodeId dest, RRSwitchId edge_switch, bool remapped) {
    edges_to_build_.emplace_back(src, dest, size_t(edge_switch), remapped);
    is_edge_dirty_ = true; // Adding a new edge revokes the flag
    is_incoming_edge_dirty_ = true;
}

void RRGraphBuilder::create_edge(RRNodeId src, RRNodeId dest, RRSwitchId edge_switch, bool remapped) {
    edges_to_build_.emplace_back(src, dest, size_t(edge_switch), remapped);
    is_edge_dirty_ = true; /* Adding a new edge revokes the flag */
    is_incoming_edge_dirty_ = true;
}

void RRGraphBuilder::build_edges(const bool& uniquify) {
    if (uniquify) {
        std::sort(edges_to_build_.begin(), edges_to_build_.end());
        edges_to_build_.erase(std::unique(edges_to_build_.begin(), edges_to_build_.end()), edges_to_build_.end());
    }
    alloc_and_load_edges(&edges_to_build_);
    edges_to_build_.clear(); 
    is_edge_dirty_ = false;
}

void RRGraphBuilder::build_in_edges() {
    VTR_ASSERT(validate());
    node_in_edges_.clear();
    node_in_edges_.resize(node_storage_.size());

    for (RRNodeId src_node : vtr::StrongIdRange<RRNodeId>(RRNodeId(0), RRNodeId(node_storage_.size()))) {
        for (t_edge_size iedge : node_storage_.edges(src_node)) {
            VTR_ASSERT(src_node == node_storage_.edge_source_node(node_storage_.edge_id(src_node, iedge)));
            RRNodeId des_node = node_storage_.edge_sink_node(node_storage_.edge_id(src_node, iedge));
            node_in_edges_[des_node].push_back(node_storage_.edge_id(src_node, iedge));
        }
    }
    is_incoming_edge_dirty_ = false;
}

std::vector<RREdgeId> RRGraphBuilder::node_in_edges(RRNodeId node) const {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    if (is_incoming_edge_dirty_) {
        VTR_LOG_ERROR("Incoming edges are not built yet in routing resource graph. Please call build_in_edges().");
        return std::vector<RREdgeId>();
    }
    if (node_in_edges_.empty()) {
        return std::vector<RREdgeId>();
    }
    return node_in_edges_[node];
}

void RRGraphBuilder::set_node_ptc_nums(RRNodeId node, const std::string& ptc_str) {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    std::vector<std::string> ptc_tokens = vtr::StringToken(ptc_str).split(",");
    VTR_ASSERT(ptc_tokens.size() >= 1);
    set_node_ptc_num(node, std::stoi(ptc_tokens[0]));
    if (ptc_tokens.size() > 1) {
        VTR_ASSERT(size_t(node) < node_tilable_track_nums_.size());
        node_tilable_track_nums_[node].resize(ptc_tokens.size());
        for (size_t iptc = 0; iptc < ptc_tokens.size(); iptc++) {
          node_tilable_track_nums_[node][iptc] = std::stoi(ptc_tokens[iptc]);
        }
    }
}

std::string RRGraphBuilder::node_ptc_nums_to_string(RRNodeId node) const {
    if (node_tilable_track_nums_.empty()) {
        return std::to_string(size_t(node_storage_.node_ptc_num(node)));
    }
    VTR_ASSERT(size_t(node) < node_tilable_track_nums_.size());
    if (node_tilable_track_nums_[node].empty()) {
        return std::to_string(size_t(node_storage_.node_ptc_num(node)));
    }
    std::string ret;
    for (size_t iptc = 0; iptc < node_tilable_track_nums_[node].size(); iptc++) {
        ret += std::to_string(size_t(node_tilable_track_nums_[node][iptc])) + ",";
    }
    // Remove the last comma
    ret.pop_back();
    return ret;
}

bool RRGraphBuilder::node_contain_multiple_ptc(RRNodeId node) const {
    if (node_tilable_track_nums_.empty()) {
        return false;
    }
    return node_tilable_track_nums_[node].size() > 1;
}

void RRGraphBuilder::add_node_track_num(RRNodeId node, vtr::Point<size_t> node_offset, short track_id) {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    VTR_ASSERT(size_t(node) < node_tilable_track_nums_.size());
    VTR_ASSERT_MSG(node_storage_.node_type(node) == e_rr_type::CHANX || node_storage_.node_type(node) == e_rr_type::CHANY, "Track number valid only for CHANX/CHANY RR nodes");

    size_t node_length = std::abs(node_storage_.node_xhigh(node) - node_storage_.node_xlow(node))
                       + std::abs(node_storage_.node_yhigh(node) - node_storage_.node_ylow(node));
    if (node_length + 1 != node_tilable_track_nums_[node].size()) {
        node_tilable_track_nums_[node].resize(node_length + 1);
    }

    size_t offset = node_offset.x() - node_storage_.node_xlow(node) + node_offset.y() - node_storage_.node_ylow(node);
    VTR_ASSERT(offset < node_tilable_track_nums_[node].size());

    node_tilable_track_nums_[node][offset] = track_id;
}

void RRGraphBuilder::add_track_node_to_lookup(RRNodeId node) {
    VTR_ASSERT_MSG(node_storage_.node_type(node) == e_rr_type::CHANX || node_storage_.node_type(node) == e_rr_type::CHANY,
                   "Update track node look-up is only valid to CHANX/CHANY nodes");

    // Compute the track id based on the (x, y) coordinate
    size_t x_start = std::min(node_storage_.node_xlow(node), node_storage_.node_xhigh(node));
    size_t y_start = std::min(node_storage_.node_ylow(node), node_storage_.node_yhigh(node));
    std::vector<size_t> node_x(std::abs(node_storage_.node_xlow(node) - node_storage_.node_xhigh(node)) + 1);
    std::vector<size_t> node_y(std::abs(node_storage_.node_ylow(node) - node_storage_.node_yhigh(node)) + 1);
    
    std::iota(node_x.begin(), node_x.end(), x_start);
    std::iota(node_y.begin(), node_y.end(), y_start);
    
    VTR_ASSERT(size_t(std::max(node_storage_.node_xlow(node), node_storage_.node_xhigh(node))) == node_x.back());
    VTR_ASSERT(size_t(std::max(node_storage_.node_ylow(node), node_storage_.node_yhigh(node))) == node_y.back());

    for (const size_t x : node_x) {
        for (const size_t y : node_y) {
            size_t ptc = node_storage_.node_ptc_num(node);
            e_rr_type node_type = node_storage_.node_type(node);
            // Routing channel nodes may have different ptc num 
            // Find the track ids using the x/y offset  
            if (e_rr_type::CHANX == node_type || e_rr_type::CHANY == node_type) {
                ptc = (node_type == e_rr_type::CHANX) ? node_tilable_track_nums_[node][x - node_storage_.node_xlow(node)] : 
                    node_tilable_track_nums_[node][y - node_storage_.node_ylow(node)];
                node_lookup_.add_node(node, node_storage_.node_layer_low(node), x, y, node_type, ptc);
            }
        }
    }
}
