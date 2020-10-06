#include "catch.hpp"

#include "vpr_api.h"
#include "vpr_signal_handler.h"
#include "globals.h"
#include "net_delay.h"
#include "place_and_route.h"
#include "route_tree_timing.h"
#include "timing_place_lookup.h"

static constexpr const char kArchFile[] = "../../vtr_flow/arch/timing/k6_frac_N10_mem32K_40nm.xml";
static constexpr int kMaxHops = 10;

namespace {

// Route from source_node to sink_node, returning either the delay, or infinity if unroutable.
static float do_one_route(int source_node, int sink_node, const t_router_opts& router_opts, const std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    t_rt_node* rt_root = init_route_tree_to_source_no_net(source_node);

    // Update base costs according to fanout and criticality rules.
    update_rr_base_costs(1);

    // Bounding box includes the entire grid.
    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;

    t_conn_cost_params cost_params;
    cost_params.criticality = router_opts.max_criticality;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;

    route_budgets budgeting_inf;

    RouterStats router_stats;
    auto router_lookahead = make_router_lookahead(
        router_opts.lookahead_type,
        router_opts.write_router_lookahead,
        router_opts.read_router_lookahead,
        segment_inf);

    ConnectionRouter<BinaryHeap> router(
        device_ctx.grid,
        *router_lookahead,
        device_ctx.rr_nodes,
        device_ctx.rr_rc_data,
        device_ctx.rr_switch_inf,
        g_vpr_ctx.mutable_routing().rr_node_route_inf);

    // Find the cheapest route if possible.
    bool found_path;
    t_heap cheapest;
    std::tie(found_path, cheapest) = router.timing_driven_route_connection_from_route_tree(rt_root, sink_node, cost_params, bounding_box, router_stats);

    // Default delay is infinity, which indicates that a route was not found.
    float delay = std::numeric_limits<float>::infinity();
    if (found_path) {
        // Check that the route goes to the requested sink.
        REQUIRE(cheapest.index == sink_node);

        // Get the delay
        t_rt_node* rt_node_of_sink = update_route_tree(&cheapest, OPEN, nullptr);
        delay = rt_node_of_sink->Tdel;

        // Clean up
        free_route_tree(rt_root);
    }

    // Reset for the next router call.
    router.reset_path_costs();
    return delay;
}

// Find a source and a sink by walking edges.
std::tuple<size_t, size_t, int> find_source_and_sink() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    // Current longest walk
    std::tuple<size_t, size_t, int> longest = std::make_tuple(0, 0, 0);

    // Start from each RR node
    for (size_t id = 0; id < rr_graph.size(); id++) {
        RRNodeId source(id), sink = source;
        for (int hops = 0; hops < kMaxHops; hops++) {
            // Take the first edge, if there is one.
            auto edge = rr_graph.first_edge(sink);
            if (edge == rr_graph.last_edge(sink)) {
                break;
            }
            sink = rr_graph.edge_sink_node(edge);

            // If this is the new longest walk, store it.
            if (hops > std::get<2>(longest)) {
                longest = std::make_tuple(size_t(source), size_t(sink), hops);
            }
        }
    }
    return longest;
}

// Test that the router can route nets individually, not considering congestion.
// This is a minimal timing driven routing test that can be used as documentation,
// and as a starting point for experimentation.
TEST_CASE("connection_router", "[vpr]") {
    // Minimal setup
    auto options = t_options();
    auto arch = t_arch();
    auto vpr_setup = t_vpr_setup();

    vpr_install_signal_handler();
    vpr_initialize_logging();

    // Command line arguments
    const char* argv[] = {
        "test_vpr",
        kArchFile,
        "wire.eblif",
        "--route_chan_width", "100"};
    vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
             &options, &vpr_setup, &arch);

    vpr_create_device_grid(vpr_setup, arch);
    vpr_setup_clock_networks(vpr_setup, arch);
    auto chan_width = init_chan(vpr_setup.RouterOpts.fixed_channel_width, arch.Chans);

    alloc_routing_structs(
        chan_width,
        vpr_setup.RouterOpts,
        &vpr_setup.RoutingArch,
        vpr_setup.Segments,
        arch.Directs,
        arch.num_directs);

    // Find a source and sink to route
    int source_rr_node, sink_rr_node, hops;
    std::tie(source_rr_node, sink_rr_node, hops) = find_source_and_sink();

    // Check that the route will be non-trivial
    REQUIRE(source_rr_node != sink_rr_node);
    REQUIRE(hops >= 3);

    // Find the route
    float delay = do_one_route(
        source_rr_node,
        sink_rr_node,
        vpr_setup.RouterOpts,
        vpr_setup.Segments);

    // Check that a route was found
    REQUIRE(delay < std::numeric_limits<float>::infinity());

    // Clean up
    free_routing_structs();
    vpr_free_all(arch, vpr_setup);
}

} // namespace
