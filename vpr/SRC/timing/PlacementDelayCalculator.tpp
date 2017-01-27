#include <cmath>
#include "PlacementDelayCalculator.hpp"

#include "vpr_error.h"
#include "vpr_utils.h"
#include "globals.h"

//Controls printing detailed info about edge delay calculation
//#define PLACEMENT_DELAY_CALC_DEBUG
inline PlacementDelayCalculator::PlacementDelayCalculator(const AtomNetlist& netlist, 
                                                          const AtomMap& netlist_map, 
                                                          float** net_delay)
    : netlist_(netlist)
    , netlist_map_(netlist_map)
    , net_delay_(net_delay)
    , atom_delay_calc_(netlist, netlist_map)
    , delay_cache_(g_timing_graph.nodes().size(), tatum::Time(NAN))
    {}

inline tatum::Time PlacementDelayCalculator::max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge) const { 
#ifdef PLACEMENT_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (max) ===\n", size_t(edge));
#endif
    tatum::EdgeType edge_type = tg.edge_type(edge);
    if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
        return atom_combinational_delay(tg, edge);

    } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
        return atom_clock_to_q_delay(tg, edge);

    } else if (edge_type == tatum::EdgeType::NET) {
        return atom_net_delay(tg, edge);
    }

    VPR_THROW(VPR_ERROR_TIMING, "Invalid edge type");

    return tatum::Time(NAN); //Suppress compiler warning
}

inline tatum::Time PlacementDelayCalculator::setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
#ifdef PLACEMENT_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (setup) ===\n", size_t(edge_id));
#endif
    return atom_setup_time(tg, edge_id);
}

inline tatum::Time PlacementDelayCalculator::min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
#ifdef PLACEMENT_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (min) ===\n", size_t(edge_id));
#endif
    return max_edge_delay(tg, edge_id); 
}

inline tatum::Time PlacementDelayCalculator::hold_time(const tatum::TimingGraph& /*tg*/, tatum::EdgeId /*edge_id*/) const { 
#ifdef PLACEMENT_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (hold) ===\n", size_t(edge_id));
#endif
    return tatum::Time(NAN); 
}

inline tatum::Time PlacementDelayCalculator::atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {

    tatum::Time delay = delay_cache_[edge_id];

    if(std::isnan(delay.value())) {
        //Miss
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        AtomPinId src_pin = netlist_map_.pin_tnode[src_node];
        AtomPinId sink_pin = netlist_map_.pin_tnode[sink_node];

        delay = tatum::Time(atom_delay_calc_.atom_combinational_delay(src_pin, sink_pin));

        //Insert
        delay_cache_[edge_id] = delay;
    }

    return delay;
}

inline tatum::Time PlacementDelayCalculator::atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::Time tsu = delay_cache_[edge_id];

    if(std::isnan(tsu.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId in_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(in_node) == tatum::NodeType::SINK);

        AtomPinId input_pin = netlist_map_.pin_tnode[in_node];
        AtomPinId clock_pin = netlist_map_.pin_tnode[clock_node];

        tsu = tatum::Time(atom_delay_calc_.atom_setup_time(clock_pin, input_pin));

        //Insert
        delay_cache_[edge_id] = tsu;
    }
    return tsu;
}

inline tatum::Time PlacementDelayCalculator::atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::Time tco = delay_cache_[edge_id];

    if(std::isnan(tco.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId out_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(out_node) == tatum::NodeType::SOURCE);

        AtomPinId output_pin = netlist_map_.pin_tnode[out_node];
        AtomPinId clock_pin = netlist_map_.pin_tnode[clock_node];

        tco = tatum::Time(atom_delay_calc_.atom_clock_to_q_delay(clock_pin, output_pin));

        //Insert
        delay_cache_[edge_id] = tco;
    }
    return tco;
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

    AtomPinId atom_src_pin = netlist_map_.pin_tnode[src_node];
    VTR_ASSERT(atom_src_pin);

    AtomPinId atom_sink_pin = netlist_map_.pin_tnode[sink_node];
    VTR_ASSERT(atom_sink_pin);

    AtomBlockId atom_src_block = netlist_.pin_block(atom_src_pin);
    AtomBlockId atom_sink_block = netlist_.pin_block(atom_sink_pin);

    AtomNetId atom_net = netlist_.pin_net(atom_sink_pin);

    int clb_src_block = netlist_map_.atom_clb(atom_src_block);
    VTR_ASSERT(clb_src_block >= 0);
    int clb_sink_block = netlist_map_.atom_clb(atom_sink_block);
    VTR_ASSERT(clb_sink_block >= 0);

    const t_pb_graph_pin* src_gpin = netlist_map_.atom_pin_pb_graph_pin(atom_src_pin);
    VTR_ASSERT(src_gpin);
    const t_pb_graph_pin* sink_gpin = netlist_map_.atom_pin_pb_graph_pin(atom_sink_pin);
    VTR_ASSERT(sink_gpin);

    int src_pb_route_id = src_gpin->pin_count_in_cluster;
    int sink_pb_route_id = sink_gpin->pin_count_in_cluster;

    VTR_ASSERT(block[clb_src_block].pb_route[src_pb_route_id].atom_net_id == atom_net);
    VTR_ASSERT(block[clb_sink_block].pb_route[sink_pb_route_id].atom_net_id == atom_net);

    //NOTE: even if both the source and sink atoms are contained in the same top-level
    //      CLB, the connection between them may not have been absorbed, and may go 
    //      through the global routing network.

    //We trace backward from the sink to the source

    //Trace the delay from the atom sink pin back to either the atom source pin (in the same cluster), 
    //or a cluster input pin
    
    float delay = NAN;
    const t_net_pin* sink_clb_net_pin = find_pb_route_clb_input_net_pin(clb_sink_block, sink_pb_route_id);
    if(sink_clb_net_pin != nullptr) {
        //Connection leaves the CLB

        int net = sink_clb_net_pin->net;
        const t_net_pin* driver_clb_net_pin = &g_clbs_nlist.net[net].pins[0];
        VTR_ASSERT(driver_clb_net_pin != nullptr);
        VTR_ASSERT(driver_clb_net_pin->block == clb_src_block);

        float driver_clb_delay = clb_delay_calc_.internal_src_to_clb_output_delay(src_pb_route_id, driver_clb_net_pin);

        float net_delay = inter_cluster_delay(driver_clb_net_pin, sink_clb_net_pin);

        float sink_clb_delay = clb_delay_calc_.clb_input_to_internal_sink_delay(sink_clb_net_pin, sink_pb_route_id);

        delay = driver_clb_delay + net_delay + sink_clb_delay;
    } else {
        //Connection entirely within the CLB
        VTR_ASSERT(clb_src_block == clb_sink_block);
        delay = clb_delay_calc_.internal_src_to_internal_sink_delay(clb_src_block, src_pb_route_id, sink_pb_route_id);
    }

    VTR_ASSERT(!std::isnan(delay));

    return tatum::Time(delay);
    
}

inline float PlacementDelayCalculator::inter_cluster_delay(const t_net_pin* driver_clb_pin, const t_net_pin* sink_clb_pin) const {
    int net = driver_clb_pin->net;
    VTR_ASSERT(driver_clb_pin->net_pin == 0);

    VTR_ASSERT(sink_clb_pin->net == net);

    return net_delay_[net][sink_clb_pin->net_pin];
}

