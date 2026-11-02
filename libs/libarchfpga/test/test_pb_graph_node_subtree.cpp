#include <algorithm>
#include <iterator>
#include <vector>

#include "catch2/catch_test_macros.hpp"

#include "physical_types.h"

// The walk must work with range-based for loops and std::ranges algorithms.
static_assert(std::input_iterator<t_pb_graph_node::SubtreeIterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, t_pb_graph_node::SubtreeIterator>);

// Hand-built pb graph used to check traversal order:
//
//   root (2 modes)
//     mode 0: pb type a, 2 instances, primitive
//     mode 1: pb type b, 1 instance, 1 mode
//               mode 0: pb type c, 2 instances, primitive
//
// Expected order: root, a[0], a[1], b[0], c[0], c[1]. Callers that take the
// first matching node, such as logical_block_location lookups, depend on it.
TEST_CASE("Pb graph node subtree", "[archfpga]") {
    t_pb_type prim_a{};
    prim_a.num_pb = 2;
    t_pb_type prim_c{};
    prim_c.num_pb = 2;

    t_mode b_mode{};
    b_mode.pb_type_children = &prim_c;
    b_mode.num_pb_type_children = 1;
    t_pb_type type_b{};
    type_b.num_pb = 1;
    type_b.modes = &b_mode;
    type_b.num_modes = 1;

    t_mode root_modes[2]{};
    root_modes[0].pb_type_children = &prim_a;
    root_modes[0].num_pb_type_children = 1;
    root_modes[1].pb_type_children = &type_b;
    root_modes[1].num_pb_type_children = 1;
    t_pb_type root_type{};
    root_type.modes = root_modes;
    root_type.num_modes = 2;

    // child_pb_graph_nodes is indexed [mode][child pb type][instance]. Nodes
    // cannot be copied, so each level is built in place.
    t_pb_graph_node root{};
    root.pb_type = &root_type;
    root.child_pb_graph_nodes = std::vector<std::vector<std::vector<t_pb_graph_node>>>(2);
    root.child_pb_graph_nodes[0] = std::vector<std::vector<t_pb_graph_node>>(1);
    root.child_pb_graph_nodes[0][0] = std::vector<t_pb_graph_node>(2);
    root.child_pb_graph_nodes[1] = std::vector<std::vector<t_pb_graph_node>>(1);
    root.child_pb_graph_nodes[1][0] = std::vector<t_pb_graph_node>(1);

    t_pb_graph_node& b = root.child_pb_graph_nodes[1][0][0];
    b.pb_type = &type_b;
    b.child_pb_graph_nodes = std::vector<std::vector<std::vector<t_pb_graph_node>>>(1);
    b.child_pb_graph_nodes[0] = std::vector<std::vector<t_pb_graph_node>>(1);
    b.child_pb_graph_nodes[0][0] = std::vector<t_pb_graph_node>(2);

    std::vector<t_pb_graph_node>& a = root.child_pb_graph_nodes[0][0];
    std::vector<t_pb_graph_node>& c = b.child_pb_graph_nodes[0][0];
    for (t_pb_graph_node& node : a)
        node.pb_type = &prim_a;
    for (t_pb_graph_node& node : c)
        node.pb_type = &prim_c;

    auto collect = [](const t_pb_graph_node& start) {
        std::vector<const t_pb_graph_node*> visited;
        for (const t_pb_graph_node& node : start.subtree())
            visited.push_back(&node);
        return visited;
    };

    SECTION("a primitive's subtree is the node itself") {
        REQUIRE(collect(a[1]) == std::vector<const t_pb_graph_node*>{&a[1]});
    }

    SECTION("every node is visited once, in pre-order") {
        REQUIRE(collect(root) == std::vector<const t_pb_graph_node*>{&root, &a[0], &a[1], &b, &c[0], &c[1]});
    }

    SECTION("a child's subtree stays inside the child") {
        REQUIRE(collect(b) == std::vector<const t_pb_graph_node*>{&b, &c[0], &c[1]});
    }

    SECTION("a search returns the first match in pre-order") {
        auto primitive = std::ranges::find_if(root.subtree(), [](const t_pb_graph_node& node) noexcept {
            return node.is_primitive();
        });
        REQUIRE(&*primitive == &a[0]);
    }

    SECTION("nested instances are reached through their parent's mode") {
        auto found = std::ranges::find_if(root.subtree(), [&](const t_pb_graph_node& node) noexcept {
            return node.pb_type == &prim_c;
        });
        REQUIRE(&*found == &c[0]);
    }
}
