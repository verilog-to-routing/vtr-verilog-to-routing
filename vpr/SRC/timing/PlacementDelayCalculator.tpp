#include <cmath>
#include "PlacementDelayCalculator.hpp"

#include "vpr_error.h"
#include "vpr_utils.h"
#include "globals.h"

inline tatum::Time PlacementDelayCalculator::max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge) const { 
    vtr::printf("=== Edge %zu (max) ===\n", size_t(edge));
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
    vtr::printf("=== Edge %zu (setup) ===\n", size_t(edge_id));
    return atom_setup_time(tg, edge_id);
}

inline tatum::Time PlacementDelayCalculator::min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
    vtr::printf("=== Edge %zu (min calling max) ===\n", size_t(edge_id));
    return max_edge_delay(tg, edge_id); 
}

inline tatum::Time PlacementDelayCalculator::hold_time(const tatum::TimingGraph& /*tg*/, tatum::EdgeId edge_id) const { 
    vtr::printf("=== Edge %zu (hold) ===\n", size_t(edge_id));
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

    tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

    AtomPinId atom_sink_pin = netlist_map_.pin_tnode[sink_node];
    VTR_ASSERT(atom_sink_pin);

    AtomBlockId atom_sink_block = netlist_.pin_block(atom_sink_pin);

    AtomNetId atom_net = netlist_.pin_net(atom_sink_pin);

    int clb_sink_block = netlist_map_.atom_clb(atom_sink_block);
    VTR_ASSERT(clb_sink_block >= 0);

    const t_pb_graph_pin* sink_gpin = find_pb_graph_pin(atom_sink_pin);
    VTR_ASSERT(sink_gpin);

    VTR_ASSERT(block[clb_sink_block].pb_route[sink_gpin->pin_count_in_cluster].atom_net_id == atom_net);

    //NOTE: even if both the source and sink atoms are contained in the same top-level
    //      CLB, the connection between them may not have been absorbed, and may go 
    //      through the global routing network.

    //We trace backward from the sink to the source

    //Trace the delay from the atom sink pin back to either the atom source pin (in the same cluster), 
    //or a cluster input pin
    float capture_cluster_delay = NAN;
    t_net_pin clb_sink_input_pin;
    std::tie(capture_cluster_delay, clb_sink_input_pin) = trace_capture_cluster_delay(clb_sink_block, sink_gpin, atom_net);

    if(clb_sink_input_pin.block != UNDEFINED && clb_sink_input_pin.block_pin != UNDEFINED) {
        //Connection leaves the cluster

        float inter_cluster_delay = NAN;
        t_net_pin clb_driver_output_pin;
        std::tie(inter_cluster_delay, clb_driver_output_pin) = trace_inter_cluster_delay(clb_sink_input_pin);

        float launch_cluster_delay = NAN;
        std::tie(launch_cluster_delay) = trace_launch_cluster_delay(clb_driver_output_pin, atom_net);

        return tatum::Time(launch_cluster_delay + inter_cluster_delay + capture_cluster_delay);
    } else {
        //Connection internal to cluster
        
        return tatum::Time(capture_cluster_delay);
    }
}

