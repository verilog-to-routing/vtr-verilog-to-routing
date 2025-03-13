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

    bool lock_atom_pb_bimap = false;

  public:
    /*
     * PBs
     */

    inline AtomPBBimap &mutable_atom_pb_bimap() {VTR_ASSERT(!lock_atom_pb_bimap); return atom_to_pb_bimap_;}
    inline const AtomPBBimap &atom_pb_bimap() const {VTR_ASSERT(!lock_atom_pb_bimap); return atom_to_pb_bimap_;}

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
    
    // Setter function for atom_to_pb_
    void set_atom_to_pb_bimap(const AtomPBBimap& atom_to_pb){
      atom_to_pb_bimap_ = atom_to_pb;
    }

  private: //Types
  private:
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
