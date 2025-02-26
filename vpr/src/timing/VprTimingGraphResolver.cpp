#include "VprTimingGraphResolver.h"
#include "atom_netlist.h"
#include "atom_lookup.h"

VprTimingGraphResolver::VprTimingGraphResolver(const AtomNetlist& netlist,
                                               const AtomLookup& netlist_lookup,
                                               const tatum::TimingGraph& timing_graph,
                                               const AnalysisDelayCalculator& delay_calc,
                                               bool is_flat,
                                               const BlkLocRegistry& blk_loc_registry)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , timing_graph_(timing_graph)
    , delay_calc_(delay_calc)
    , is_flat_(is_flat)
    , blk_loc_registry_(blk_loc_registry) {}

std::string VprTimingGraphResolver::node_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);

    return netlist_.pin_name(pin);
}

std::string VprTimingGraphResolver::node_type_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    AtomBlockId blk = netlist_.pin_block(pin);

    std::string name = netlist_.block_model(blk)->name;

    if (detail_level() == e_timing_report_detail::AGGREGATED
        || detail_level() == e_timing_report_detail::DETAILED_ROUTING
        || detail_level() == e_timing_report_detail::DEBUG) {
        //Detailed report consist of the aggregated reported with a breakdown of inter-block routing
        //Annotate primitive grid location, if known
        auto& atom_ctx = g_vpr_ctx.atom();
        auto& block_locs = blk_loc_registry_.block_locs();
        ClusterBlockId cb = atom_ctx.lookup().atom_clb(blk);
        if (cb && block_locs.count(cb)) {
            int x = block_locs[cb].loc.x;
            int y = block_locs[cb].loc.y;
            int layer = block_locs[cb].loc.layer;
            name += " at (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(layer) + ")";
        }
        if (detail_level() == e_timing_report_detail::DEBUG) {
            name += " tnode(" + std::to_string(size_t(node)) + ")";
        }
    }

    return name;
}

tatum::EdgeDelayBreakdown VprTimingGraphResolver::edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType tatum_delay_type) const {
    tatum::EdgeDelayBreakdown delay_breakdown;

    if (edge && (detail_level() == e_timing_report_detail::AGGREGATED || detail_level() == e_timing_report_detail::DETAILED_ROUTING || detail_level() == e_timing_report_detail::DEBUG)) {
        auto edge_type = timing_graph_.edge_type(edge);

        DelayType delay_type; //TODO: should unify vpr/tatum DelayType
        if (tatum_delay_type == tatum::DelayType::MAX) {
            delay_type = DelayType::MAX;
        } else {
            VTR_ASSERT(tatum_delay_type == tatum::DelayType::MIN);
            delay_type = DelayType::MIN;
        }

        if (edge_type == tatum::EdgeType::INTERCONNECT) {
            delay_breakdown.components = interconnect_delay_breakdown(edge, delay_type);
        } else {
            //Primtiive edge
            //
            tatum::DelayComponent component;

            tatum::NodeId node = timing_graph_.edge_sink_node(edge);

            AtomPinId atom_pin = netlist_lookup_.tnode_atom_pin(node);
            AtomBlockId atom_blk = netlist_.pin_block(atom_pin);

            //component.inst_name = netlist_.block_name(atom_blk);

            component.type_name = "primitive '";
            component.type_name += netlist_.block_model(atom_blk)->name;
            component.type_name += "'";

            if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
                component.type_name += " combinational delay";

                if (delay_type == DelayType::MAX) {
                    component.delay = delay_calc_.max_edge_delay(timing_graph_, edge);
                } else {
                    VTR_ASSERT(delay_type == DelayType::MIN);
                    component.delay = delay_calc_.min_edge_delay(timing_graph_, edge);
                }
            } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                if (delay_type == DelayType::MAX) {
                    component.type_name += " Tcq_max";
                    component.delay = delay_calc_.max_edge_delay(timing_graph_, edge);
                } else {
                    VTR_ASSERT(delay_type == DelayType::MIN);
                    component.type_name += " Tcq_min";
                    component.delay = delay_calc_.min_edge_delay(timing_graph_, edge);
                }

            } else {
                VTR_ASSERT(edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE);

                if (delay_type == DelayType::MAX) {
                    component.type_name += " Tsu";
                    component.delay = delay_calc_.setup_time(timing_graph_, edge);
                } else {
                    component.type_name += " Thld";
                    component.delay = delay_calc_.hold_time(timing_graph_, edge);
                }
            }

            delay_breakdown.components.push_back(component);
        }
    }

    return delay_breakdown;
}

