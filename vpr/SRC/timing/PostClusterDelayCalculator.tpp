#include <cmath>
#include "PostClusterDelayCalculator.h"

#include "vpr_error.h"
#include "vpr_utils.h"
#include "globals.h"

//Print detailed debug info about edge delay calculation
/*#define POST_CLUSTER_DELAY_CALC_DEBUG*/

inline PostClusterDelayCalculator::PostClusterDelayCalculator(const AtomNetlist& netlist, 
                                                          const AtomLookup& netlist_lookup, 
                                                          float** net_delay)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , net_delay_(net_delay)
    , atom_delay_calc_(netlist, netlist_lookup)
    , edge_delay_cache_(g_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , driver_clb_delay_cache_(g_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , sink_clb_delay_cache_(g_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , net_pin_cache_(g_ctx.timing().graph->edges().size(), std::pair<const t_net_pin*,const t_net_pin*>(nullptr,nullptr))
    {}

inline tatum::Time PostClusterDelayCalculator::max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge) const { 
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (max) ===\n", size_t(edge));
#endif
    tatum::EdgeType edge_type = tg.edge_type(edge);
    if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
        return atom_combinational_delay(tg, edge);

    } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
        return atom_clock_to_q_delay(tg, edge);

    } else if (edge_type == tatum::EdgeType::INTERCONNECT) {
        return atom_net_delay(tg, edge);
    }

    VPR_THROW(VPR_ERROR_TIMING, "Invalid edge type");

    return tatum::Time(NAN); //Suppress compiler warning
}

inline tatum::Time PostClusterDelayCalculator::setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (setup) ===\n", size_t(edge_id));
#endif
    return atom_setup_time(tg, edge_id);
}

inline tatum::Time PostClusterDelayCalculator::min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const { 
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    vtr::printf("=== Edge %zu (min) ===\n", size_t(edge_id));
#endif
    return max_edge_delay(tg, edge_id); 
}

inline tatum::Time PostClusterDelayCalculator::hold_time(const tatum::TimingGraph& /*tg*/, tatum::EdgeId /*edge_id*/) const { 
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    /*vtr::printf("=== Edge %zu (hold) ===\n", size_t(edge_id));*/
#endif
    return tatum::Time(NAN); 
}