inline std::tuple<float,t_net_pin> PlacementDelayCalculator::trace_capture_cluster_delay(int clb_sink_block, const t_pb_graph_pin* sink_gpin, const AtomNetId atom_net) const {
    t_pb_route* sink_pb_routes = block[clb_sink_block].pb_route;

    float delay = 0.;

    int sink_pb_route_idx = sink_gpin->pin_count_in_cluster;
    const t_pb_route* sink_pb_route = &sink_pb_routes[sink_pb_route_idx];
    float incr_delay = pb_route_max_delay(clb_sink_block, sink_pb_route_idx);
    delay += incr_delay;
    vtr::printf("CLB: %d PB Route %d: atom_net=%zu (%s) prev_pb_pin_id=%d delay=%g incr_delay=%g\n", 
                clb_sink_block,
                sink_pb_route_idx, sink_pb_route->atom_net_id, 
                netlist_.net_name(sink_pb_route->atom_net_id).c_str(), 
                sink_pb_route->prev_pb_pin_id,
                delay,
                incr_delay);

    while(sink_pb_route->prev_pb_pin_id >= 0) {
        //Advance to the next element
        sink_pb_route_idx = sink_pb_route->prev_pb_pin_id;
        sink_pb_route = &sink_pb_routes[sink_pb_route_idx];
        incr_delay = pb_route_max_delay(clb_sink_block, sink_pb_route_idx);
        delay += incr_delay;

        vtr::printf("CLB: %d PB Route %d: atom_net=%zu (%s) prev_pb_pin_id=%d delay=%g incr_delay=%g\n", 
                    clb_sink_block,
                    sink_pb_route_idx, sink_pb_route->atom_net_id, 
                    netlist_.net_name(sink_pb_route->atom_net_id).c_str(), 
                    sink_pb_route->prev_pb_pin_id,
                    delay,
                    incr_delay);

        VTR_ASSERT(sink_pb_route->atom_net_id == atom_net);
    }


    t_net_pin clb_input_pin;
    if(is_clb_io_pin(clb_sink_block, sink_pb_route_idx)) {
        //Leaves the current cluster, so find the associated CLB net pin
        int iclb_net = block[clb_sink_block].nets[sink_pb_route_idx];

        for(size_t ipin = 0; ipin < g_clbs_nlist.net.size(); ++ipin) {
            if(g_clbs_nlist.net[iclb_net].pins[ipin].block == clb_sink_block
               && g_clbs_nlist.net[iclb_net].pins[ipin].block_pin == sink_pb_route_idx) {
                clb_input_pin = g_clbs_nlist.net[iclb_net].pins[ipin];
                break;
            }
        }
        VTR_ASSERT(clb_input_pin.block == clb_sink_block);
        VTR_ASSERT(clb_input_pin.block_pin == sink_pb_route_idx);
    }

    
    //TODO: delay
    return std::make_tuple(delay, clb_input_pin);
}

inline std::tuple<float,t_net_pin> PlacementDelayCalculator::trace_inter_cluster_delay(t_net_pin clb_sink_input_pin) const {
    int iclb_net = clb_sink_input_pin.net;
    int iclb_net_sink_pin = clb_sink_input_pin.net_pin;

    VTR_ASSERT(g_clbs_nlist.net[iclb_net].pins[iclb_net_sink_pin].block == clb_sink_input_pin.block);
    VTR_ASSERT(g_clbs_nlist.net[iclb_net].pins[iclb_net_sink_pin].block_pin == clb_sink_input_pin.block_pin);

    float delay = net_delay_[iclb_net][iclb_net_sink_pin];

    vtr::printf("CLB Net: %d (%s) delay=%g\n", iclb_net, g_clbs_nlist.net[iclb_net].name, delay);

    t_net_pin clb_driver_output_pin = g_clbs_nlist.net[clb_sink_input_pin.net].pins[0];

    //TODO: delay
    return std::make_tuple(delay, clb_driver_output_pin);
}

inline std::tuple<float> PlacementDelayCalculator::trace_launch_cluster_delay(t_net_pin clb_driver_output_pin, const AtomNetId atom_net) const {
    t_pb_route* driver_pb_routes = block[clb_driver_output_pin.block].pb_route;

    float delay = 0.;

    int driver_pb_route_idx = clb_driver_output_pin.block_pin;
    const t_pb_route* driver_pb_route = &driver_pb_routes[driver_pb_route_idx];
    float incr_delay = pb_route_max_delay(clb_driver_output_pin.block, driver_pb_route_idx);
    delay += incr_delay;
    vtr::printf("CLB: %d PB Route %d: atom_net=%zu (%s) prev_pb_pin_id=%d delay=%g incr_delay=%g\n", 
                clb_driver_output_pin.block,
                driver_pb_route_idx, driver_pb_route->atom_net_id, 
                netlist_.net_name(driver_pb_route->atom_net_id).c_str(), 
                driver_pb_route->prev_pb_pin_id,
                delay,
                incr_delay);

    while(driver_pb_route->prev_pb_pin_id >= 0) {

        //Advance to the next element
        driver_pb_route_idx = driver_pb_route->prev_pb_pin_id;
        driver_pb_route = &driver_pb_routes[driver_pb_route_idx];

        incr_delay = pb_route_max_delay(clb_driver_output_pin.block, driver_pb_route_idx);
        delay += incr_delay;

    vtr::printf("CLB: %d PB Route %d: atom_net=%zu (%s) prev_pb_pin_id=%d delay=%g incr_delay=%g\n", 
                clb_driver_output_pin.block,
                driver_pb_route_idx, driver_pb_route->atom_net_id, 
                netlist_.net_name(driver_pb_route->atom_net_id).c_str(), 
                driver_pb_route->prev_pb_pin_id,
                delay,
                incr_delay);

        VTR_ASSERT(driver_pb_route->atom_net_id == atom_net);
    }

    //TODO: delay
    return std::make_tuple(delay);
}



