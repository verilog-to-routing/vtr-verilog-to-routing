#pragma once

#include "netlist_fwd.h"
#include "tatum/TimingGraphFwd.hpp"
#include "timing_info.h"
#include "vtr_range.h"

#include "vtr_vec_id_set.h"

#ifdef VPR_USE_TBB
#    include <tbb/concurrent_unordered_set.h>
#endif

/** Make NetPinTimingInvalidator a virtual class since it does nothing for the general case of non-incremental
 * timing updates. It should really be templated to not pay the cost for vtable lookups, but this is the
 * best approach without putting a template on every function which uses this machine. */
class NetPinTimingInvalidator {
  public:
    typedef vtr::Range<const tatum::EdgeId*> tedge_range;
    virtual ~NetPinTimingInvalidator() = default;
    virtual tedge_range pin_timing_edges(ParentPinId /* pin */) const = 0;
    virtual void invalidate_connection(ParentPinId /* pin */, TimingInfo* /* timing_info */) = 0;
    virtual void reset() = 0;
};

//Helper class for iterating through the timing edges associated with a particular
//clustered netlist pin, and invalidating them.
//
//For efficiency, it pre-calculates and stores the mapping from ClusterPinId -> tatum::EdgeIds,
//and tracks whether a particular ClusterPinId has been already invalidated (to avoid the expense
//of invalidating it multiple times)
class IncrNetPinTimingInvalidator : public NetPinTimingInvalidator {
  public:
    IncrNetPinTimingInvalidator(const Netlist<>& net_list,
                                const ClusteredPinAtomPinsLookup& clb_atom_pin_lookup,
                                const AtomNetlist& atom_nlist,
                                const AtomLookup& atom_lookup,
                                const tatum::TimingGraph& timing_graph,
                                bool is_flat) {
        size_t num_pins = net_list.pins().size();
        pin_first_edge_.reserve(num_pins + 1); //Exact
        timing_edges_.reserve(num_pins + 1);   //Lower bound
        for (ParentPinId pin_id : net_list.pins()) {
            pin_first_edge_.push_back(timing_edges_.size());
            if (is_flat) {
                tatum::EdgeId tedge = atom_pin_to_timing_edge(timing_graph, atom_nlist, atom_lookup, convert_to_atom_pin_id(pin_id));

                if (!tedge) {
                    continue;
                }

                timing_edges_.push_back(tedge);
            } else {
                auto cluster_pin_id = convert_to_cluster_pin_id(pin_id);
                auto atom_pins = clb_atom_pin_lookup.connected_atom_pins(cluster_pin_id);
                for (const AtomPinId atom_pin : atom_pins) {
                    tatum::EdgeId tedge = atom_pin_to_timing_edge(timing_graph, atom_nlist, atom_lookup, atom_pin);

                    if (!tedge) {
                        continue;
                    }

                    timing_edges_.push_back(tedge);
                }
            }
        }

        //Sentinels
        timing_edges_.push_back(tatum::EdgeId::INVALID());
        pin_first_edge_.push_back(timing_edges_.size());

        VTR_ASSERT(pin_first_edge_.size() == net_list.pins().size() + 1);
    }

    //Returns the set of timing edges associated with the specified cluster pin
    tedge_range pin_timing_edges(ParentPinId pin) const {
        int ipin = size_t(pin);
        return vtr::make_range(&timing_edges_[pin_first_edge_[ipin]],
                               &timing_edges_[pin_first_edge_[ipin + 1]]);
    }

    /** Invalidates all timing edges associated with the clustered netlist connection
     * driving the specified pin.
     * Is concurrently safe. */
    void invalidate_connection(ParentPinId pin, TimingInfo* timing_info) {
        if (invalidated_pins_.count(pin)) return; //Already invalidated

        for (tatum::EdgeId edge : pin_timing_edges(pin)) {
            timing_info->invalidate_delay(edge);
        }

        invalidated_pins_.insert(pin);
    }

    /** Resets invalidation state for this class
     * Not concurrently safe! */
    void reset() {
        invalidated_pins_.clear();
    }

  private:
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
    std::vector<int> pin_first_edge_; //Indices into timing_edges corresponding
    std::vector<tatum::EdgeId> timing_edges_;

    /** Cache for invalidated pins. Use concurrent set when TBB is turned on, since the
     * invalidator may be shared between threads */
#ifdef VPR_USE_TBB
    tbb::concurrent_unordered_set<ParentPinId> invalidated_pins_;
#else
    vtr::vec_id_set<ParentPinId> invalidated_pins_;
#endif
};

/** NetPinTimingInvalidator is only a rube goldberg machine when incremental timing analysis
 * is disabled, since timing_info->invalidate_delay does nothing. Use this class when incremental
 * STA is disabled. */
class NoopNetPinTimingInvalidator : public NetPinTimingInvalidator {
  public:
    tedge_range pin_timing_edges(ParentPinId /* pin */) const {
        return vtr::make_range((const tatum::EdgeId*)nullptr, (const tatum::EdgeId*)nullptr);
    }

    void invalidate_connection(ParentPinId /* pin */, TimingInfo* /* timing_info */) {
    }

    void reset() {
    }
};

/** Make a NetPinTimingInvalidator depending on update_type. Will return a NoopInvalidator if it's not INCREMENTAL. */
inline std::unique_ptr<NetPinTimingInvalidator> make_net_pin_timing_invalidator(
    e_timing_update_type update_type,
    const Netlist<>& net_list,
    const ClusteredPinAtomPinsLookup& clb_atom_pin_lookup,
    const AtomNetlist& atom_nlist,
    const AtomLookup& atom_lookup,
    const tatum::TimingGraph& timing_graph,
    bool is_flat) {
    if (update_type == e_timing_update_type::FULL || update_type == e_timing_update_type::AUTO) {
        return std::make_unique<NoopNetPinTimingInvalidator>();
    } else {
        VTR_ASSERT(update_type == e_timing_update_type::INCREMENTAL);
        return std::make_unique<IncrNetPinTimingInvalidator>(net_list, clb_atom_pin_lookup, atom_nlist, atom_lookup, timing_graph, is_flat);
    }
}