#include <cmath>
#include "PostClusterDelayCalculator.h"

#include "vpr_error.h"
#include "vpr_utils.h"
#include "globals.h"

#include "vtr_assert.h"

//Print detailed debug info about edge delay calculation
/*#define POST_CLUSTER_DELAY_CALC_DEBUG*/

inline PostClusterDelayCalculator::PostClusterDelayCalculator(const AtomNetlist& netlist,
                                                              const AtomLookup& netlist_lookup,
                                                              vtr::vector<ClusterNetId, float*>& net_delay)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , net_delay_()
    , atom_delay_calc_(netlist, netlist_lookup)
    , edge_min_delay_cache_(g_vpr_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , edge_max_delay_cache_(g_vpr_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , driver_clb_min_delay_cache_(g_vpr_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , driver_clb_max_delay_cache_(g_vpr_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , sink_clb_min_delay_cache_(g_vpr_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , sink_clb_max_delay_cache_(g_vpr_ctx.timing().graph->edges().size(), tatum::Time(NAN))
    , pin_cache_min_(g_vpr_ctx.timing().graph->edges().size(), std::pair<ClusterPinId, ClusterPinId>(ClusterPinId::INVALID(), ClusterPinId::INVALID()))
    , pin_cache_max_(g_vpr_ctx.timing().graph->edges().size(), std::pair<ClusterPinId, ClusterPinId>(ClusterPinId::INVALID(), ClusterPinId::INVALID())) {
    net_delay_ = net_delay;
}

inline void PostClusterDelayCalculator::clear_cache() {
    std::fill(edge_min_delay_cache_.begin(), edge_min_delay_cache_.end(), tatum::Time(NAN));
    std::fill(edge_max_delay_cache_.begin(), edge_max_delay_cache_.end(), tatum::Time(NAN));
    std::fill(driver_clb_min_delay_cache_.begin(), driver_clb_min_delay_cache_.end(), tatum::Time(NAN));
    std::fill(driver_clb_max_delay_cache_.begin(), driver_clb_max_delay_cache_.end(), tatum::Time(NAN));
    std::fill(sink_clb_min_delay_cache_.begin(), sink_clb_min_delay_cache_.end(), tatum::Time(NAN));
    std::fill(sink_clb_max_delay_cache_.begin(), sink_clb_max_delay_cache_.end(), tatum::Time(NAN));
    std::fill(pin_cache_min_.begin(), pin_cache_min_.end(), std::pair<ClusterPinId, ClusterPinId>(ClusterPinId::INVALID(), ClusterPinId::INVALID()));
    std::fill(pin_cache_max_.begin(), pin_cache_max_.end(), std::pair<ClusterPinId, ClusterPinId>(ClusterPinId::INVALID(), ClusterPinId::INVALID()));
}

inline void PostClusterDelayCalculator::set_tsu_margin_relative(float new_margin) {
    tsu_margin_rel_ = new_margin;
}

inline void PostClusterDelayCalculator::set_tsu_margin_absolute(float new_margin) {
    tsu_margin_abs_ = new_margin;
}

inline tatum::Time PostClusterDelayCalculator::max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("=== Edge %zu (max) ===\n", size_t(edge_id));
#endif
    return calc_edge_delay(tg, edge_id, DelayType::MAX);
}

inline tatum::Time PostClusterDelayCalculator::min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("=== Edge %zu (min) ===\n", size_t(edge_id));
#endif
    return calc_edge_delay(tg, edge_id, DelayType::MIN);
}

inline tatum::Time PostClusterDelayCalculator::setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("=== Edge %zu (setup) ===\n", size_t(edge_id));
#endif
    return atom_setup_time(tg, edge_id);
}

inline tatum::Time PostClusterDelayCalculator::hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("=== Edge %zu (hold) ===\n", size_t(edge_id));
#endif
    return atom_hold_time(tg, edge_id);
}

inline tatum::Time PostClusterDelayCalculator::calc_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge, DelayType delay_type) const {
    tatum::EdgeType edge_type = tg.edge_type(edge);
    if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
        return atom_combinational_delay(tg, edge, delay_type);

    } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
        return atom_clock_to_q_delay(tg, edge, delay_type);

    } else if (edge_type == tatum::EdgeType::INTERCONNECT) {
        return atom_net_delay(tg, edge, delay_type);
    }

    VPR_THROW(VPR_ERROR_TIMING, "Invalid edge type");

    return tatum::Time(NAN); //Suppress compiler warning
}