std::vector<tatum::DelayComponent> VprTimingGraphResolver::interconnect_delay_breakdown(tatum::EdgeId edge, DelayType delay_type) const {
    VTR_ASSERT(timing_graph_.edge_type(edge) == tatum::EdgeType::INTERCONNECT);
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& route_ctx = g_vpr_ctx.routing();

    std::vector<tatum::DelayComponent> components;

    // We assume that the delay calculator has already cached all of the relevant delays,
    // we just retrieve the cached values. This assumption greatly simplifies the calculation
    // process and avoids us duplicating the complex delay calculation logic from the delay
    // calculator.
    //
    // However note that this does couple this code tightly with the delay calculator implementation.

    //Force delay calculation to ensure results are cached (redundant if already up-to-date)
    delay_calc_.atom_net_delay(timing_graph_, edge, delay_type);

    ParentPinId src_pin = ParentPinId::INVALID();
    ParentPinId sink_pin = ParentPinId::INVALID();

    std::tie(src_pin, sink_pin) = delay_calc_.get_cached_pins(edge, delay_type);

    if (!src_pin && !sink_pin && !is_flat_) {
        //Cluster internal
        tatum::NodeId node = timing_graph_.edge_sink_node(edge);

        AtomPinId atom_pin = netlist_lookup_.tnode_atom_pin(node);
        AtomBlockId atom_blk = netlist_.pin_block(atom_pin);

        ClusterBlockId clb_blk = netlist_lookup_.atom_clb(atom_blk);

        tatum::DelayComponent internal_component;
        //internal_component.inst_name = cluster_ctx.clb_nlist.block_name(clb_blk);
        internal_component.type_name = "intra '";
        internal_component.type_name += cluster_ctx.clb_nlist.block_type(clb_blk)->name;
        internal_component.type_name += "' routing";
        internal_component.delay = delay_calc_.get_cached_delay(edge, delay_type);
        components.push_back(internal_component);

    } else {
        //Cluster external
        VTR_ASSERT(src_pin);
        VTR_ASSERT(sink_pin);

        ParentBlockId src_blk = ParentBlockId::INVALID();
        ParentBlockId sink_blk = ParentBlockId::INVALID();

        int sink_net_pin_index;

        ParentNetId src_net = ParentNetId::INVALID();
        tatum::Time driver_clb_delay;
        tatum::Time sink_clb_delay;

        if (is_flat_) {
            AtomNetId tmp_atom_net = atom_ctx.netlist().pin_net((AtomPinId&)src_pin);
            VTR_ASSERT(tmp_atom_net == atom_ctx.netlist().pin_net((AtomPinId&)sink_pin));

            AtomBlockId tmp_atom_src_block = atom_ctx.netlist().pin_block((AtomPinId&)src_pin);
            AtomBlockId tmp_atom_sink_block = atom_ctx.netlist().pin_block((AtomPinId&)sink_pin);

            src_blk = (ParentBlockId&)tmp_atom_src_block;
            sink_blk = (ParentBlockId&)tmp_atom_sink_block;

            src_net = (ParentNetId&)tmp_atom_net;

            driver_clb_delay = tatum::Time(0);
            sink_clb_delay = tatum::Time(0);

            sink_net_pin_index = g_vpr_ctx.atom().netlist().pin_net_index((AtomPinId&)sink_pin);

        } else {
            ClusterNetId tmp_cluster_net = cluster_ctx.clb_nlist.pin_net((ClusterPinId&)src_pin);
            VTR_ASSERT(tmp_cluster_net == cluster_ctx.clb_nlist.pin_net((ClusterPinId&)sink_pin));

            ClusterBlockId tmp_cluster_src_block = cluster_ctx.clb_nlist.pin_block((ClusterPinId&)src_pin);
            ClusterBlockId tmp_cluster_sink_block = cluster_ctx.clb_nlist.pin_block((ClusterPinId&)sink_pin);

            src_blk = (ParentBlockId&)tmp_cluster_src_block;
            sink_blk = (ParentBlockId&)tmp_cluster_sink_block;

            src_net = (ParentNetId&)tmp_cluster_net;

            driver_clb_delay = delay_calc_.get_driver_clb_cached_delay(edge, delay_type);
            sink_clb_delay = delay_calc_.get_sink_clb_cached_delay(edge, delay_type);

            sink_net_pin_index = cluster_ctx.clb_nlist.pin_net_index((ClusterPinId&)sink_pin);
        }

        tatum::Time net_delay = tatum::Time(delay_calc_.inter_cluster_delay(src_net, 0, sink_net_pin_index));

        VTR_ASSERT(driver_clb_delay.valid());
        VTR_ASSERT(net_delay.valid());
        VTR_ASSERT(sink_clb_delay.valid());

        tatum::DelayComponent driver_component;
        //driver_component.inst_name = cluster_ctx.clb_nlist.block_name(src_blk);
        driver_component.type_name = "intra '";
        if (is_flat_) {
            const t_pb* atom_pb = atom_ctx.lookup().atom_pb((AtomBlockId&)src_blk);
            driver_component.type_name += (std::string(atom_pb->name) + "(" + atom_pb->hierarchical_type_name() + ")");
        } else {
            driver_component.type_name += cluster_ctx.clb_nlist.block_type((ClusterBlockId&)src_blk)->name;
        }
        driver_component.type_name += "' routing";
        driver_component.delay = driver_clb_delay;
        components.push_back(driver_component);

        // For detailed timing report, we would like to breakdown the inter-block routing into
        // its constituent net components. As a pre-requisite, we would like to ensure these conditions:
        // 1. detailed timing report is selected
        // 2. the vector of tracebacks are not empty (important because this module is also called during
        // placement, which occurs before routing and implies and empty vector of tracebacks).
        // 3. the traceback is not a nullptr, for global routing, there are null tracebacks representing
        // unrouted nets.

        tatum::DelayComponent interblock_component;
        interblock_component.type_name = "inter-block routing";
        interblock_component.delay = net_delay;

        if ((detail_level() == e_timing_report_detail::DETAILED_ROUTING || detail_level() == e_timing_report_detail::DEBUG)
            && !route_ctx.route_trees.empty()) {
            // check if detailed timing report has been selected and that the routing
            // is not empty.
            if (route_ctx.route_trees[src_net]) {
                // the route tree exists, so we find the path of the net from source to sink.
                // Note that the previously declared interblock_component will not be added to the
                // vector of net components.
                get_detailed_interconnect_components(components, src_net, sink_pin);
            } else {
                // this is an unrouted net as part of global routing.
                // We add the tag global net, and push the interblock into the vector of net components.
                interblock_component.type_name += ":global net";
                components.push_back(interblock_component);
            }
        } else {
            // for aggregated and netlist modes, we simply add the previously declared interblock_component
            // to the vector.
            components.push_back(interblock_component);
        }
        tatum::DelayComponent sink_component;
        //sink_component.inst_name = cluster_ctx.clb_nlist.block_name(sink_blk);
        sink_component.type_name = "intra '";
        if (is_flat_) {
            sink_component.type_name += atom_ctx.lookup().atom_pb((AtomBlockId&)sink_blk)->name;
        } else {
            sink_component.type_name += cluster_ctx.clb_nlist.block_type((ClusterBlockId&)sink_blk)->name;
        }
        sink_component.type_name += "' routing";
        sink_component.delay = sink_clb_delay;
        components.push_back(sink_component);
    }
    return components;
}

