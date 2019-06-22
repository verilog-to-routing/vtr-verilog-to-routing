#include "PreClusterTimingGraphResolver.h"
#include "atom_netlist.h"
#include "atom_lookup.h"

PreClusterTimingGraphResolver::PreClusterTimingGraphResolver(
    const AtomNetlist& netlist,
    const AtomLookup& netlist_lookup,
    const tatum::TimingGraph& timing_graph,
    const tatum::DelayCalculator& delay_calc)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , timing_graph_(timing_graph)
    , delay_calc_(delay_calc) {}

std::string PreClusterTimingGraphResolver::node_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);

    return netlist_.pin_name(pin);
}

std::string PreClusterTimingGraphResolver::node_type_name(tatum::NodeId node) const {
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

tatum::EdgeDelayBreakdown PreClusterTimingGraphResolver::edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType tatum_delay_type) const {
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
            tatum::DelayComponent inter_cluster;
            inter_cluster.type_name = "inter-cluster net delay estimate";
            inter_cluster.delay = delay_calc_.max_edge_delay(timing_graph_, edge);
            delay_breakdown.components.push_back(inter_cluster);
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

e_timing_report_detail PreClusterTimingGraphResolver::detail_level() const {
    return detail_level_;
}

void PreClusterTimingGraphResolver::set_detail_level(e_timing_report_detail report_detail) {
    detail_level_ = report_detail;
}
