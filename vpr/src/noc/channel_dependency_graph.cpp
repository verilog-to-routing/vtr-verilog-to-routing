
#include "channel_dependency_graph.h"
#include "vtr_assert.h"

#include <stack>

ChannelDependencyGraph::ChannelDependencyGraph(const NocStorage& noc_model,
                                               const NocTrafficFlows& traffic_flow_storage,
                                               const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& traffic_flow_routes,
                                               const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    VTR_ASSERT((size_t)traffic_flow_storage.get_number_of_traffic_flows() == traffic_flow_routes.size());

    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);
        const auto& traffic_flow_route = traffic_flow_routes[traffic_flow_id];

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId src_router_id = noc_model.get_router_at_grid_location(block_locs[logical_source_router_block_id].loc);
        NocRouterId dst_router_id = noc_model.get_router_at_grid_location(block_locs[logical_sink_router_block_id].loc);

        const NocLink& first_link = noc_model.get_single_noc_link(traffic_flow_route.front());
        VTR_ASSERT(first_link.get_source_router() == src_router_id);
        const NocLink& last_link = noc_model.get_single_noc_link(traffic_flow_route.back());
        VTR_ASSERT(last_link.get_sink_router() == dst_router_id);

        for (size_t i = 0; i < traffic_flow_route.size() - 1; i++) {
            const NocLink& noc_link1 = noc_model.get_single_noc_link(traffic_flow_route[i]);
            const NocLink& noc_link2 = noc_model.get_single_noc_link(traffic_flow_route[i + 1]);
            VTR_ASSERT(noc_link1.get_sink_router() == noc_link2.get_source_router());
        }
    }

    const size_t n_links = noc_model.get_noc_links().size();

    adjacency_list_.clear();
    // In channel dependency graph, vertices represent NoC links.
    // reserve enough space so that all vertices can store their outgoing neighbors
    adjacency_list_.resize(n_links);

    /*
     * A traffic flow travels through some NoC links. In channel dependency graph (CDG),
     * consecutive NoC links travelled by the flow are connected using an edge.
     * More specifically, for each pair of consecutive NoC links in a traffic flow route,
     * there exists a directed edge from the NoC link travelled first to the other one.
     * For example, if traffic flow T travels NoC links L0, L2, and L5 to reach its
     * destination, we need to add (L0, L2) and (L2, L5) edges to CDG.
     */

    // iterate over all traffic flows and populate the channel dependency graph
    for (const auto& traffic_flow_route : traffic_flow_routes) {
        auto prev_link_id = NocLinkId::INVALID();
        for (auto cur_link_id : traffic_flow_route) {
            VTR_ASSERT(prev_link_id != cur_link_id);
            if (prev_link_id != NocLinkId::INVALID()) {
                adjacency_list_[prev_link_id].push_back(cur_link_id);
            }
            prev_link_id = cur_link_id;
        }
    }

    // remove repetitive neighbors
    for (auto& neighboring_nodes : adjacency_list_) {
        // sort neighbors so that repetitive nodes are put beside each other
        std::sort(neighboring_nodes.begin(), neighboring_nodes.end());

        // remove consecutive duplicates
        auto it = std::unique(neighboring_nodes.begin(), neighboring_nodes.end());

        // erase the elements from the iterator to the end
        neighboring_nodes.erase(it, neighboring_nodes.end());
    }
}

bool ChannelDependencyGraph::has_cycles() {
    // get the number vertices in CDG
    const size_t n_vertices = adjacency_list_.size();

    // indicates whether a node (NoC link) in CDG is visited during DFS
    vtr::vector<NocLinkId, bool> visited(n_vertices, false);
    // indicates whether a node (NoC links) is currently in stack
    vtr::vector<NocLinkId, bool> on_stack(n_vertices, false);
    // the stack used to perform graph traversal (DFS). Contains to-be-visited vertices
    std::stack<NocLinkId> stack;

    // iterate over all vertices (NoC links)
    for (NocLinkId noc_link_id : adjacency_list_.keys()) {
        // the node (NoC link) has already been visited
        if (visited[noc_link_id]) {
            continue;
        }

        // An un-visited node is found. Add to the stack
        stack.push(noc_link_id);

        // continue the traversal until the stack is empty
        while (!stack.empty()) {
            auto current_vertex_id = stack.top();

            if (!visited[current_vertex_id]) {
                on_stack[current_vertex_id] = true;
                visited[current_vertex_id] = true;
            } else { // the neighboring vertices have already been processed
                // remove it from the stack
                stack.pop();
                on_stack[current_vertex_id] = false;
            }

            // get the outgoing edges of the current vertex
            const auto& neighbor_ids = adjacency_list_[current_vertex_id];

            // iterate over all outgoing neighbors
            for (auto& neighbor_id : neighbor_ids) {
                if (!visited[neighbor_id]) {
                    stack.push(neighbor_id);
                } else if (on_stack[neighbor_id]) { // the current vertex is pointing to one of its ancestors
                    return true;
                }
            }
        }
    }

    // if no vertex in the graph points to at least one of its ancestors, the graph does not have any cycles
    return false;
}