inline tatum::Time PostClusterDelayCalculator::atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const {
    tatum::Time delay = get_cached_delay(edge_id, delay_type);

    if (std::isnan(delay.value())) {
        //Miss
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

        delay = tatum::Time(atom_delay_calc_.atom_combinational_delay(src_pin, sink_pin, delay_type));

        //Insert
        set_cached_delay(edge_id, delay_type, delay);
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("  Edge %zu Atom Comb Delay: %g\n", size_t(edge_id), delay.value());
#endif

    return delay;
}

inline tatum::Time PostClusterDelayCalculator::atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::Time tsu = get_cached_setup_time(edge_id);

    if (std::isnan(tsu.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId in_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(in_node) == tatum::NodeType::SINK);

        AtomPinId input_pin = netlist_lookup_.tnode_atom_pin(in_node);
        AtomPinId clock_pin = netlist_lookup_.tnode_atom_pin(clock_node);

        tsu = tatum::Time(tsu_margin_rel_ * atom_delay_calc_.atom_setup_time(clock_pin, input_pin) + tsu_margin_abs_);

        //Insert
        set_cached_setup_time(edge_id, tsu);
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("  Edge %zu Atom Tsu: %g\n", size_t(edge_id), tsu.value());
#endif
    return tsu;
}

inline tatum::Time PostClusterDelayCalculator::atom_hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const {
    tatum::Time thld = get_cached_hold_time(edge_id);

    if (std::isnan(thld.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId in_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(in_node) == tatum::NodeType::SINK);

        AtomPinId input_pin = netlist_lookup_.tnode_atom_pin(in_node);
        AtomPinId clock_pin = netlist_lookup_.tnode_atom_pin(clock_node);

        thld = tatum::Time(atom_delay_calc_.atom_hold_time(clock_pin, input_pin));

        //Insert
        set_cached_hold_time(edge_id, thld);
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("  Edge %zu Atom Thld: %g\n", size_t(edge_id), thld.value());
#endif
    return thld;
}

inline tatum::Time PostClusterDelayCalculator::atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const {
    tatum::Time tco = get_cached_delay(edge_id, delay_type);

    if (std::isnan(tco.value())) {
        //Miss
        tatum::NodeId clock_node = tg.edge_src_node(edge_id);
        VTR_ASSERT(tg.node_type(clock_node) == tatum::NodeType::CPIN);

        tatum::NodeId out_node = tg.edge_sink_node(edge_id);
        VTR_ASSERT(tg.node_type(out_node) == tatum::NodeType::SOURCE);

        AtomPinId output_pin = netlist_lookup_.tnode_atom_pin(out_node);
        AtomPinId clock_pin = netlist_lookup_.tnode_atom_pin(clock_node);

        tco = tatum::Time(atom_delay_calc_.atom_clock_to_q_delay(clock_pin, output_pin, delay_type));

        //Insert
        set_cached_delay(edge_id, delay_type, tco);
    }
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
    VTR_LOG("  Edge %zu Atom Tco: %g\n", size_t(edge_id), tco.value());
#endif
    return tco;
}

inline tatum::Time PostClusterDelayCalculator::atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const {
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
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterNetId net_id = ClusterNetId::INVALID();
    int src_block_pin_index, sink_net_pin_index, sink_block_pin_index;

    tatum::Time edge_delay = get_cached_delay(edge_id, delay_type);
    if (std::isnan(edge_delay.value())) {
        //Miss

        //Note that the net pin pair will only be valid if we have already processed and cached
        //the clb delays on a previous call
        ClusterPinId src_pin = ClusterPinId::INVALID();
        ClusterPinId sink_pin = ClusterPinId::INVALID();

        std::tie(src_pin, sink_pin) = get_cached_pins(edge_id, delay_type);

        if (src_pin == ClusterPinId::INVALID() && sink_pin == ClusterPinId::INVALID()) {
            //No cached intermediate results

            tatum::NodeId src_node = tg.edge_src_node(edge_id);
            tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

            AtomPinId atom_src_pin = netlist_lookup_.tnode_atom_pin(src_node);
            VTR_ASSERT(atom_src_pin);

            AtomPinId atom_sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);
            VTR_ASSERT(atom_sink_pin);

            AtomBlockId atom_src_block = netlist_.pin_block(atom_src_pin);
            AtomBlockId atom_sink_block = netlist_.pin_block(atom_sink_pin);

            ClusterBlockId clb_src_block = netlist_lookup_.atom_clb(atom_src_block);
            VTR_ASSERT(clb_src_block != ClusterBlockId::INVALID());
            ClusterBlockId clb_sink_block = netlist_lookup_.atom_clb(atom_sink_block);
            VTR_ASSERT(clb_sink_block != ClusterBlockId::INVALID());

            const t_pb_graph_pin* src_gpin = netlist_lookup_.atom_pin_pb_graph_pin(atom_src_pin);
            VTR_ASSERT(src_gpin);
            const t_pb_graph_pin* sink_gpin = netlist_lookup_.atom_pin_pb_graph_pin(atom_sink_pin);
            VTR_ASSERT(sink_gpin);

            int src_pb_route_id = src_gpin->pin_count_in_cluster;
            int sink_pb_route_id = sink_gpin->pin_count_in_cluster;

            AtomNetId atom_net = netlist_.pin_net(atom_sink_pin);
            VTR_ASSERT(cluster_ctx.clb_nlist.block_pb(clb_src_block)->pb_route[src_pb_route_id].atom_net_id == atom_net);
            VTR_ASSERT(cluster_ctx.clb_nlist.block_pb(clb_sink_block)->pb_route[sink_pb_route_id].atom_net_id == atom_net);

            //NOTE: even if both the source and sink atoms are contained in the same top-level
            //      CLB, the connection between them may not have been absorbed, and may go
            //      through the global routing network.

            //We trace the delay backward from the atom sink pin to the source
            //Either the atom source pin (in the same cluster), or a cluster input pin
            std::tie(net_id, sink_block_pin_index, sink_net_pin_index) = find_pb_route_clb_input_net_pin(clb_sink_block, sink_pb_route_id);

            if (net_id != ClusterNetId::INVALID() && sink_block_pin_index != -1 && sink_net_pin_index != -1) {
                //Connection leaves the CLB
                ClusterBlockId driver_block_id = cluster_ctx.clb_nlist.net_driver_block(net_id);
                VTR_ASSERT(driver_block_id == clb_src_block);

                src_block_pin_index = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, 0);

                tatum::Time driver_clb_delay = tatum::Time(clb_delay_calc_.internal_src_to_clb_output_delay(driver_block_id,
                                                                                                            src_block_pin_index,
                                                                                                            src_pb_route_id,
                                                                                                            delay_type));

                tatum::Time net_delay = tatum::Time(inter_cluster_delay(net_id, 0, sink_net_pin_index));

                tatum::Time sink_clb_delay = tatum::Time(clb_delay_calc_.clb_input_to_internal_sink_delay(clb_sink_block,
                                                                                                          sink_block_pin_index,
                                                                                                          sink_pb_route_id,
                                                                                                          delay_type));

                edge_delay = driver_clb_delay + net_delay + sink_clb_delay;

                //Save the clb delays (but not the net delay since it may change during placement)
                //Also save the source and sink pins associated with these delays
                set_driver_clb_cached_delay(edge_id, delay_type, driver_clb_delay);
                set_sink_clb_cached_delay(edge_id, delay_type, sink_clb_delay);

                src_pin = cluster_ctx.clb_nlist.net_driver(net_id);
                sink_pin = *(cluster_ctx.clb_nlist.net_pins(net_id).begin() + sink_net_pin_index);
                VTR_ASSERT(src_pin != ClusterPinId::INVALID());
                VTR_ASSERT(sink_pin != ClusterPinId::INVALID());
                set_cached_pins(edge_id, delay_type, src_pin, sink_pin);

#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
                VTR_LOG("  Edge %zu net delay: %g = %g + %g + %g (= clb_driver + net + clb_sink) [UNcached]\n",
                        size_t(edge_id),
                        edge_delay.value(),
                        driver_clb_delay.value(),
                        net_delay.value(),
                        sink_clb_delay.value());
#endif
            } else {
                //Connection entirely within the CLB
                VTR_ASSERT(clb_src_block == clb_sink_block);

                edge_delay = tatum::Time(clb_delay_calc_.internal_src_to_internal_sink_delay(clb_src_block, src_pb_route_id, sink_pb_route_id, delay_type));

                //Save the delay, since it won't change during placement or routing
                // Note that we cache the full edge delay for edges completely contained within CLBs
                set_cached_delay(edge_id, delay_type, edge_delay);

#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
                VTR_LOG("  Edge %zu CLB Internal delay: %g [UNcached]\n", size_t(edge_id), edge_delay.value());
#endif
            }
        } else {
            //Calculate the edge delay using cached clb delays and the latest net delay
            //
            //  Note: we do not need to handle the case of edges fully contained within CLBs,
            //        since such delays are cached using edge_delay_cache_ and are handled earlier
            VTR_ASSERT(src_pin != ClusterPinId::INVALID());
            VTR_ASSERT(sink_pin != ClusterPinId::INVALID());

            tatum::Time driver_clb_delay = get_driver_clb_cached_delay(edge_id, delay_type);
            tatum::Time sink_clb_delay = get_sink_clb_cached_delay(edge_id, delay_type);

            VTR_ASSERT(!std::isnan(driver_clb_delay.value()));

            VTR_ASSERT(!std::isnan(sink_clb_delay.value()));

            ClusterNetId src_net = cluster_ctx.clb_nlist.pin_net(src_pin);
            VTR_ASSERT(src_net == cluster_ctx.clb_nlist.pin_net(sink_pin));
            tatum::Time net_delay = tatum::Time(inter_cluster_delay(src_net,
                                                                    0,
                                                                    cluster_ctx.clb_nlist.pin_net_index(sink_pin)));

            edge_delay = driver_clb_delay + net_delay + sink_clb_delay;
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
            VTR_LOG("  Edge %zu net delay: %g = %g + %g + %g (= clb_driver + net + clb_sink) [Cached]\n",
                    size_t(edge_id),
                    edge_delay.value(),
                    driver_clb_delay.value(),
                    net_delay.value(),
                    sink_clb_delay.value());
#endif
        }
    } else {
#ifdef POST_CLUSTER_DELAY_CALC_DEBUG
        VTR_LOG("  Edge %zu CLB Internal delay: %g [Cached]\n", size_t(edge_id), edge_delay.value());
#endif
    }

    VTR_ASSERT(!std::isnan(edge_delay.value()));

    return edge_delay;
}