inline const t_pb_graph_pin* PlacementDelayCalculator::find_pb_graph_pin(const AtomPinId atom_pin) const {
#if 0
    AtomBlockId blk = netlist_.pin_block(pin);

    //Graph node for this primitive
    const t_pb_graph_node* pb_gnode = netlist_map_.atom_pb_graph_node(blk);
    VTR_ASSERT(pb_gnode);

    AtomPortId port = netlist_.pin_port(pin);
    const t_model_ports* model_port = netlist_.port_model(port);
    int ipin = netlist_.pin_port_bit(pin);

    const t_pb_graph_pin* gpin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb_gnode);
    VTR_ASSERT(gpin);

    return gpin;
#else
    AtomNetId atom_net = netlist_.pin_net(atom_pin);
    AtomPortId atom_port = netlist_.pin_port(atom_pin);
    AtomBlockId atom_blk = netlist_.pin_block(atom_pin);

    const t_model_ports* model_port = netlist_.port_model(atom_port);

    int clb = netlist_map_.atom_clb(atom_blk);
    const t_pb_route* pb_route = block[clb].pb_route;
    const t_pb_graph_node* pb_gnode = netlist_map_.atom_pb_graph_node(atom_blk);

    //Find the pb graph pin associated with the this atom pin
    //
    //Note that we can not look-up the pb graph pin directly by index, since the pins on primitives 
    //such as LUTs may have been rotated (this happens somewhere inside the packer...)
    //TODO: find where this is done in the packer and update the atom netlist to reflect the applied
    //      rotation and update this code to perform the look-up directly
    const t_pb_graph_pin* net_gpin = nullptr;
    for(BitIndex ipin = 0; ipin < netlist_.port_width(atom_port); ++ipin) {
        const t_pb_graph_pin* gpin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb_gnode);
        if(gpin) {
            AtomNetId connected_atom_net = pb_route[gpin->pin_count_in_cluster].atom_net_id;

            if(connected_atom_net == atom_net) {
                net_gpin = gpin;
                break;
            }
        }
    }
    VTR_ASSERT(net_gpin != nullptr);

    return net_gpin;
#endif
}

inline bool PlacementDelayCalculator::is_clb_io_pin(int clb_block, int clb_pb_route_idx) const {
    return clb_pb_route_idx < block[clb_block].type->num_pins;
}

inline float PlacementDelayCalculator::pb_route_max_delay(int clb_block, int pb_route_idx) const {
    const t_pb_graph_edge* pb_edge = find_pb_graph_edge(clb_block, pb_route_idx);

    if(pb_edge) {
        return pb_edge->delay_max;
    } else {
        return 0.;
    }
}

const t_pb_graph_edge* PlacementDelayCalculator::find_pb_graph_edge(int clb_block, int pb_route_idx) const {
    int type_index = block[clb_block].type->index;

    int upstream_pb_route_idx = block[clb_block].pb_route[pb_route_idx].prev_pb_pin_id;

    if(upstream_pb_route_idx >= 0) {

        const t_pb_graph_pin* pb_gpin = intra_lb_pb_pin_lookup_.pb_gpin(type_index, pb_route_idx);
        const t_pb_graph_pin* upstream_pb_gpin = intra_lb_pb_pin_lookup_.pb_gpin(type_index, upstream_pb_route_idx);

        return find_pb_graph_edge(upstream_pb_gpin, pb_gpin); 
    } else {
        return nullptr;
    }
}

const t_pb_graph_edge* PlacementDelayCalculator::find_pb_graph_edge(const t_pb_graph_pin* driver, const t_pb_graph_pin* sink) const {
    VTR_ASSERT(driver);
    VTR_ASSERT(sink);

    const t_pb_graph_edge* pb_edge = nullptr;
    for(int iedge = 0; iedge < driver->num_output_edges; ++iedge) {
        const t_pb_graph_edge* check_edge = driver->output_edges[iedge];
        VTR_ASSERT(check_edge);

        VTR_ASSERT(check_edge->num_output_pins == 1);
        if(check_edge->output_pins[0] == sink) {
            pb_edge = check_edge;
        }

    }
    VTR_ASSERT_MSG(pb_edge, "Failed to find pb_graph_edge connecting PB pins");

    return pb_edge;
}

