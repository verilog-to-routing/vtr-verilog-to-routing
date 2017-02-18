#ifndef PRE_CLUSTER_DELAY_CALCULATOR_H
#define PRE_CLUSTER_DELAY_CALCULATOR_H
#include "vtr_assert.h"

#include "Time.hpp"

#include "vpr_error.h"
#include "vpr_utils.h"

#include "atom_netlist.h"
#include "atom_map.h"
#include "physical_types.h"

class PreClusterDelayCalculator {
public:
    PreClusterDelayCalculator(const AtomNetlist& netlist,
                              const AtomMap& netlist_map,
                              float intercluster_net_delay, 
                              std::unordered_map<AtomBlockId,t_pb_graph_node*> expected_lowest_cost_pb_gnode)
        : netlist_(netlist)
        , netlist_map_(netlist_map)
        , inter_cluster_net_delay_(intercluster_net_delay)
        , block_to_pb_gnode_(expected_lowest_cost_pb_gnode) {
        //nop
    }

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        if(tg.node_type(src_node) == tatum::NodeType::CPIN) {
            //Tcq
            VTR_ASSERT_MSG(tg.node_type(sink_node) == tatum::NodeType::SOURCE, "Tcq only defined from CPIN to SOURCE");

            AtomPinId sink_pin = netlist_map_.pin_tnode[sink_node];
            VTR_ASSERT(sink_pin);

            const t_pb_graph_pin* gpin = find_pb_graph_pin(sink_pin);
            VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

            //Clock-to-q delay marked on the SOURCE node (the sink node of this edge)
            return tatum::Time(gpin->tsu_tco);

        } else if (tg.node_type(src_node) == tatum::NodeType::IPIN && tg.node_type(sink_node) == tatum::NodeType::OPIN) {
            //Primitive internal combinational delay
            AtomPinId input_pin = netlist_map_.pin_tnode[src_node];
            VTR_ASSERT(input_pin);
            const t_pb_graph_pin* input_gpin = find_pb_graph_pin(input_pin);

            AtomPinId output_pin = netlist_map_.pin_tnode[sink_node];
            VTR_ASSERT(output_pin);
            const t_pb_graph_pin* output_gpin = find_pb_graph_pin(output_pin);

            for(int i = 0; i < input_gpin->num_pin_timing; ++i) {
                const t_pb_graph_pin* sink_gpin = input_gpin->pin_timing[i];

                if(sink_gpin == output_gpin) {
                    return tatum::Time(input_gpin->pin_timing_del_max[i]);
                }
            }
            VTR_ASSERT_MSG(false, "Found no primitive combinational delay for edge");

        } else {
            VTR_ASSERT(   tg.node_type(src_node) == tatum::NodeType::OPIN 
                       || tg.node_type(src_node) == tatum::NodeType::SOURCE);
            VTR_ASSERT(   tg.node_type(sink_node) == tatum::NodeType::IPIN 
                       || tg.node_type(sink_node) == tatum::NodeType::SINK 
                       || tg.node_type(sink_node) == tatum::NodeType::CPIN);
            //External net delay
            return tatum::Time(inter_cluster_net_delay_);
        }

        VTR_ASSERT_MSG(false, "Unreachable");
        return tatum::Time(NAN);
    }

    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        VTR_ASSERT_MSG(tg.node_type(src_node) == tatum::NodeType::CPIN, "Edge setup time only valid if source node is a CPIN");
        VTR_ASSERT_MSG(tg.node_type(sink_node) == tatum::NodeType::SINK, "Edge setup time only valid if sink node is a SINK");
        
        AtomPinId sink_pin = netlist_map_.pin_tnode[sink_node];
        VTR_ASSERT(sink_pin);

        const t_pb_graph_pin* gpin = find_pb_graph_pin(sink_pin);
        VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

        return tatum::Time(gpin->tsu_tco);
    }

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
        //Currently return the same delay
        return max_edge_delay(tg, edge_id);
    }

    tatum::Time hold_time(const tatum::TimingGraph& /*tg*/, tatum::EdgeId /*edge_id*/) const {
        //Unimplemented
        return tatum::Time(NAN);
    }

private:
    const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin) const {
        AtomBlockId blk = netlist_.pin_block(pin);

        auto iter = block_to_pb_gnode_.find(blk);
        VTR_ASSERT(iter != block_to_pb_gnode_.end());

        const t_pb_graph_node* pb_gnode = iter->second;

        AtomPortId port = netlist_.pin_port(pin);
        const t_model_ports* model_port = netlist_.port_model(port);
        int ipin = netlist_.pin_port_bit(pin);

        const t_pb_graph_pin* gpin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb_gnode);
        VTR_ASSERT(gpin);

        return gpin;
    }

    const t_pb_graph_pin* find_associated_clock_pin(const AtomPinId io_pin) const {
        const t_pb_graph_pin* io_gpin = find_pb_graph_pin(io_pin);

        const t_pb_graph_pin* clock_gpin = io_gpin->associated_clock_pin; 

        if(!clock_gpin) {
            AtomBlockId blk = netlist_.pin_block(io_pin);
            const t_model* model = netlist_.block_model(blk);
            VPR_THROW(VPR_ERROR_TIMING, "Failed to find clock pin associated with pin '%s' (model '%s')", netlist_.pin_name(io_pin).c_str(), model->name);
        }
        return clock_gpin;
    }
private:
    const AtomNetlist& netlist_;
    const AtomMap& netlist_map_;
    const float inter_cluster_net_delay_;
    const std::unordered_map<AtomBlockId,t_pb_graph_node*> block_to_pb_gnode_;
};

#endif
