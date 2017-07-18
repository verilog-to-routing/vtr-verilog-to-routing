#ifndef PRE_CLUSTER_DELAY_CALCULATOR_H
#define PRE_CLUSTER_DELAY_CALCULATOR_H
#include "vtr_assert.h"

#include "tatum/Time.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

#include "vpr_error.h"
#include "vpr_utils.h"

#include "atom_netlist.h"
#include "atom_lookup.h"
#include "physical_types.h"
#include "DelayType.h"
#include "atom_delay_calc.h"

class PreClusterDelayCalculator : public tatum::DelayCalculator {
public:
    PreClusterDelayCalculator(const AtomNetlist& netlist,
                              const AtomLookup& netlist_lookup,
                              float intercluster_net_delay)
        : netlist_(netlist)
        , netlist_lookup_(netlist_lookup)
        , atom_delay_calc_(netlist_, netlist_lookup_)
        , inter_cluster_net_delay_(intercluster_net_delay) {
        //nop
    }

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override { 
        return calc_edge_delay(tg, edge_id, DelayType::MAX);
    }

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override { 
        return calc_edge_delay(tg, edge_id, DelayType::MIN);
    }

    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override { 
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);
        
        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

        return tatum::Time(atom_delay_calc_.atom_setup_time(src_pin, sink_pin));
    }

    tatum::Time hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override {
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);
        
        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

        return tatum::Time(atom_delay_calc_.atom_hold_time(src_pin, sink_pin));
    }

private:

    tatum::Time calc_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const {
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

        auto edge_type = tg.edge_type(edge_id);

        if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
            return tatum::Time(atom_delay_calc_.atom_combinational_delay(src_pin, sink_pin, delay_type));
        } else if(edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
            return tatum::Time(atom_delay_calc_.atom_clock_to_q_delay(src_pin, sink_pin, delay_type));
        } else {
            VTR_ASSERT(edge_type == tatum::EdgeType::INTERCONNECT);

            //External net delay
            return tatum::Time(inter_cluster_net_delay_);
        }
    }

private:
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;
    const AtomDelayCalc atom_delay_calc_;
    const float inter_cluster_net_delay_;
};

#endif