e_timing_report_detail VprTimingGraphResolver::detail_level() const {
    return detail_level_;
}

void VprTimingGraphResolver::set_detail_level(e_timing_report_detail report_detail) {
    detail_level_ = report_detail;
}

void VprTimingGraphResolver::get_detailed_interconnect_components(std::vector<tatum::DelayComponent>& components, ParentNetId net_id, ParentPinId sink_pin) const {
    /* This routine obtains the interconnect components such as: OPIN, CHANX, CHANY, IPIN which join
     * two intra-block clusters in two parts. In part one, we construct the route tree
     * from the traceback and computes its value for R, C, and Tdel. Next, we find the pointer to
     * the route tree sink which corresponds to the sink_pin. In part two, we call the helper function,
     * which walks the route tree from the sink to the source. Along the way, we process each node
     * and construct net_components that are added to the vector of components. */

    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (!route_ctx.route_trees[net_id])
        return;

    auto& netlist = is_flat_ ? (const Netlist<>&)g_vpr_ctx.atom().netlist() : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;

    int ipin = netlist.pin_net_index(sink_pin);
    RRNodeId sink_rr_inode = RRNodeId(route_ctx.net_rr_terminals[net_id][ipin]); //obtain the value of the routing resource sink

    auto rt_sink = route_ctx.route_trees[net_id].value().find_by_rr_id(sink_rr_inode);

    get_detailed_interconnect_components_helper(components, rt_sink.value()); //from sink, walk up to source and add net components
}

