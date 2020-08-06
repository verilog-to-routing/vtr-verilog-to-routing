#include <vector>
#include <utility>
#include <cstddef>
#include <set>
#include <random>
#include <algorithm>

#include "catch.hpp"

#include "edge_groups.h"

namespace {

TEST_CASE("edge_groups_create_sets", "[vpr]") {
    // Construct a set of edges that result in these connected sets
    std::vector<std::set<int>> connected_sets{{{1, 2, 3, 4, 5, 6, 7, 8},
                                               {9, 0}}};
    int max_node_id = 0;
    std::vector<std::pair<int, int>> edges;
    for (auto set : connected_sets) {
        int last = *set.cbegin();
        std::for_each(std::next(set.cbegin()),
                      set.cend(),
                      [&](int node) {
                          edges.push_back(std::make_pair(last, node));
                          last = node;
                          max_node_id = std::max(max_node_id, node);
                      });
    }

    // Build the id map for node IDs
    std::vector<int> nodes(max_node_id + 1);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::mt19937 g(1);

    for (int i = 0; i < 1000; i++) {
        // Shuffle node IDs
        auto random_nodes = nodes;
        std::shuffle(random_nodes.begin(), random_nodes.end(), g);

        // Apply shuffled IDs to edges
        auto random_edges = edges;
        for (auto& edge : random_edges) {
            edge.first = random_nodes[edge.first];
            edge.second = random_nodes[edge.second];
        }

        // Shuffle edges
        std::shuffle(random_edges.begin(), random_edges.end(), g);

        // Add edges to the EdgeGroups object
        EdgeGroups groups;
        for (auto edge : random_edges) {
            groups.add_non_config_edge(edge.first, edge.second);
        }

        // The algorithm to test
        groups.create_sets();
        t_non_configurable_rr_sets sets = groups.output_sets();

        // Check for the expected sets
        for (auto set : connected_sets) {
            std::set<int> random_set;
            for (auto elem : set) {
                random_set.insert(random_nodes[elem]);
            }
            REQUIRE(sets.node_sets.find(random_set) != sets.node_sets.end());
        }
    }
}

} // namespace
