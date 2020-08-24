#ifndef EDGE_GROUPS_H
#define EDGE_GROUPS_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstddef>

#include "vpr_types.h"
#include "vpr_context.h"

// Class for building a group of connected edges.
class EdgeGroups {
  public:
    EdgeGroups() {}

    // Adds non-configurable (undirected) edge to be grouped.
    //
    // Returns true if this is a new edge.
    bool add_non_config_edge(int from_node, int to_node);

    // After add_non_config_edge has been called for all edges, create_sets
    // will form groups of nodes that are connected via non-configurable
    // edges.
    void create_sets();

    // Create t_non_configurable_rr_sets from set data.
    // NOTE: The stored graph is undirected, so this may generate reverse edges that don't exist.
    t_non_configurable_rr_sets output_sets();

    // Set device context structures for non-configurable node sets.
    void set_device_context(DeviceContext& device_ctx);

  private:
    struct node_data {
        std::unordered_set<int> edges;
        int set = OPEN;
    };

    // Perform a DFS traversal marking everything reachable with the same set id
    size_t add_connected_group(const node_data& node);

    // Set of non-configurable edges.
    std::unordered_map<int, node_data> graph_;

    // Compact set of node sets. Map key is arbitrary.
    std::vector<std::vector<int>> rr_non_config_node_sets_map_;
};

#endif