void VprTimingGraphResolver::get_detailed_interconnect_components_helper(std::vector<tatum::DelayComponent>& components, const RouteTreeNode& node) const {
    /* This routine comprises the second part of obtaining the interconnect components.
     * We begin at the sink node and travel up the tree towards the source. For each node, we would
     * like to retrieve information such as the routing resource type, index, and incremental delay.
     * If the type is a channel, then we retrieve the name of the segment as well as the coordinates
     * of its start and end point. All of this information is stored in the object DelayComponent,
     * which belong to the vector "components".
     */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    // Declare a separate vector "interconnect_components" to hold the interconnect components. We need
    // this because we walk from the sink to the source, we will need to add elements to the front of
    // interconnect_components so that we maintain the correct order of net components.
    // Illustration:
    //    INTRA
    //      |
    //     IPIN <-end
    //      |
    //    CHANX
    //      |
    //     OPIN <-start
    //      |
    //    INTRA - not seen yet
    // Pushing a stack steps:
    // 1. OPIN, 2. CHANX, OPIN 3. IPIN, CHANX, OPIN (order is preserved)
    // At this point of the code, the vector "components" contains intrablock components.
    // At the end of the module, we will append "interconnect_components" to the end of vector "components".

    std::vector<tatum::DelayComponent> interconnect_components;

    vtr::optional<const RouteTreeNode&> current_node = node;
    while (current_node) {
        // Process the current interconnect component if it is of type OPIN, CHANX, CHANY, IPIN
        // Only process SOURCE, SINK in debug report mode
        auto rr_type = rr_graph.node_type(RRNodeId(current_node->inode));
        if (rr_type == OPIN
            || rr_type == IPIN
            || rr_type == CHANX
            || rr_type == CHANY
            || ((rr_type == SOURCE || rr_type == SINK) && (detail_level() == e_timing_report_detail::DEBUG))) {
            tatum::DelayComponent net_component; // declare a new instance of DelayComponent

            net_component.type_name = rr_graph.node_coordinate_to_string(RRNodeId(current_node->inode));

            if (current_node->parent()) {
                net_component.delay = tatum::Time(current_node->Tdel - current_node->parent()->Tdel); // add the incremental delay
            } else {
                net_component.delay = tatum::Time(0.); // No delay on SOURCE
            }
            interconnect_components.push_back(net_component); // insert net_component into the front of vector interconnect_component
        }
        current_node = current_node->parent();
    }

    components.insert(components.end(), interconnect_components.rbegin(), interconnect_components.rend()); // append the completed "interconnect_component" to "component_vector"
}
