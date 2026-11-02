#include <vector>

#include "catch2/catch_test_macros.hpp"

#include "logic_block_location_util.h"
#include "physical_types.h"

// Hand-built pb graph used to check traversal order:
//
//   root (2 modes)
//     mode 0: pb type a, 2 instances, primitive
//     mode 1: pb type b, 1 instance, 1 mode
//               mode 0: pb type c, 2 instances, primitive
//
// Expected order: root, a[0], a[1], b[0], c[0], c[1]. LbHierPathParser uses
// the first matching node to resolve logical_block_location constraints,
// so this order matters when a path matches several nodes.
TEST_CASE("FindPbGraphNodeIf", "[vpr]") {
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

    const std::vector<const t_pb_graph_node*> pre_order = {&root, &a[0], &a[1], &b, &c[0], &c[1]};

    SECTION("a null root visits nothing") {
        size_t num_visited = 0;
        const t_pb_graph_node* found = find_pb_graph_node_if(nullptr, [&](const t_pb_graph_node&) {
            num_visited++;
            return true;
        });
        REQUIRE(found == nullptr);
        REQUIRE(num_visited == 0);
    }

    SECTION("the root is visited first") {
        size_t num_visited = 0;
        const t_pb_graph_node* found = find_pb_graph_node_if(&root, [&](const t_pb_graph_node&) {
            num_visited++;
            return true;
        });
        REQUIRE(found == &root);
        REQUIRE(num_visited == 1);
    }

    SECTION("a predicate that never matches visits every node once, in pre-order") {
        std::vector<const t_pb_graph_node*> visited;
        const t_pb_graph_node* found = find_pb_graph_node_if(&root, [&](const t_pb_graph_node& node) {
            visited.push_back(&node);
            return false;
        });
        REQUIRE(found == nullptr);
        REQUIRE(visited == pre_order);
    }

    SECTION("the walk stops at the first match") {
        std::vector<const t_pb_graph_node*> visited;
        const t_pb_graph_node* found = find_pb_graph_node_if(&root, [&](const t_pb_graph_node& node) {
            visited.push_back(&node);
            return node.is_primitive();
        });
        REQUIRE(found == &a[0]);
        REQUIRE(visited == std::vector<const t_pb_graph_node*>{&root, &a[0]});
    }

    SECTION("nested instances are reached through their parent's mode") {
        const t_pb_graph_node* found = find_pb_graph_node_if(&root, [&](const t_pb_graph_node& node) {
            return node.pb_type == &prim_c;
        });
        REQUIRE(found == &c[0]);
    }
}
