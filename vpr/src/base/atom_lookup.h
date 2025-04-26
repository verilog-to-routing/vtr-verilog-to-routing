#ifndef ATOM_LOOKUP_H
#define ATOM_LOOKUP_H
#include "atom_lookup_fwd.h"

#include <unordered_map>

#include "vtr_bimap.h"
#include "vtr_vector_map.h"
#include "vtr_range.h"

#include "atom_netlist_fwd.h"
#include "clustered_netlist_fwd.h"
#include "vpr_types.h"
#include "tatum/TimingGraphFwd.hpp"

#include "vtr_optional.h"
#include "atom_pb_bimap.h"

/**
 * @brief The AtomLookup class describes the mapping between components in the AtomNetlist
 *        and other netlists/entities (i.e. atom block <-> t_pb, atom block <-> clb)
 */
class AtomLookup {
  public:
    typedef vtr::linear_map<AtomPinId, tatum::NodeId>::const_iterator pin_tnode_iterator;

    typedef vtr::Range<pin_tnode_iterator> pin_tnode_range;

  public:
    /*
     * PBs
     */

    /**
     * @brief Sets the atom to pb bimap access lock to value.
     * If set to true, access to the bimap is prohibited and will result in failing assertions.
     * 
     * @param value Value to set to lock to
     */
    inline void set_atom_pb_bimap_lock(bool value) {
        VTR_ASSERT_SAFE_MSG(lock_atom_pb_bimap_ != value, "Double locking or unlocking the atom pb bimap lock");
        lock_atom_pb_bimap_ = value;
    }

    /// @brief Gets the current atom to pb bimap lock value.
    inline bool atom_pb_bimap_islocked() const { return lock_atom_pb_bimap_; }

    // All accesses, mutable or immutable, to the atom to pb bimap
    // will result in failing assertions if the lock is set to true.
    // This is done to make sure there is only a single source of
    // data in places that are supposed to use a local data structure
    // instead of the global context.

    /// @brief Returns a mutable reference to the atom to pb bimap, provided that access to it is unlocked. It will result in a crash otherwise.
    /// @return Mutable reference to the atom pb bimap.
    inline AtomPBBimap& mutable_atom_pb_bimap() {
        VTR_ASSERT(!lock_atom_pb_bimap_);
        return atom_to_pb_bimap_;
    }

    /// @brief Returns an immutable reference to the atom to pb bimap, provided that access to it is unlocked. It will result in a crash otherwise.
    /// @return Immutable reference to the atom pb bimap.
    inline const AtomPBBimap& atom_pb_bimap() const {
        VTR_ASSERT(!lock_atom_pb_bimap_);
        return atom_to_pb_bimap_;
    }

    /**
     * @brief Set atom to pb bimap
     * 
     * @param atom_to_pb Reference to AtomPBBimab to be copied from
     */
    void set_atom_to_pb_bimap(const AtomPBBimap& atom_to_pb) { atom_to_pb_bimap_ = atom_to_pb; }

    /*
     * PB Pins
     */

    ///@brief Returns the pb graph pin associated with the specified atom pin
    const t_pb_graph_pin* atom_pin_pb_graph_pin(AtomPinId atom_pin) const;

    ///@brief Sets the mapping between an atom pin and pb graph pin
    void set_atom_pin_pb_graph_pin(AtomPinId atom_pin, const t_pb_graph_pin* gpin);

    /*
     * Blocks
     */

    ///@brief Returns the clb index associated with blk_id
    ClusterBlockId atom_clb(const AtomBlockId blk_id) const;

    /**
     * @brief Sets the bidirectional mapping between an atom and clb
     *
     * If either blk_id or clb_index are not valid any existing mapping is removed
     */
    void set_atom_clb(const AtomBlockId blk_id, const ClusterBlockId clb);

    /*
     * Nets
     */

    ///@brief Returns the atom net id associated with the clb_net_index
    AtomNetId atom_net(const ClusterNetId cluster_net_id) const;

    ///@brief Returns the clb net indices associated with atom_net_id
    vtr::optional<const std::vector<ClusterNetId>&> clb_nets(const AtomNetId atom_net_id) const;

    /**
     * @brief Sets the bidirectional mapping between an atom net and a clb net
     */
    void add_atom_clb_net(const AtomNetId atom_net, const ClusterNetId clb_net);

    /** Remove given clb net from the mapping */
    void remove_clb_net(const ClusterNetId clb_net);

    /** Remove given atom net from the mapping */
    void remove_atom_net(const AtomNetId atom_net);

    /*
     * Timing Nodes
     */

    ///@brief Returns the timing graph node associated with the specified atom netlist pin
    tatum::NodeId atom_pin_tnode(const AtomPinId pin, BlockTnode block_tnode_type = BlockTnode::EXTERNAL) const;

    ///@brief Returns the atom netlist pin associated with the specified timing graph node
    AtomPinId tnode_atom_pin(const tatum::NodeId tnode) const;

    ///@brief Returns a range of all pin to tnode mappingsg of the specified type
    AtomLookup::pin_tnode_range atom_pin_tnodes(BlockTnode block_tnode_type) const;

    ///@brief Sets the bi-directional mapping between an atom netlist pin and timing graph node
    void set_atom_pin_tnode(const AtomPinId pin, const tatum::NodeId node, BlockTnode block_tnode_type);

  private: //Types
  private:
    /**
     * @brief Allows or disallows access to the AtomPBBimap data.
     * Useful to make sure global context is not accessed in places you don't want it to.
     */
    bool lock_atom_pb_bimap_ = false;
    AtomPBBimap atom_to_pb_bimap_;

    vtr::vector_map<AtomPinId, const t_pb_graph_pin*> atom_pin_to_pb_graph_pin_;

    vtr::vector_map<AtomBlockId, ClusterBlockId> atom_to_clb_;

    vtr::linear_map<AtomNetId, std::vector<ClusterNetId>> atom_net_to_clb_nets_;
    vtr::linear_map<ClusterNetId, AtomNetId> clb_net_to_atom_net_;

    vtr::linear_map<AtomPinId, tatum::NodeId> atom_pin_tnode_external_;
    vtr::linear_map<AtomPinId, tatum::NodeId> atom_pin_tnode_internal_;
    vtr::linear_map<tatum::NodeId, AtomPinId> tnode_atom_pin_;
};

#endif
