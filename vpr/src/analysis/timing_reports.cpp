#include "timing_reports.h"

#include <fstream>

#include "timing_reports.h"

#include "tatum/TimingReporter.hpp"

#include "vpr_types.h"
#include "globals.h"

#include "timing_info.h"
#include "timing_util.h"

#include "VprTimingGraphResolver.h"

/**
 * @brief Get the bounding box of a routed net.
 * If the net is completely absorbed into a cluster block, return the bounding box of the cluster block.
 * Otherwise, return the bounding box of the net's route tree.
 * 
 * @param atom_net_id The id of the atom net to get the bounding box of.
 * 
 * @return The bounding box of the net. If the net is not routed, a bounding box 
 * is returned with default values (OPEN).
 */
static t_bb get_net_bounding_box(const AtomNetId atom_net_id) {
    const auto& route_trees = g_vpr_ctx.routing().route_trees;
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;

    // Lambda to get the bounding box of a route tree
    auto route_tree_bb = [&](const RouteTree& route_tree) {
        t_bb bb;

        // Set the initial bounding box to the root node's location
        RRNodeId route_tree_root = route_tree.root().inode;
        bb.xmin = rr_graph.node_xlow(route_tree_root);
        bb.xmax = rr_graph.node_xhigh(route_tree_root);
        bb.ymin = rr_graph.node_ylow(route_tree_root);
        bb.ymax = rr_graph.node_yhigh(route_tree_root);
        bb.layer_min = rr_graph.node_layer_low(route_tree_root);
        bb.layer_max = rr_graph.node_layer_high(route_tree_root);

        // Iterate over all nodes in the route tree and update the bounding box
        for (auto& rt_node : route_tree.all_nodes()) {
            RRNodeId inode = rt_node.inode;

            bb.xmin = std::min(static_cast<int>(rr_graph.node_xlow(inode)), bb.xmin);
            bb.xmax = std::max(static_cast<int>(rr_graph.node_xhigh(inode)), bb.xmax);

            bb.ymin = std::min(static_cast<int>(rr_graph.node_ylow(inode)), bb.ymin);
            bb.ymax = std::max(static_cast<int>(rr_graph.node_yhigh(inode)), bb.ymax);

            bb.layer_min = std::min(static_cast<int>(rr_graph.node_layer_low(inode)), bb.layer_min);
            bb.layer_max = std::max(static_cast<int>(rr_graph.node_layer_high(inode)), bb.layer_max);
        }
        return bb;
    };

    if (g_vpr_ctx.routing().is_flat) {
        // If flat router is used, route tree data structure can be used
        // directly to get the bounding box of the net
        const auto& route_tree = route_trees[atom_net_id];
        if (!route_tree)
            return t_bb();
        return route_tree_bb(*route_tree);
    } else {
        // If two-stage router is used, we need to first get the cluster net id
        // corresponding to the atom net and then get the bounding box of the net
        // from the route tree. If the net is completely absorbed into a cluster block,
        const auto& atom_lookup = g_vpr_ctx.atom().lookup();
        const auto& cluster_net_id = atom_lookup.clb_nets(atom_net_id);
        std::vector<t_bb> bbs;
        t_bb max_bb;
        // There maybe multiple cluster nets corresponding to a single atom net.
        // We iterate over all cluster nets and the final bounding box is the union
        // of all cluster net bounding boxes
        if (cluster_net_id != vtr::nullopt) {
            for (const auto& clb_net_id : *cluster_net_id) {
                const auto& route_tree = route_trees[clb_net_id];
                if (!route_tree)
                    continue;
                bbs.push_back(route_tree_bb(*route_tree));
            }
            if (bbs.empty()) {
                return t_bb();
            }
            // Assign the first cluster net's bounding box to the final bounding box
            // and then iteratively update it with the union of bounding boxes of
            // all cluster nets
            max_bb = bbs[0];
            for (size_t i = 1; i < bbs.size(); ++i) {
                max_bb.xmin = std::min(bbs[i].xmin, max_bb.xmin);
                max_bb.xmax = std::max(bbs[i].xmax, max_bb.xmax);
                max_bb.ymin = std::min(bbs[i].ymin, max_bb.ymin);
                max_bb.ymax = std::max(bbs[i].ymax, max_bb.ymax);
                max_bb.layer_min = std::min(bbs[i].layer_min, max_bb.layer_min);
                max_bb.layer_max = std::max(bbs[i].layer_max, max_bb.layer_max);
            }
            return max_bb;
        } else {
            // If there is no cluster net corresponding to the atom net,
            // it means the net is completely absorbed into a cluster block.
            // In that case, we set the bounding box the cluster block's bounding box
            const auto& atom_ctx = g_vpr_ctx.atom();
            const auto& atom_nlist = atom_ctx.netlist();
            AtomPinId source_pin = atom_nlist.net_driver(atom_net_id);

            AtomBlockId atom_block = atom_nlist.pin_block(source_pin);
            VTR_ASSERT(atom_block != AtomBlockId::INVALID());
            ClusterBlockId cluster_block = atom_lookup.atom_clb(atom_block);
            VTR_ASSERT(cluster_block != ClusterBlockId::INVALID());

            const t_pl_loc& cluster_block_loc = g_vpr_ctx.placement().block_locs()[cluster_block].loc;
            const auto& grid = g_vpr_ctx.device().grid;
            vtr::Rect<int> tile_bb = grid.get_tile_bb({cluster_block_loc.x, cluster_block_loc.y, cluster_block_loc.layer});
            const int block_layer = cluster_block_loc.layer;
            return t_bb(tile_bb.xmin(),
                        tile_bb.xmax(),
                        tile_bb.ymin(),
                        tile_bb.ymax(),
                        block_layer,
                        block_layer);
        }
    }
}