inline float PostClusterDelayCalculator::inter_cluster_delay(const ClusterNetId net_id, const int src_net_pin_index, const int sink_net_pin_index) const {
    VTR_ASSERT(src_net_pin_index == 0);

    //TODO: support minimum net delays
    return net_delay_[net_id][sink_net_pin_index];
}

inline tatum::Time PostClusterDelayCalculator::get_cached_delay(tatum::EdgeId edge, DelayType delay_type) const {
    if (delay_type == DelayType::MAX) {
        return edge_max_delay_cache_[edge];
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        return edge_min_delay_cache_[edge];
    }
}

inline void PostClusterDelayCalculator::set_cached_delay(tatum::EdgeId edge, DelayType delay_type, tatum::Time delay) const {
    if (delay_type == DelayType::MAX) {
        edge_max_delay_cache_[edge] = delay;
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        edge_min_delay_cache_[edge] = delay;
    }
}

inline void PostClusterDelayCalculator::set_driver_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type, tatum::Time delay) const {
    if (delay_type == DelayType::MAX) {
        driver_clb_max_delay_cache_[edge] = delay;
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        driver_clb_min_delay_cache_[edge] = delay;
    }
}

inline tatum::Time PostClusterDelayCalculator::get_driver_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type) const {
    if (delay_type == DelayType::MAX) {
        return driver_clb_max_delay_cache_[edge];
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        return driver_clb_min_delay_cache_[edge];
    }
}

