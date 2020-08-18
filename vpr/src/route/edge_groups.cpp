#include "edge_groups.h"

#include <stack>

// Adds non-configurable (undirected) edge to be grouped.
//
// Returns true if this is a new edge.
bool EdgeGroups::add_non_config_edge(int from_node, int to_node) {
    return graph_[from_node].edges.insert(to_node).second && graph_[to_node].edges.insert(from_node).second;
}

// After add_non_config_edge has been called for all edges, create_sets
// will form groups of nodes that are connected via non-configurable
// edges.
void EdgeGroups::create_sets() {
    rr_non_config_node_sets_.clear();

    // https://en.wikipedia.org/wiki/Component_(graph_theory)#Algorithms
    std::vector<size_t> group_size;
    for (auto& node : graph_) {
        if (node.second.set == OPEN) {
            node.second.set = group_size.size();
            group_size.push_back(add_connected_group(node.second));
        }
    }

    // Sanity check the node sets.
    for (const auto& node : graph_) {
        VTR_ASSERT(node.second.set != OPEN);
        for (const auto& e : node.second.edges) {
            int set = graph_[e].set;
            VTR_ASSERT(set == node.second.set);
        }
    }

    // Create compact set of sets.
    rr_non_config_node_sets_.resize(group_size.size());
    for (size_t i = 0; i < group_size.size(); i++) {
        rr_non_config_node_sets_[i].reserve(group_size[i]);
    }
    for (const auto& node : graph_) {
        rr_non_config_node_sets_[node.second.set].push_back(node.first);
    }
}

// Create t_non_configurable_rr_sets from set data.
// NOTE: The stored graph is undirected, so this may generate reverse edges that don't exist.
t_non_configurable_rr_sets EdgeGroups::output_sets() {
    t_non_configurable_rr_sets sets;
    for (const auto& nodes : rr_non_config_node_sets_) {
        std::set<t_node_edge> edge_set;
        std::set<int> node_set(nodes.begin(), nodes.end());

        for (const auto& src : node_set) {
            for (const auto& dest : graph_[src].edges) {
                edge_set.emplace(t_node_edge(src, dest));
            }
        }

        sets.node_sets.emplace(std::move(node_set));
        sets.edge_sets.emplace(std::move(edge_set));
    }

    return sets;
}

// Set device context structures for non-configurable node sets.
void EdgeGroups::set_device_context(DeviceContext& device_ctx) {
    std::vector<std::vector<int>> rr_non_config_node_sets;
    for (const auto& item : rr_non_config_node_sets_) {
        rr_non_config_node_sets.emplace_back(std::move(item));
    }

    std::unordered_map<int, int> rr_node_to_non_config_node_set;
    for (size_t set = 0; set < rr_non_config_node_sets.size(); ++set) {
        for (const auto inode : rr_non_config_node_sets[set]) {
            rr_node_to_non_config_node_set.insert(
                std::make_pair(inode, set));
        }
    }

    device_ctx.rr_non_config_node_sets = std::move(rr_non_config_node_sets);
    device_ctx.rr_node_to_non_config_node_set = std::move(rr_node_to_non_config_node_set);
}

// Perform a DFS traversal marking everything reachable with the same set id
size_t EdgeGroups::add_connected_group(const node_data& node) {
    // stack contains nodes with edges to mark with node.set
    // The set for each node must be set before pushing it onto the stack
    std::stack<const node_data*> stack;
    stack.push(&node);
    size_t n = 1;
    while (!stack.empty()) {
        auto top = stack.top();
        stack.pop();
        for (auto e : top->edges) {
            auto& next = graph_[e];
            if (next.set != node.set) {
                VTR_ASSERT(next.set == OPEN);
                n++;
                next.set = node.set;
                stack.push(&next);
            }
        }
    }
    return n;
}
