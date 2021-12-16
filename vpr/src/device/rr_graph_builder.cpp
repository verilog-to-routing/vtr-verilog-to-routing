#include "vtr_log.h"
#include "rr_graph_builder.h"
#include "vtr_time.h"
#include <queue>
#include "globals.h"

RRGraphBuilder::RRGraphBuilder(t_rr_graph_storage* node_storage)
    : node_storage_(*node_storage) {
}

t_rr_graph_storage& RRGraphBuilder::node_storage() {
    return node_storage_;
}

RRSpatialLookup& RRGraphBuilder::node_lookup() {
    return node_lookup_;
}

void RRGraphBuilder::add_node_to_all_locs(RRNodeId node) {
    t_rr_type node_type = node_storage_.node_type(node);
    short node_ptc_num = node_storage_.node_ptc_num(node);
    for (int ix = node_storage_.node_xlow(node); ix <= node_storage_.node_xhigh(node); ix++) {
        for (int iy = node_storage_.node_ylow(node); iy <= node_storage_.node_yhigh(node); iy++) {
            switch (node_type) {
                case SOURCE:
                case SINK:
                case CHANY:
                    node_lookup_.add_node(node, ix, iy, node_type, node_ptc_num, SIDES[0]);
                    break;
                case CHANX:
                    /* Currently need to swap x and y for CHANX because of chan, seg convention 
                     * TODO: Once the builders is reworked for use consistent (x, y) convention,
                     * the following swapping can be removed
                     */
                    node_lookup_.add_node(node, iy, ix, node_type, node_ptc_num, SIDES[0]);
                    break;
                case OPIN:
                case IPIN:
                    for (const e_side& side : SIDES) {
                        if (node_storage_.is_node_on_specific_side(node, side)) {
                            node_lookup_.add_node(node, ix, iy, node_type, node_ptc_num, side);
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

void RRGraphBuilder::clear() {
    node_lookup_.clear();
}

// Reorder RRNodeId's using one of these algorithms:
//   - DONT_REORDER: The identity reordering (does nothing.)
//   - DEGREE_BFS: Order by degree primarily, and BFS traversal order secondarily.
//   - RANDOM_SHUFFLE: Shuffle using the specified seed. Great for testing.
// The DEGREE_BFS algorithm was selected because it had the best performance of seven
// existing algorithms here: https://github.com/SymbiFlow/vtr-rrgraph-reordering-tool
// It might be worth further research, as the DEGREE_BFS algorithm is simple and
// makes some arbitrary choices, such as the starting node.
// Nonetheless, it does improve performance ~7% for the SymbiFlow Xilinx Artix 7 graph.
//
// NOTE: Re-ordering will invalidate any references to rr_graph nodes, so this
//       should generally be called before creating such references.
void RRGraphBuilder::reorder_rr_graph_nodes_(const t_router_opts& router_opts) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_graph = device_ctx.rr_graph;
    size_t v_num = node_storage_.size();

    if (router_opts.reorder_rr_graph_nodes_algorithm == DONT_REORDER) return;
    if (router_opts.reorder_rr_graph_nodes_threshold < 0 || v_num < (size_t)router_opts.reorder_rr_graph_nodes_threshold) return;

    vtr::ScopedStartFinishTimer timer("Reordering rr_graph nodes");

    vtr::vector<RRNodeId, RRNodeId> src_order(v_num); // new id -> old id
    size_t cur_idx = 0;
    for (RRNodeId& n : src_order) { // Initialize to [0, 1, 2 ...]
        n = RRNodeId(cur_idx++);
    }

    // This method works well. The intution is that highly connected nodes are enumerated first (together),
    // and since there will be a lot of nodes with the same degree, they are then ordered based on some
    // distance from the starting node.
    if (router_opts.reorder_rr_graph_nodes_algorithm == DEGREE_BFS) {
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
                for (RREdgeId edge = rr_graph.node_first_edge(u); edge < rr_graph.node_last_edge(u); edge = RREdgeId(size_t(edge) + 1)) {
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
    } else if (router_opts.reorder_rr_graph_nodes_algorithm == RANDOM_SHUFFLE) {
        std::mt19937 g(router_opts.reorder_rr_graph_nodes_seed);
        std::shuffle(src_order.begin(), src_order.end(), g);
    }
    vtr::vector<RRNodeId, RRNodeId> dest_order(v_num);
    cur_idx = 0;
    for (auto u : src_order)
        dest_order[u] = RRNodeId(cur_idx++);

    node_storage_.reorder(dest_order, src_order);

    node_lookup().reorder(dest_order);

    device_ctx.rr_node_metadata.remap_keys([&](int node) { return size_t(dest_order[RRNodeId(node)]); });
    device_ctx.rr_edge_metadata.remap_keys([&](std::tuple<int, int, short> edge) {
        return std::make_tuple(size_t(dest_order[RRNodeId(std::get<0>(edge))]),
                               size_t(dest_order[RRNodeId(std::get<1>(edge))]),
                               std::get<2>(edge));
    });
}
