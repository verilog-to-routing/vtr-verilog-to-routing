#include <cmath>
#include "PlacementDelayCalculator.hpp"

#include "vpr_error.h"
#include "vpr_utils.h"

inline tatum::Time PlacementDelayCalculator::max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge) const { 
    tatum::EdgeType edge_type = tg.edge_type(edge);
    if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
        return atom_combinational_delay(tg, edge);

    } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
        return atom_setup_time(tg, edge);

    } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
        return atom_clock_to_q_delay(tg, edge);

    } else if (edge_type == tatum::EdgeType::NET) {
        return atom_net_delay(tg, edge);
    }

    VPR_THROW(VPR_ERROR_TIMING, "Invalid edge type");

    return tatum::Time(NAN); //Suppress compiler warning
}

inline tatum::Time PlacementDelayCalculator::setup_time(const tatum::TimingGraph& /*tg*/, tatum::EdgeId /*edge_id*/) const { 
    return tatum::Time(NAN); 
}

inline tatum::Time PlacementDelayCalculator::min_edge_delay(const tatum::TimingGraph& /*tg*/, tatum::EdgeId /*edge_id*/) const { 
    return tatum::Time(NAN); 
}

inline tatum::Time PlacementDelayCalculator::hold_time(const tatum::TimingGraph& /*tg*/, tatum::EdgeId /*edge_id*/) const { 
    return tatum::Time(NAN); 
}

inline tatum::Time PlacementDelayCalculator::atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::NodeId src_node = tg.edge_src_node(edge_id);
    tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

    AtomPinId src_pin = netlist_map_.pin_tnode[src_node];
    AtomPinId sink_pin = netlist_map_.pin_tnode[sink_node];

    AtomBlockId blk = netlist_.pin_block(src_pin);
    VTR_ASSERT_MSG(netlist_.pin_block(sink_pin) == blk, "Combinational primitive delay must be between pins on the same block");

    VTR_ASSERT_MSG(   netlist_.port_type(netlist_.pin_port(src_pin)) == AtomPortType::INPUT 
                   && netlist_.port_type(netlist_.pin_port(sink_pin)) == AtomPortType::OUTPUT,
                   "Combinational connections must go from primitive input to output");

    //Determine the combinational delay from the pb_graph_pin.
    //  By convention the delay is annotated on the corresponding input pg_graph_pin
    const t_pb_graph_pin* src_gpin = find_pb_graph_pin(src_pin);
    const t_pb_graph_pin* sink_gpin = find_pb_graph_pin(sink_pin);
    VTR_ASSERT(src_gpin->num_pin_timing > 0);

    //Find the annotation on the source that matches the sink
    float delay = NAN;
    for(int i = 0; i < src_gpin->num_pin_timing; ++i) {
        const t_pb_graph_pin* timing_sink_gpin = src_gpin->pin_timing[i]; 

        if(timing_sink_gpin == sink_gpin) {
            delay = src_gpin->pin_timing_del_max[i];
            break;
        }
    }

    VTR_ASSERT_MSG(!std::isnan(delay), "Must have a valid delay");

    return tatum::Time(delay);
}

inline tatum::Time PlacementDelayCalculator::atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::NodeId clock_node = tg.edge_src_node(edge_id);
    VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

    tatum::NodeId in_node = tg.edge_sink_node(edge_id);
    VTR_ASSERT(tg.node_type(in_node) == tatum::NodeType::SINK);

    AtomPinId input_pin = netlist_map_.pin_tnode[in_node];
    VTR_ASSERT(netlist_.port_type(netlist_.pin_port(input_pin)) == AtomPortType::INPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(input_pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    return tatum::Time(gpin->tsu_tco);
}

inline tatum::Time PlacementDelayCalculator::atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::NodeId clock_node = tg.edge_src_node(edge_id);
    VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

    tatum::NodeId out_node = tg.edge_sink_node(edge_id);
    VTR_ASSERT(tg.node_type(out_node) == tatum::NodeType::SOURCE);

    AtomPinId output_pin = netlist_map_.pin_tnode[out_node];
    VTR_ASSERT(netlist_.port_type(netlist_.pin_port(output_pin)) == AtomPortType::OUTPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(output_pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    return tatum::Time(gpin->tsu_tco);
}

inline tatum::Time PlacementDelayCalculator::atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    //A net in the atom netlist consists of several different delay components:
    //
    //      delay = launch_cluster_delay + inter_cluster_delay + capture_cluster_delay
    //
    //where:
    //  * launch_cluster_delay is the delay from the primitive output pin to the cluster output pin,
    //  * inter_cluster_delay is the routing delay through the inter-cluster routing network, and
    //  * catpure_cluster_delay is the delay from the cluster input pin to the primitive input pin
    //
    //Note that if the connection is completely absorbed within the cluster then inter_cluster_delay
    //and capture_cluster_delay will both be zero.

    tatum::NodeId src_node = tg.edge_src_node(edge_id);
    tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

    AtomPinId src_pin = netlist_map_.pin_tnode[src_node];
    AtomPinId sink_pin = netlist_map_.pin_tnode[sink_node];

    const t_pb_graph_pin* src_gpin = find_pb_graph_pin(src_pin);
    const t_pb_graph_pin* sink_gpin = find_pb_graph_pin(sink_pin);

    //NOTE: even if both the source and sink atoms are contained in the same top-level
    //      CLB, the connection between them may not have been absorbed, and may go 
    //      through the global routing network.

    //Trace the delay from the atom source pin to either the atom sink pin, 
    //or a cluster output pin
    float launch_cluster_delay = 0.;
    //TODO

    //Trace the global routing delay from the source cluster output to the sink
    //cluster input pin
    float inter_cluster_delay = 0.;
    //TODO

    //Trace the delay from the sink cluster input pin to the atom input pin
    float capture_cluster_delay = 0.;
    //TODO
    
    return tatum::Time(launch_cluster_delay + inter_cluster_delay + capture_cluster_delay);
}


inline const t_pb_graph_pin* PlacementDelayCalculator::find_pb_graph_pin(const AtomPinId pin) const {
    AtomBlockId blk = netlist_.pin_block(pin);

    const t_pb_graph_node* pb_gnode = netlist_map_.atom_pb_graph_node(blk);
    VTR_ASSERT(pb_gnode);

    AtomPortId port = netlist_.pin_port(pin);
    const t_model_ports* model_port = netlist_.port_model(port);
    int ipin = netlist_.pin_port_bit(pin);

    const t_pb_graph_pin* gpin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb_gnode);
    VTR_ASSERT(gpin);

    return gpin;
}
