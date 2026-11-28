#pragma once

#include <array>

#include "clustered_netlist.h"
#include "clustered_netlist_utils.h"
#include "netlist.h"
#include "routing_netlist_fwd.h"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "timing_info.h"
#include "vtr_range.h"
#include "move_transactions.h"

#ifdef VPR_USE_TBB
#include <tbb/concurrent_unordered_set.h>
#else
#include "vtr_vec_id_set.h"
#endif

/** Make NetPinTimingInvalidator a virtual class since it does nothing for the general case of non-incremental
 * timing updates. It should really be templated to not pay the cost for vtable lookups, but this is the
 * best approach without putting a template on every function which uses this machine. */
class NetPinTimingInvalidator {
  public:
    typedef vtr::Range<const tatum::EdgeId*> tedge_range;
    virtual ~NetPinTimingInvalidator() = default;
    virtual tedge_range pin_timing_edges(RoutingPinId /* pin */) const = 0;
    virtual void invalidate_connection(RoutingPinId /* pin */) = 0;
    virtual void reset() = 0;

    /**
     * @brief Invalidates the connections affected by the specified block moves.
     *
     * All the connections recorded in blocks_affected.affected_pins have a proposed
     * delay that differs from their committed connection_delay.
     *
     * Invalidate all the timing graph edges associated with these connections via
     * the NetPinTimingInvalidator class.
     */
    void invalidate_affected_connections(const t_pl_blocks_to_be_moved& blocks_affected) {
        // Invalidate timing graph edges affected by the move
        for (ClusterPinId pin : blocks_affected.affected_pins) {
            invalidate_connection(pin);
        }
    }
};

//Helper class for iterating through the timing edges associated with a particular
//pin of the netlist used by routing, and invalidating them.
//
//For efficiency, it pre-calculates and stores the mapping from the pins -> tatum::EdgeIds,
//and tracks whether a particular pin has been already invalidated (to avoid the expense
//of invalidating it multiple times)
class IncrNetPinTimingInvalidator : public NetPinTimingInvalidator {
  public:
    /// @brief Builds the invalidator of the pins of the clustered netlist.
    IncrNetPinTimingInvalidator(const ClusteredNetlist& clb_nlist,
                                const ClusteredPinAtomPinsLookup& clb_atom_pin_lookup,
                                const AtomNetlist& atom_nlist,
                                const AtomLookup& atom_lookup,
                                std::shared_ptr<TimingInfo> timing_info)
        : timing_info_(std::move(timing_info)) {
        size_t num_pins = clb_nlist.pins().size();
        reserve_pins_(num_pins);
        for (ClusterPinId clb_pin : clb_nlist.pins()) {
            VTR_ASSERT_SAFE(size_t(clb_pin) == pin_first_edge_.size());
            add_pin_(clb_atom_pin_lookup.connected_atom_pins(clb_pin), atom_nlist, atom_lookup);
        }
        finish_pins_(num_pins);
    }

    /// @brief Builds the invalidator of the pins of the netlist used by routing (the atom
    ///        netlist if is_flat is true, otherwise the clustered netlist).
    IncrNetPinTimingInvalidator(const Netlist<>& net_list,
                                const ClusteredPinAtomPinsLookup& clb_atom_pin_lookup,
                                const AtomNetlist& atom_nlist,
                                const AtomLookup& atom_lookup,
                                std::shared_ptr<TimingInfo> timing_info,
                                bool is_flat)
        : timing_info_(std::move(timing_info)) {
        size_t num_pins = net_list.pins().size();
        reserve_pins_(num_pins);
        for (RoutingPinId pin_id : net_list.pins()) {
            if (is_flat) {
                std::array<AtomPinId, 1> atom_pins = {convert_to_atom_pin_id(pin_id)};
                add_pin_(atom_pins, atom_nlist, atom_lookup);
            } else {
                add_pin_(clb_atom_pin_lookup.connected_atom_pins(convert_to_cluster_pin_id(pin_id)), atom_nlist, atom_lookup);
            }
        }
        finish_pins_(num_pins);
    }

    //Returns the set of timing edges associated with the specified pin of the netlist used by routing
    tedge_range pin_timing_edges(RoutingPinId pin) const {
        int ipin = size_t(pin);
        return vtr::make_range(&timing_edges_[pin_first_edge_[ipin]],
                               &timing_edges_[pin_first_edge_[ipin + 1]]);
    }

    /** Invalidates all timing edges associated with the connection of the netlist used by routing
     * driving the specified pin.
     * Is concurrently safe. */
    void invalidate_connection(RoutingPinId pin) {
        if (invalidated_pins_.count(pin)) return; //Already invalidated

        for (tatum::EdgeId edge : pin_timing_edges(pin)) {
            timing_info_->invalidate_delay(edge);
        }

        invalidated_pins_.insert(pin);
    }

    /** Resets invalidation state for this class
     * Not concurrently safe! */
    void reset() {
        invalidated_pins_.clear();
    }

  private:
    /// @brief Allocates the space to record the timing edges of the given number of pins
    void reserve_pins_(size_t num_pins) {
        pin_first_edge_.reserve(num_pins + 1); //Exact
        timing_edges_.reserve(num_pins + 1);   //Lower bound
    }