void generate_setup_timing_stats(const std::string& prefix,
                                 const SetupTimingInfo& timing_info,
                                 const AnalysisDelayCalculator& delay_calc,
                                 const t_analysis_opts& analysis_opts,
                                 bool is_flat,
                                 const BlkLocRegistry& blk_loc_registry) {
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();
    const LogicalModels& models = g_vpr_ctx.device().arch->models;

    print_setup_timing_summary(*timing_ctx.constraints, *timing_info.setup_analyzer(), "Final ", analysis_opts.write_timing_summary);

    VprTimingGraphResolver resolver(atom_ctx.netlist(), atom_ctx.lookup(), models, *timing_ctx.graph, delay_calc, is_flat, blk_loc_registry);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    timing_reporter.report_timing_setup(prefix + "report_timing.setup.rpt", *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);

    if (analysis_opts.timing_report_skew) {
        timing_reporter.report_skew_setup(prefix + "report_skew.setup.rpt", *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
    }

    timing_reporter.report_unconstrained_setup(prefix + "report_unconstrained_timing.setup.rpt", *timing_info.setup_analyzer());
}

void generate_hold_timing_stats(const std::string& prefix,
                                const HoldTimingInfo& timing_info,
                                const AnalysisDelayCalculator& delay_calc,
                                const t_analysis_opts& analysis_opts,
                                bool is_flat,
                                const BlkLocRegistry& blk_loc_registry) {
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();
    const LogicalModels& models = g_vpr_ctx.device().arch->models;

    print_hold_timing_summary(*timing_ctx.constraints, *timing_info.hold_analyzer(), "Final ");

    VprTimingGraphResolver resolver(atom_ctx.netlist(), atom_ctx.lookup(), models, *timing_ctx.graph, delay_calc, is_flat, blk_loc_registry);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    timing_reporter.report_timing_hold(prefix + "report_timing.hold.rpt", *timing_info.hold_analyzer(), analysis_opts.timing_report_npaths);

    if (analysis_opts.timing_report_skew) {
        timing_reporter.report_skew_hold(prefix + "report_skew.hold.rpt", *timing_info.hold_analyzer(), analysis_opts.timing_report_npaths);
    }

    timing_reporter.report_unconstrained_hold(prefix + "report_unconstrained_timing.hold.rpt", *timing_info.hold_analyzer());
}

void generate_net_timing_report(const std::string& prefix,
                                const SetupHoldTimingInfo& timing_info,
                                const AnalysisDelayCalculator& delay_calc) {
    std::ofstream os(prefix + "report_net_timing.csv");
    const auto& atom_netlist = g_vpr_ctx.atom().netlist();
    const auto& atom_lookup = g_vpr_ctx.atom().lookup();
    const auto& timing_ctx = g_vpr_ctx.timing();
    const auto& timing_graph = timing_ctx.graph;

    // Write CSV header
    os << "netname,Fanout,bb_xmin,bb_ymin,bb_layer_min,"
       << "bb_xmax,bb_ymax,bb_layer_max,"
       << "src_pin_name,src_pin_slack,sinks" << std::endl;

    for (const auto& net : atom_netlist.nets()) {
        const auto& net_name = atom_netlist.net_name(net);
        const auto& source_pin = *atom_netlist.net_pins(net).begin();
        // for the driver/source, this is the worst slack to any fanout.
        auto source_pin_slack = timing_info.setup_pin_slack(source_pin);
        auto tg_source_node = atom_lookup.atom_pin_tnode(source_pin);
        VTR_ASSERT(tg_source_node.is_valid());

        const size_t fanout = atom_netlist.net_sinks(net).size();
        const auto& net_bb = get_net_bounding_box(net);

        os << "\"" << net_name << "\"," // netname (quoted for safety)
           << fanout << ","
           << net_bb.xmin << "," << net_bb.ymin << "," << net_bb.layer_min << ","
           << net_bb.xmax << "," << net_bb.ymax << "," << net_bb.layer_max << ","
           << "\"" << atom_netlist.pin_name(source_pin) << "\"," << source_pin_slack << ",";

        // Write sinks column (quoted, semicolon-delimited, each sink: name,slack,delay)
        os << "\"";
        for (size_t i = 0; i < fanout; ++i) {
            const auto& pin = *(atom_netlist.net_pins(net).begin() + i + 1);
            auto tg_sink_node = atom_lookup.atom_pin_tnode(pin);
            VTR_ASSERT(tg_sink_node.is_valid());

            auto tg_edge_id = timing_graph->find_edge(tg_source_node, tg_sink_node);
            VTR_ASSERT(tg_edge_id.is_valid());

            auto pin_setup_slack = timing_info.setup_pin_slack(pin);
            auto pin_delay = delay_calc.max_edge_delay(*timing_graph, tg_edge_id);
            const auto& pin_name = atom_netlist.pin_name(pin);

            os << pin_name << "," << pin_setup_slack << "," << pin_delay;
            if (i != fanout - 1) os << ";";
        }
        os << "\"" << std::endl; // Close quoted sinks field and finish the row
    }
}
