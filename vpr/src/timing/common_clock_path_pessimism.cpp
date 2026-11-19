/**
 * @file
 * @brief Implementation of the common clock path pessimism removal (CCPP) credit of each clock.
 */

#include "common_clock_path_pessimism.h"

#include <algorithm>
#include <tuple>

#include "tatum/TimingConstraints.hpp"
#include "tatum/TimingGraph.hpp"

#include "atom_lookup.h"
#include "atom_netlist.h"
#include "clb_delay_calc.h"
#include "clustered_netlist.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"

/**
 * @brief Finds the clock net through which the clock signal leaves its source cluster.
 *
 *   @param clock_source  The clock domain's source node in the timing graph.
 *   @param timing_graph  The timing graph.
 *   @param atom_netlist  The atom netlist.
 *   @param atom_lookup   The atom to cluster and timing graph lookup.
 *
 *   @return The clock net shared by every clock sink of clock_source, or ClusterNetId::INVALID() if
 *           there is no such single net.
 */
static ClusterNetId find_shared_clock_net(tatum::NodeId clock_source,
                                          const tatum::TimingGraph& timing_graph,
                                          const AtomNetlist& atom_netlist,
                                          const AtomLookup& atom_lookup);

/**
 * @brief Returns the delay variation (late - early) inside the clock source's cluster, from the clock
 *        source to the cluster output pin that drives clock_net.
 *
 *   @param clock_source  The clock domain's source node in the timing graph.
 *   @param clock_net     The clock net driven by the clock source's cluster.
 *   @param atom_lookup   The atom to cluster and timing graph lookup.
 *   @param clb_nlist     The clustered netlist.
 */
static tatum::Time driver_cluster_variation(tatum::NodeId clock_source,
                                            ClusterNetId clock_net,
                                            const AtomLookup& atom_lookup,
                                            const ClusteredNetlist& clb_nlist);

void set_common_clock_path_pessimism(tatum::TimingConstraints& timing_constraints,
                                     const tatum::TimingGraph& timing_graph,
                                     const AtomNetlist& atom_netlist,
                                     const AtomLookup& atom_lookup,
                                     const ClusteredNetlist& clb_nlist,
                                     bool is_flat) {
    // With flat routing, the clock paths (including the clock source's cluster) are all routing, and the
    // timing analysis uses the routing's max delays for both setup and hold (see issue #3946). There is
    // then no variation to credit.
    if (is_flat) {
        for (tatum::DomainId domain : timing_constraints.clock_domains()) {
            timing_constraints.set_common_clock_path_pessimism(domain, tatum::Time(0.));
        }
        return;
    }

    // A domain's credit is the variation of the clock path shared by all of its clock sinks. Currently
    // this is the part inside the clock source's cluster (e.g. a clock input pad), and virtual clocks
    // get none.
    for (tatum::DomainId domain : timing_constraints.clock_domains()) {
        tatum::Time shared_variation(0.);

        tatum::NodeId clock_source = timing_constraints.clock_domain_source_node(domain);
        ClusterNetId clock_net = ClusterNetId::INVALID();
        if (clock_source) {
            clock_net = find_shared_clock_net(clock_source, timing_graph, atom_netlist, atom_lookup);
        }

        if (clock_net) {
            shared_variation += driver_cluster_variation(clock_source, clock_net, atom_lookup, clb_nlist);

            // The routing of clock_net up to its first branch point is also shared by every clock
            // sink, but isn't credited yet. Only the final timing analysis uses the routing's min
            // delays, so this is where its variation would be added (for the final analysis only).
        }

        timing_constraints.set_common_clock_path_pessimism(domain, shared_variation);

        if (shared_variation.value() > 0.) {
            VTR_LOG("Common clock path pessimism removal: clock '%s' credits %g ns (shared clock path up to its clock net driver)\n",
                    timing_constraints.clock_domain_name(domain).c_str(), 1e9 * shared_variation.value());
        }
    }
}

static ClusterNetId find_shared_clock_net(tatum::NodeId clock_source,
                                          const tatum::TimingGraph& timing_graph,
                                          const AtomNetlist& atom_netlist,
                                          const AtomLookup& atom_lookup) {
    // The timing graph has one edge per clock sink, each holding the delay of the whole connection
    // (source cluster, routing and sink cluster). The clock path is shared up to the source cluster's
    // output pin only if every edge leaves the cluster through the same clock net.
    ClusterNetId shared_clock_net = ClusterNetId::INVALID();

    for (tatum::EdgeId edge : timing_graph.node_out_edges(clock_source)) {
        if (timing_graph.edge_type(edge) != tatum::EdgeType::INTERCONNECT) return ClusterNetId::INVALID();

        AtomPinId sink_pin = atom_lookup.tnode_atom_pin(timing_graph.edge_sink_node(edge));
        VTR_ASSERT(sink_pin);

        ClusterBlockId sink_clb = atom_lookup.atom_clb(atom_netlist.pin_block(sink_pin));
        const t_pb_graph_pin* sink_gpin = atom_lookup.atom_pin_pb_graph_pin(sink_pin);
        VTR_ASSERT(sink_clb && sink_gpin);

        ClusterNetId clock_net;
        int sink_block_pin_index;
        int sink_net_pin_index;
        std::tie(clock_net, sink_block_pin_index, sink_net_pin_index) = find_pb_route_clb_input_net_pin(sink_clb, sink_gpin->pin_count_in_cluster);

        // An invalid net means the connection stays inside the source cluster
        if (!clock_net) return ClusterNetId::INVALID();
        if (shared_clock_net && clock_net != shared_clock_net) return ClusterNetId::INVALID();
        shared_clock_net = clock_net;
    }

    return shared_clock_net;
}

static tatum::Time driver_cluster_variation(tatum::NodeId clock_source,
                                            ClusterNetId clock_net,
                                            const AtomLookup& atom_lookup,
                                            const ClusteredNetlist& clb_nlist) {
    AtomPinId src_pin = atom_lookup.tnode_atom_pin(clock_source);
    const t_pb_graph_pin* src_gpin = atom_lookup.atom_pin_pb_graph_pin(src_pin);
    ClusterBlockId driver_clb = clb_nlist.net_driver_block(clock_net);
    int src_block_pin_index = clb_nlist.net_pin_logical_index(clock_net, 0);
    VTR_ASSERT(src_gpin && driver_clb && src_block_pin_index >= 0);

    ClbDelayCalc clb_delay_calc;
    float max_delay = clb_delay_calc.internal_src_to_clb_output_delay(driver_clb, src_block_pin_index, src_gpin->pin_count_in_cluster, DelayType::MAX);
    float min_delay = clb_delay_calc.internal_src_to_clb_output_delay(driver_clb, src_block_pin_index, src_gpin->pin_count_in_cluster, DelayType::MIN);

    return tatum::Time(std::max(0.f, max_delay - min_delay));
}
