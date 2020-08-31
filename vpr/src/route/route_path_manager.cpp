#include "route_path_manager.h"
#include "globals.h"

PathManager::PathManager(bool init_rcv_structures) {
    // Only init data structure if required by RCV
    // TODO: Use arena allocation method in the future, which would be setup here

    if (init_rcv_structures) {
        _is_enabled = true;
    } else {
        _is_enabled = false;
    }
}

PathManager::~PathManager() {}

bool PathManager::node_exists_in_tree(t_heap_path* path_data,
                                      RRNodeId& to_node) {
    // Prevent seg faults for searching path data structures that haven't been created yet
    if (!path_data) return false;

    // First check the smaller current path, the ordering of these checks might effect runtime slightly
    for (auto& node : path_data->path_rr) {
        if (node == to_node) {
            return true;
        }
    }

    // Search through route tree set for nodes existance
    auto node_exists_in_route_tree = route_tree_nodes.find(to_node);

    if (node_exists_in_route_tree != route_tree_nodes.end()) {
        return true;
    }

    return false;
}

void PathManager::insert_node(RRNodeId node) {
    if (_is_enabled) {
        route_tree_nodes.insert(node);
    }
}

void PathManager::insert_backwards_path_into_traceback(t_heap_path* path_data, float cost, float backward_path_cost) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    
    for (unsigned i = 1; i < path_data->edge.size() - 1; i++) {
        size_t node_2 = (size_t) path_data->path_rr[i];
        RREdgeId edge = path_data->edge[i - 1];
        route_ctx.rr_node_route_inf[node_2].prev_node = (size_t) path_data->path_rr[i - 1];
        route_ctx.rr_node_route_inf[node_2].prev_edge = edge;
        route_ctx.rr_node_route_inf[node_2].path_cost = cost;
        route_ctx.rr_node_route_inf[node_2].backward_path_cost = backward_path_cost;
    }
}

bool PathManager::is_enabled() {
    return _is_enabled;
}