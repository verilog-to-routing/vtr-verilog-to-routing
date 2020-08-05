#include <queue>
#include <random>
#include <algorithm>

#include "vtr_memory.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node) {
    /* Returns the segment number (distance along the channel) of the connection *
     * box from from_rr_type (CHANX or CHANY) to to_node (IPIN).                 */

    auto& device_ctx = g_vpr_ctx.device();

    if (from_rr_type == CHANX)
        return (device_ctx.rr_nodes[to_node].xlow());
    else
        /* CHANY */
        return (device_ctx.rr_nodes[to_node].ylow());
}

int seg_index_of_sblock(int from_node, int to_node) {
    /* Returns the segment number (distance along the channel) of the switch box *
     * box from from_node (CHANX or CHANY) to to_node (CHANX or CHANY).  The     *
     * switch box on the left side of a CHANX segment at (i,j) has seg_index =   *
     * i-1, while the switch box on the right side of that segment has seg_index *
     * = i.  CHANY stuff works similarly.  Hence the range of values returned is *
     * 0 to device_ctx.grid.width()-1 (if from_node is a CHANX) or 0 to device_ctx.grid.height()-1 (if from_node is a CHANY).   */

    t_rr_type from_rr_type, to_rr_type;

    auto& device_ctx = g_vpr_ctx.device();

    from_rr_type = device_ctx.rr_nodes[from_node].type();
    to_rr_type = device_ctx.rr_nodes[to_node].type();

    if (from_rr_type == CHANX) {
        if (to_rr_type == CHANY) {
            return (device_ctx.rr_nodes[to_node].xlow());
        } else if (to_rr_type == CHANX) {
            if (device_ctx.rr_nodes[to_node].xlow() > device_ctx.rr_nodes[from_node].xlow()) { /* Going right */
                return (device_ctx.rr_nodes[from_node].xhigh());
            } else { /* Going left */
                return (device_ctx.rr_nodes[to_node].xhigh());
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in seg_index_of_sblock: to_node %d is of type %d.\n",
                            to_node, to_rr_type);
            return OPEN; //Should not reach here once thrown
        }
    }
    /* End from_rr_type is CHANX */
    else if (from_rr_type == CHANY) {
        if (to_rr_type == CHANX) {
            return (device_ctx.rr_nodes[to_node].ylow());
        } else if (to_rr_type == CHANY) {
            if (device_ctx.rr_nodes[to_node].ylow() > device_ctx.rr_nodes[from_node].ylow()) { /* Going up */
                return (device_ctx.rr_nodes[from_node].yhigh());
            } else { /* Going down */
                return (device_ctx.rr_nodes[to_node].yhigh());
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in seg_index_of_sblock: to_node %d is of type %d.\n",
                            to_node, to_rr_type);
            return OPEN; //Should not reach here once thrown
        }
    }
    /* End from_rr_type is CHANY */
    else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in seg_index_of_sblock: from_node %d is of type %d.\n",
                        from_node, from_rr_type);
        return OPEN; //Should not reach here once thrown
    }
}

void reorder_rr_graph_nodes(const t_router_opts& router_opts) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& graph = device_ctx.rr_nodes;
    size_t v_num = graph.size();

    if (router_opts.reorder_rr_graph_nodes_algorithm == DONT_REORDER) return;
    if (router_opts.reorder_rr_graph_nodes_threshold < 0 || v_num < (size_t)router_opts.reorder_rr_graph_nodes_threshold) return;

    vtr::ScopedStartFinishTimer timer("Reordering rr_graph nodes");

    vtr::vector<RRNodeId, RRNodeId> src_order(v_num); // new id -> old id
    size_t cur_idx = 0;
    for (RRNodeId& n : src_order) { // Initialize to [0, 1, 2 ...]
        n = RRNodeId(cur_idx++);
    }

    if (router_opts.reorder_rr_graph_nodes_algorithm == DEGREE_BFS) {
        vtr::vector<RRNodeId, size_t> bfs_idx(v_num);
        vtr::vector<RRNodeId, size_t> degree(v_num);
        std::queue<RRNodeId> que;

        cur_idx = 0;
        for (size_t i = 0; i < v_num; ++i) {
            if (bfs_idx[RRNodeId(i)]) continue;
            que.push(RRNodeId(i));
            bfs_idx[RRNodeId(i)] = cur_idx++;
            while (!que.empty()) {
                RRNodeId u = que.front();
                que.pop();
                degree[u] += graph.num_edges(u);
                for (RREdgeId edge = graph.first_edge(u); edge < graph.last_edge(u); edge = RREdgeId(size_t(edge) + 1)) {
                    RRNodeId v = graph.edge_sink_node(edge);
                    degree[v]++;
                    if (bfs_idx[v]) continue;
                    bfs_idx[v] = cur_idx++;
                    que.push(v);
                }
            }
        }

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

    graph.reorder(dest_order, src_order);

    // update rr_node_indices
    for (auto& grid : device_ctx.rr_node_indices) {
        for (size_t x = 0; x < grid.dim_size(0); x++) {
            for (size_t y = 0; y < grid.dim_size(1); y++) {
                for (size_t s = 0; s < grid.dim_size(2); s++) {
                    for (auto& node : grid[x][y][s]) {
                        if (node != OPEN) {
                            node = size_t(dest_order[RRNodeId(node)]);
                        }
                    }
                }
            }
        }
    }
}
