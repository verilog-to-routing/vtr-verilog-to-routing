#include "timing_reports.h"

#include <fstream>
#include <sstream>

#include "timing_reports.h"
#include "rr_graph.h"

#include "tatum/TimingReporter.hpp"

#include "vtr_version.h"
#include "vpr_types.h"
#include "globals.h"

#include "timing_info.h"
#include "timing_util.h"

#include "VprTimingGraphResolver.h"

/**
 * @brief Get the bounding box of a net.
 * If the net is completely absorbed into a cluster block, return the bounding box of the cluster block.
 * Otherwise, return the bounding box of the net's route tree.
 * If a net is not routed, bounding box is returned with default values (OPEN).
 * 
 * @param atom_net_id The id of the atom net to get the bounding box of.
 */
static t_bb get_net_bounding_box(const AtomNetId atom_net_id) {
    const auto& route_trees = g_vpr_ctx.routing().route_trees;
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    const bool flat_router = g_vpr_ctx.routing().is_flat;

    // Lambda to get the bounding box of a route tree
    auto route_tree_bb = [](const RRGraphView& rr_graph, const RouteTree& route_tree) {
        t_bb bb;

        // Set the initial bounding box to the root node's location
        RRNodeId route_tree_root = route_tree.root().inode;
        bb.xmin = rr_graph.node_xlow(route_tree_root);
        bb.xmax = rr_graph.node_xhigh(route_tree_root);
        bb.ymin = rr_graph.node_ylow(route_tree_root);
        bb.ymax = rr_graph.node_yhigh(route_tree_root);
        bb.layer_min = rr_graph.node_layer(route_tree_root);
        bb.layer_max = rr_graph.node_layer(route_tree_root);

        // Iterate over all nodes in the route tree and update the bounding box
        for (auto& rt_node : route_tree.all_nodes()) {
            RRNodeId inode = rt_node.inode;
            if (rr_graph.node_xlow(inode) < bb.xmin)
                bb.xmin = rr_graph.node_xlow(inode);
            if (rr_graph.node_xhigh(inode) > bb.xmax)
                bb.xmax = rr_graph.node_xhigh(inode);
            if (rr_graph.node_ylow(inode) < bb.ymin)
                bb.ymin = rr_graph.node_ylow(inode);
            if (rr_graph.node_layer(inode) > bb.layer_min)
                bb.layer_min = rr_graph.node_layer(inode);
            if (rr_graph.node_layer(inode) > bb.layer_max)
                bb.layer_max = rr_graph.node_layer(inode);
        }
        return bb;
    };

    if (flat_router) {
        // If flat router is used, route tree data structure can be used
        // directly to get the bounding box of the net
        const auto& route_tree = route_trees[atom_net_id];
        if (!route_tree)
            return t_bb();
        return route_tree_bb(rr_graph, *route_tree);
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
                bbs.push_back(route_tree_bb(rr_graph, *route_tree));
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
    /* Create a report file for net timing information */
    std::ofstream os(prefix + "report_net_timing.rpt");
    const auto& atom_netlist = g_vpr_ctx.atom().netlist();
    const auto& atom_lookup = g_vpr_ctx.atom().lookup();

    const auto& timing_ctx = g_vpr_ctx.timing();
    const auto& timing_graph = timing_ctx.graph;

    os << "# This file is generated by VTR" << std::endl;
    os << "# Version: " << vtr::VERSION << std::endl;
    os << "# Revision: " << vtr::VCS_REVISION << std::endl;
    os << "# For each net, the timing information is reported in the following format:" << std::endl;
    os << "# netname : Fanout : source_instance <slack_on source pin> : "
       << "<load pin name1> <slack on load pin name1> <net delay for this net> : "
       << "<load pin name2> <slack on load pin name2> <net delay for this net> : ..."
       << std::endl;

    os << std::endl;

    for (const auto& net : atom_netlist.nets()) {
        /* Skip constant nets */
        if (atom_netlist.net_is_constant(net)) {
            continue;
        }

        const auto& net_name = atom_netlist.net_name(net);

        /* Get source pin and its timing information */
        const auto& source_pin = *atom_netlist.net_pins(net).begin();
        auto source_pin_slack = timing_info.setup_pin_slack(source_pin);
        /* Timing graph node id corresponding to the net's source pin */
        auto tg_source_node = atom_lookup.atom_pin_tnode(source_pin);
        VTR_ASSERT(tg_source_node.is_valid());

        const size_t fanout = atom_netlist.net_sinks(net).size();
        os << net_name << " : " 
           << fanout << " : " 
           << atom_netlist.pin_name(source_pin).c_str() << " " << source_pin_slack << " : ";

        /* Iterate over all fanout pins and print their timing information */
        for (size_t net_pin_index = 1; net_pin_index <= fanout; ++net_pin_index) {
            const auto& pin = *(atom_netlist.net_pins(net).begin() + net_pin_index);

            /* Get timing graph node id corresponding to the fanout pin */
            const auto& tg_sink_node = atom_lookup.atom_pin_tnode(pin);
            VTR_ASSERT(tg_sink_node.is_valid());

            /* Get timing graph edge id between atom pins */
            const auto& tg_edge_id = timing_graph->find_edge(tg_source_node, tg_sink_node);
            VTR_ASSERT(tg_edge_id.is_valid());

            /* Get timing information for the fanout pin */
            const auto& pin_setup_slack = timing_info.setup_pin_slack(pin);
            const auto& pin_delay = delay_calc.max_edge_delay(*timing_graph, tg_edge_id);

            const auto& pin_name = atom_netlist.pin_name(pin);
            os << pin_name << " " << std::scientific << pin_setup_slack << " " << pin_delay;
            if (net_pin_index < fanout) {
                os << " : ";
            }
        }
        os << "," << std::endl;
    }
}
