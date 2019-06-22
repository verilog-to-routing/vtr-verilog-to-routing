#include "VprTimingGraphResolver.h"
#include "atom_netlist.h"
#include "atom_lookup.h"

VprTimingGraphResolver::VprTimingGraphResolver(const AtomNetlist& netlist, const AtomLookup& netlist_lookup, const tatum::TimingGraph& timing_graph, const AnalysisDelayCalculator& delay_calc)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , timing_graph_(timing_graph)
    , delay_calc_(delay_calc) {}

std::string VprTimingGraphResolver::node_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);

    return netlist_.pin_name(pin);
}

std::string VprTimingGraphResolver::node_type_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    AtomBlockId blk = netlist_.pin_block(pin);

    std::string name = netlist_.block_model(blk)->name;

    if (detail_level() == e_timing_report_detail::AGGREGATED) {
        //Annotate primitive grid location, if known
        auto& atom_ctx = g_vpr_ctx.atom();
        auto& place_ctx = g_vpr_ctx.placement();
        ClusterBlockId cb = atom_ctx.lookup.atom_clb(blk);
        if (cb && place_ctx.block_locs.count(cb)) {
            int x = place_ctx.block_locs[cb].loc.x;
            int y = place_ctx.block_locs[cb].loc.y;
            name += " at (" + std::to_string(x) + "," + std::to_string(y) + ")";
        }
    }

    return name;
}

tatum::EdgeDelayBreakdown VprTimingGraphResolver::edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType tatum_delay_type) const {
    tatum::EdgeDelayBreakdown delay_breakdown;

    if (edge && detail_level() == e_timing_report_detail::AGGREGATED) {
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

    std::vector<tatum::DelayComponent> components;

    //We assume that the delay calculator has already cached all of the relevant delays,
    //we just retrieve the cached values. This assumption greatly simplifies the calculation
    //process and avoids us duplicating the complex delay calculation logic from the delay
    //calcualtor.
    //
    //However note that this does couple this code tightly with the delay calculator implementation.

    //Force delay calculation to ensure results are cached (redundant if already up-to-date)
    delay_calc_.atom_net_delay(timing_graph_, edge, delay_type);

    ClusterPinId src_pin = ClusterPinId::INVALID();
    ClusterPinId sink_pin = ClusterPinId::INVALID();

    std::tie(src_pin, sink_pin) = delay_calc_.get_cached_pins(edge, delay_type);

    if (!src_pin && !sink_pin) {
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

        ClusterBlockId src_blk = cluster_ctx.clb_nlist.pin_block(src_pin);
        ClusterBlockId sink_blk = cluster_ctx.clb_nlist.pin_block(sink_pin);

        tatum::Time driver_clb_delay = delay_calc_.get_driver_clb_cached_delay(edge, delay_type);
        tatum::Time sink_clb_delay = delay_calc_.get_sink_clb_cached_delay(edge, delay_type);

        ClusterNetId src_net = cluster_ctx.clb_nlist.pin_net(src_pin);
        VTR_ASSERT(src_net == cluster_ctx.clb_nlist.pin_net(sink_pin));
        tatum::Time net_delay = tatum::Time(delay_calc_.inter_cluster_delay(src_net,
                                                                            0,
                                                                            cluster_ctx.clb_nlist.pin_net_index(sink_pin)));

        VTR_ASSERT(driver_clb_delay.valid());
        VTR_ASSERT(net_delay.valid());
        VTR_ASSERT(sink_clb_delay.valid());

        tatum::DelayComponent driver_component;
        //driver_component.inst_name = cluster_ctx.clb_nlist.block_name(src_blk);
        driver_component.type_name = "intra '";
        driver_component.type_name += cluster_ctx.clb_nlist.block_type(src_blk)->name;
        driver_component.type_name += "' routing";
        driver_component.delay = driver_clb_delay;
        components.push_back(driver_component);

        tatum::DelayComponent net_component;
        //net_component.inst_name = cluster_ctx.clb_nlist.net_name(src_net);
        net_component.type_name = "inter-block routing";
        net_component.delay = net_delay;
        components.push_back(net_component);

        tatum::DelayComponent sink_component;
        //sink_component.inst_name = cluster_ctx.clb_nlist.block_name(sink_blk);
        sink_component.type_name = "intra '";
        sink_component.type_name += cluster_ctx.clb_nlist.block_type(sink_blk)->name;
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