    /// @brief Records the timing edges of the next pin, which is connected to the given atom pins
    template<typename AtomPinRange>
    void add_pin_(const AtomPinRange& atom_pins,
                  const AtomNetlist& atom_nlist,
                  const AtomLookup& atom_lookup) {
        pin_first_edge_.push_back(timing_edges_.size());
        for (const AtomPinId atom_pin : atom_pins) {
            tatum::EdgeId tedge = atom_pin_to_timing_edge(*timing_info_->timing_graph(), atom_nlist, atom_lookup, atom_pin);

            if (!tedge) {
                continue;
            }

            timing_edges_.push_back(tedge);
        }
    }

    /// @brief Closes the list of timing edges once all the pins have been added
    void finish_pins_(size_t num_pins) {
        //Sentinels
        timing_edges_.push_back(tatum::EdgeId::INVALID());
        pin_first_edge_.push_back(timing_edges_.size());

        VTR_ASSERT(pin_first_edge_.size() == num_pins + 1);
    }

    tatum::EdgeId atom_pin_to_timing_edge(const tatum::TimingGraph& timing_graph,
                                          const AtomNetlist& atom_nlist,
                                          const AtomLookup& atom_lookup,
                                          const AtomPinId atom_pin) {
        tatum::NodeId pin_tnode = atom_lookup.atom_pin_tnode(atom_pin);
        VTR_ASSERT_SAFE(pin_tnode);

        AtomNetId atom_net = atom_nlist.pin_net(atom_pin);
        VTR_ASSERT_SAFE(atom_net);

        AtomPinId atom_net_driver = atom_nlist.net_driver(atom_net);
        VTR_ASSERT_SAFE(atom_net_driver);

        tatum::NodeId driver_tnode = atom_lookup.atom_pin_tnode(atom_net_driver);
        VTR_ASSERT_SAFE(driver_tnode);

        //Find and invalidate the incoming timing edge corresponding
        //to the connection between the net driver and sink pin
        for (tatum::EdgeId edge : timing_graph.node_in_edges(pin_tnode)) {
            if (timing_graph.edge_src_node(edge) == driver_tnode) {
                //The edge corresponding to this atom pin
                return edge;
            }
        }
        return tatum::EdgeId::INVALID(); //None found
    }

  private:
    std::shared_ptr<TimingInfo> timing_info_;
    std::vector<int> pin_first_edge_; //Indices into timing_edges corresponding
    std::vector<tatum::EdgeId> timing_edges_;

    /** Cache for invalidated pins. Use concurrent set when TBB is turned on, since the
     * invalidator may be shared between threads */
#ifdef VPR_USE_TBB
    tbb::concurrent_unordered_set<RoutingPinId> invalidated_pins_;
#else
    vtr::vec_id_set<RoutingPinId> invalidated_pins_;
#endif
};

/** NetPinTimingInvalidator is only a rube goldberg machine when incremental timing analysis
 * is disabled, since timing_info->invalidate_delay does nothing. Use this class when incremental
 * STA is disabled. */
class NoopNetPinTimingInvalidator : public NetPinTimingInvalidator {
  public:
    tedge_range pin_timing_edges(RoutingPinId /* pin */) const {
        return vtr::make_range((const tatum::EdgeId*)nullptr, (const tatum::EdgeId*)nullptr);
    }

    void invalidate_connection(RoutingPinId /* pin */) {
    }

    void reset() {
    }
};

/** Make a NetPinTimingInvalidator of the pins of the clustered netlist depending on update_type.
 * Will return a NoopInvalidator if it's not INCREMENTAL. */
inline std::unique_ptr<NetPinTimingInvalidator> make_net_pin_timing_invalidator(
    e_timing_update_type update_type,
    const ClusteredNetlist& clb_nlist,
    const ClusteredPinAtomPinsLookup& clb_atom_pin_lookup,
    const AtomNetlist& atom_nlist,
    const AtomLookup& atom_lookup,
    const std::shared_ptr<TimingInfo>& timing_info) {
    if (update_type == e_timing_update_type::FULL || update_type == e_timing_update_type::AUTO) {
        return std::make_unique<NoopNetPinTimingInvalidator>();
    } else {
        VTR_ASSERT(update_type == e_timing_update_type::INCREMENTAL);
        return std::make_unique<IncrNetPinTimingInvalidator>(clb_nlist, clb_atom_pin_lookup, atom_nlist,
                                                             atom_lookup, timing_info);
    }
}

/** Make a NetPinTimingInvalidator of the pins of the netlist used by routing depending on update_type.
 * Will return a NoopInvalidator if it's not INCREMENTAL. */
inline std::unique_ptr<NetPinTimingInvalidator> make_net_pin_timing_invalidator(
    e_timing_update_type update_type,
    const Netlist<>& net_list,
    const ClusteredPinAtomPinsLookup& clb_atom_pin_lookup,
    const AtomNetlist& atom_nlist,
    const AtomLookup& atom_lookup,
    const std::shared_ptr<TimingInfo>& timing_info,
    bool is_flat) {
    if (update_type == e_timing_update_type::FULL || update_type == e_timing_update_type::AUTO) {
        return std::make_unique<NoopNetPinTimingInvalidator>();
    } else {
        VTR_ASSERT(update_type == e_timing_update_type::INCREMENTAL);
        return std::make_unique<IncrNetPinTimingInvalidator>(net_list, clb_atom_pin_lookup, atom_nlist,
                                                             atom_lookup, timing_info, is_flat);
    }
}