inline tatum::Time PostClusterDelayCalculator::atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {

    tatum::Time delay = edge_delay_cache_[edge_id];

    if(std::isnan(delay.value())) {
        //Miss
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

        delay = tatum::Time(atom_delay_calc_.atom_combinational_delay(src_pin, sink_pin));

        //Insert
        edge_delay_cache_[edge_id] = delay;
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    vtr::printf("  Edge %zu Atom Comb Delay: %g\n", size_t(edge_id), delay.value());
#endif

    return delay;
}

inline tatum::Time PostClusterDelayCalculator::atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::Time tsu = edge_delay_cache_[edge_id];

    if(std::isnan(tsu.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId in_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(in_node) == tatum::NodeType::SINK);

        AtomPinId input_pin = netlist_lookup_.tnode_atom_pin(in_node);
        AtomPinId clock_pin = netlist_lookup_.tnode_atom_pin(clock_node);

        tsu = tatum::Time(atom_delay_calc_.atom_setup_time(clock_pin, input_pin));

        //Insert
        edge_delay_cache_[edge_id] = tsu;
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    vtr::printf("  Edge %zu Atom Tsu: %g\n", size_t(edge_id), tsu.value());
#endif
    return tsu;
}

inline tatum::Time PostClusterDelayCalculator::atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::Time tco = edge_delay_cache_[edge_id];

    if(std::isnan(tco.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId out_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(out_node) == tatum::NodeType::SOURCE);

        AtomPinId output_pin = netlist_lookup_.tnode_atom_pin(out_node);
        AtomPinId clock_pin = netlist_lookup_.tnode_atom_pin(clock_node);

        tco = tatum::Time(atom_delay_calc_.atom_clock_to_q_delay(clock_pin, output_pin));

        //Insert
        edge_delay_cache_[edge_id] = tco;
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    vtr::printf("  Edge %zu Atom Tco: %g\n", size_t(edge_id), tco.value());
#endif
    return tco;
}

inline tatum::Time PostClusterDelayCalculator::atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
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

    tatum::Time edge_delay = edge_delay_cache_[edge_id];
    if (std::isnan(edge_delay.value())) {
        //Miss

        //Note that the net pin pair will only be valid if we have already processed and cached
        //the clb delays on a previous call
        const t_net_pin* driver_clb_net_pin = nullptr;
        const t_net_pin* sink_clb_net_pin = nullptr;
        std::tie(driver_clb_net_pin, sink_clb_net_pin) = net_pin_cache_[edge_id];

        if(driver_clb_net_pin == nullptr && sink_clb_net_pin == nullptr) {
            //No cached intermediate results

            tatum::NodeId src_node = tg.edge_src_node(edge_id);
            tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

            AtomPinId atom_src_pin = netlist_lookup_.tnode_atom_pin(src_node);
            VTR_ASSERT(atom_src_pin);

            AtomPinId atom_sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);
            VTR_ASSERT(atom_sink_pin);

            AtomBlockId atom_src_block = netlist_.pin_block(atom_src_pin);
            AtomBlockId atom_sink_block = netlist_.pin_block(atom_sink_pin);

            int clb_src_block = netlist_lookup_.atom_clb(atom_src_block);
            VTR_ASSERT(clb_src_block >= 0);
            int clb_sink_block = netlist_lookup_.atom_clb(atom_sink_block);
            VTR_ASSERT(clb_sink_block >= 0);

            const t_pb_graph_pin* src_gpin = netlist_lookup_.atom_pin_pb_graph_pin(atom_src_pin);
            VTR_ASSERT(src_gpin);
            const t_pb_graph_pin* sink_gpin = netlist_lookup_.atom_pin_pb_graph_pin(atom_sink_pin);
            VTR_ASSERT(sink_gpin);

            int src_pb_route_id = src_gpin->pin_count_in_cluster;
            int sink_pb_route_id = sink_gpin->pin_count_in_cluster;

            auto& cluster_ctx = g_ctx.clustering();

            AtomNetId atom_net = netlist_.pin_net(atom_sink_pin);
            VTR_ASSERT(cluster_ctx.blocks[clb_src_block].pb_route[src_pb_route_id].atom_net_id == atom_net);
            VTR_ASSERT(cluster_ctx.blocks[clb_sink_block].pb_route[sink_pb_route_id].atom_net_id == atom_net);

            //NOTE: even if both the source and sink atoms are contained in the same top-level
            //      CLB, the connection between them may not have been absorbed, and may go 
            //      through the global routing network.

            //We trace backward from the sink to the source

            //Trace the delay from the atom sink pin back to either the atom source pin (in the same cluster), 
            //or a cluster input pin

            
            VTR_ASSERT(sink_clb_net_pin == nullptr);

            sink_clb_net_pin = find_pb_route_clb_input_net_pin(clb_sink_block, sink_pb_route_id);
            if(sink_clb_net_pin != nullptr) {
                //Connection leaves the CLB

                VTR_ASSERT(driver_clb_net_pin == nullptr);

                int net = sink_clb_net_pin->net;
                driver_clb_net_pin = &cluster_ctx.clbs_nlist.net[net].pins[0];
                VTR_ASSERT(driver_clb_net_pin != nullptr);
                VTR_ASSERT(driver_clb_net_pin->block == clb_src_block);

                tatum::Time driver_clb_delay = tatum::Time(clb_delay_calc_.internal_src_to_clb_output_delay(src_pb_route_id, driver_clb_net_pin));

                tatum::Time net_delay = tatum::Time(inter_cluster_delay(driver_clb_net_pin, sink_clb_net_pin));

                tatum::Time sink_clb_delay = tatum::Time(clb_delay_calc_.clb_input_to_internal_sink_delay(sink_clb_net_pin, sink_pb_route_id));

                edge_delay = driver_clb_delay + net_delay + sink_clb_delay;

                //Save the clb delays (but not the net dealy since it may change during placement)
                //also save the net pins associated with this edge
                driver_clb_delay_cache_[edge_id] = driver_clb_delay;
                sink_clb_delay_cache_[edge_id] = sink_clb_delay;
                net_pin_cache_[edge_id] = std::make_pair(driver_clb_net_pin, sink_clb_net_pin);
            } else {
                //Connection entirely within the CLB
                VTR_ASSERT(clb_src_block == clb_sink_block);

                edge_delay = tatum::Time(clb_delay_calc_.internal_src_to_internal_sink_delay(clb_src_block, src_pb_route_id, sink_pb_route_id));

                //Save the delay, since it won't change during placement or routing
                // Note that we cache the full edge delay for edges completely contained within CLBs
                edge_delay_cache_[edge_id] = edge_delay;
            }
        } else {
            //Calculate the edge delay using cached clb delays and the latest net delay
            //
            //  Note: we do not need to handle the case of edges fully contained within CLBs,
            //        since such delays are cached using edge_delay_cache_ and is handled earlier
            //        before this point

            VTR_ASSERT(driver_clb_net_pin != nullptr);
            VTR_ASSERT(sink_clb_net_pin != nullptr);

            tatum::Time driver_clb_delay = driver_clb_delay_cache_[edge_id];
            VTR_ASSERT(!std::isnan(driver_clb_delay.value()));

            tatum::Time sink_clb_delay = sink_clb_delay_cache_[edge_id];
            VTR_ASSERT(!std::isnan(sink_clb_delay.value()));

            tatum::Time net_delay = tatum::Time(inter_cluster_delay(driver_clb_net_pin, sink_clb_net_pin));

            edge_delay = driver_clb_delay + net_delay + sink_clb_delay;
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
            vtr::printf("  Edge %zu net delay: %g = %g + %g + %g (= clb_driver + net + clb_sink)\n", 
                        size_t(edge_id),
                        edge_delay.value(),
                        driver_clb_delay.value(),
                        net_delay.value(),
                        sink_clb_delay.value());
#endif
        }
    } else {
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
        vtr::printf("  Edge %zu CLB Internal delay: %g\n", size_t(edge_id), edge_delay.value());
#endif
    }

    VTR_ASSERT(!std::isnan(edge_delay.value()));

    return edge_delay;
    
}

inline float PostClusterDelayCalculator::inter_cluster_delay(const t_net_pin* driver_clb_pin, const t_net_pin* sink_clb_pin) const {
    int net = driver_clb_pin->net;
    VTR_ASSERT(driver_clb_pin->net_pin == 0);

    VTR_ASSERT(sink_clb_pin->net == net);

    return net_delay_[net][sink_clb_pin->net_pin];
}

