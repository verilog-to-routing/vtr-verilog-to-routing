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

PathManager::~PathManager() {
    if (_is_enabled) free_all_memory();
}

bool PathManager::node_exists_in_tree(t_heap_path* path_data,
                                      RRNodeId& to_node) {
    // Prevent seg faults for searching path data structures that haven't been created yet
    if (!path_data || !_is_enabled) return false;

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
    if (!_is_enabled) return;

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

void PathManager::alloc_path_struct(t_heap_path*& tptr) {
    // TODO: Use arena allocation for this part
    
    // If RCV isn't enabled return a nullptr
    if (!_is_enabled) {
        return;
    }

    // if (tptr == nullptr) {
        // Use a free node list to avoid unnecessary data allocation
        // If there are unused data structures in memory use these
        if (_freed_nodes.size() > 0) {
            tptr = _freed_nodes.back();
            _freed_nodes.pop_back();
        } else {
            tptr = new t_heap_path;
            _alloc_list.push_back(tptr);
        }
    // }
    
    tptr->path_rr.clear();
    tptr->edge.clear();
    tptr->backward_cong = 0.;
    tptr->backward_delay = 0.;
}

void PathManager::free_path_struct(t_heap_path*& tptr) {
    if (!_is_enabled) return;

    // Put freed structs in a freed array to be used later
    if (tptr != nullptr) {
        _freed_nodes.push_back(tptr);
        tptr = nullptr;
    }
}

void PathManager::free_all_memory() {
    if (!_is_enabled) return;

    // Only delete freed nodes because doing a partial arena alloc scheme
    // Actually delete in heap_type
    for (t_heap_path* node : _alloc_list) {
        if (node != nullptr) delete node;
    }
}

void PathManager::empty_heap() {
    // Don't use this, it's a significant time sink
    return;
    if (!_is_enabled) return;

    _freed_nodes.clear();

    // Push all nodes onto the freed_nodes list
    for (auto& node : _alloc_list)
        _freed_nodes.push_back(node);
}

void PathManager::empty_route_tree_nodes() {
    if (!_is_enabled) return;

    route_tree_nodes.clear();
}