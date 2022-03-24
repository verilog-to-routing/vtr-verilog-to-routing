#include <tuple>
#include "catch2/catch_test_macros.hpp"

#include "rr_graph_fwd.h"
#include "vpr_api.h"
#include "vpr_signal_handler.h"
#include "globals.h"
#include "net_delay.h"
#include "place_and_route.h"
#include "timing_place_lookup.h"

static constexpr const char kArchFile[] = "../../vtr_flow/arch/timing/k6_frac_N10_mem32K_40nm.xml";
static constexpr int kMaxHops = 10;

namespace {

// Route from source_node to sink_node, returning either the delay, or infinity if unroutable.
static float do_one_route(RRNodeId source_node,
                          RRNodeId sink_node,
                          const t_det_routing_arch& det_routing_arch,
                          const t_router_opts& router_opts,
                          const std::vector<t_segment_inf>& segment_inf) {
    bool is_flat = router_opts.flat_routing;
    auto& device_ctx = g_vpr_ctx.device();

    RouteTree tree((RRNodeId(source_node)));

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

    const Netlist<>& net_list = is_flat ? (const Netlist<>&)g_vpr_ctx.atom().nlist : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;
    route_budgets budgeting_inf(net_list, is_flat);

    RouterStats router_stats;
    auto router_lookahead = make_router_lookahead(det_routing_arch,
                                                  router_opts.lookahead_type,
                                                  router_opts.write_router_lookahead,
                                                  router_opts.read_router_lookahead,
                                                  segment_inf,
                                                  is_flat);

    ConnectionRouter<BinaryHeap> router(
        device_ctx.grid,
        *router_lookahead,
        device_ctx.rr_graph.rr_nodes(),
        &device_ctx.rr_graph,
        device_ctx.rr_rc_data,
        device_ctx.rr_graph.rr_switch(),
        g_vpr_ctx.mutable_routing().rr_node_route_inf,
        is_flat);

    // Find the cheapest route if possible.
    bool found_path;
    t_heap cheapest;
    ConnectionParameters conn_params(ParentNetId::INVALID(),
                                     -1,
                                     false,
                                     std::unordered_map<RRNodeId, int>());
    std::tie(found_path, std::ignore, cheapest) = router.timing_driven_route_connection_from_route_tree(tree.root(),
                                                                                                        sink_node,
                                                                                                        cost_params,
                                                                                                        bounding_box,
                                                                                                        router_stats,
                                                                                                        conn_params,
                                                                                                        true);

    // Default delay is infinity, which indicates that a route was not found.
    float delay = std::numeric_limits<float>::infinity();
    if (found_path) {
        // Check that the route goes to the requested sink.
        REQUIRE(RRNodeId(cheapest.index) == sink_node);

        // Get the delay
        vtr::optional<const RouteTreeNode&> rt_node_of_sink;
        std::tie(std::ignore, rt_node_of_sink) = tree.update_from_heap(&cheapest, OPEN, nullptr, router_opts.flat_routing);
        delay = rt_node_of_sink.value().Tdel;
    }

    // Reset for the next router call.
    router.reset_path_costs();
    return delay;
}

// Find a source and a sink by walking edges.
std::tuple<RRNodeId, RRNodeId, int> find_source_and_sink() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    // Current longest walk
    std::tuple<RRNodeId, RRNodeId, int> longest = std::make_tuple(RRNodeId::INVALID(), RRNodeId::INVALID(), 0);

    // Start from each RR node
    for (size_t id = 0; id < rr_graph.num_nodes(); id++) {
        RRNodeId source(id), sink = source;
        for (int hops = 0; hops < kMaxHops; hops++) {
            // Take the first edge, if there is one.
            auto edge = rr_graph.node_first_edge(sink);
            if (edge == rr_graph.node_last_edge(sink)) {
                break;
            }
            sink = rr_graph.rr_nodes().edge_sink_node(edge);

            // If this is the new longest walk, store it.
            if (hops > std::get<2>(longest)) {
                longest = std::make_tuple(source, sink, hops);
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
    auto det_routing_arch = &vpr_setup.RoutingArch;
    auto& router_opts = vpr_setup.RouterOpts;
    t_graph_type graph_directionality;

    if (router_opts.route_type == GLOBAL) {
        graph_directionality = GRAPH_BIDIR;
    } else {
        graph_directionality = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    auto chan_width = init_chan(vpr_setup.RouterOpts.fixed_channel_width, arch.Chans, graph_directionality);

    alloc_routing_structs(
        chan_width,
        vpr_setup.RouterOpts,
        &vpr_setup.RoutingArch,
        vpr_setup.Segments,
        arch.Directs,
        arch.num_directs,
        router_opts.flat_routing);

    // Find a source and sink to route
    RRNodeId source_rr_node, sink_rr_node;
    int hops;
    std::tie(source_rr_node, sink_rr_node, hops) = find_source_and_sink();

    // Check that the route will be non-trivial
    REQUIRE(source_rr_node != sink_rr_node);
    REQUIRE(hops >= 3);

    // Find the route
    float delay = do_one_route(source_rr_node,
                               sink_rr_node,
                               vpr_setup.RoutingArch,
                               vpr_setup.RouterOpts,
                               vpr_setup.Segments);

    // Check that a route was found
    REQUIRE(delay < std::numeric_limits<float>::infinity());

    // Clean up
    free_routing_structs();
    vpr_free_all(arch,
                 vpr_setup);

    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    free_pack_molecules(atom_ctx.list_of_pack_molecules.release());
    atom_ctx.atom_molecules.clear();
}

} // namespace