inline void PostClusterDelayCalculator::set_sink_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type, tatum::Time delay) const {
    if (delay_type == DelayType::MAX) {
        sink_clb_max_delay_cache_[edge] = delay;
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        sink_clb_min_delay_cache_[edge] = delay;
    }
}

inline tatum::Time PostClusterDelayCalculator::get_sink_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type) const {
    if (delay_type == DelayType::MAX) {
        return sink_clb_max_delay_cache_[edge];
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        return sink_clb_min_delay_cache_[edge];
    }
}

inline tatum::Time PostClusterDelayCalculator::get_cached_setup_time(tatum::EdgeId edge) const {
    //We store the cached setup times in the max delay cache, since there will be no regular edge
    //delays on setup edges
    return edge_max_delay_cache_[edge];
}

inline void PostClusterDelayCalculator::set_cached_setup_time(tatum::EdgeId edge, tatum::Time setup) const {
    edge_max_delay_cache_[edge] = setup;
}

inline tatum::Time PostClusterDelayCalculator::get_cached_hold_time(tatum::EdgeId edge) const {
    //We store the cached hold times in the min delay cache, since there will be no regular edge
    //delays on hold edges
    return edge_min_delay_cache_[edge];
}

inline void PostClusterDelayCalculator::set_cached_hold_time(tatum::EdgeId edge, tatum::Time hold) const {
    edge_min_delay_cache_[edge] = hold;
}

inline std::pair<ClusterPinId, ClusterPinId> PostClusterDelayCalculator::get_cached_pins(tatum::EdgeId edge, DelayType delay_type) const {
    if (delay_type == DelayType::MAX) {
        return pin_cache_max_[edge];
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        return pin_cache_min_[edge];
    }
}

inline void PostClusterDelayCalculator::set_cached_pins(tatum::EdgeId edge, DelayType delay_type, ClusterPinId src_pin, ClusterPinId sink_pin) const {
    if (delay_type == DelayType::MAX) {
        pin_cache_max_[edge] = std::make_pair(src_pin, sink_pin);
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        pin_cache_min_[edge] = std::make_pair(src_pin, sink_pin);
    }
}
