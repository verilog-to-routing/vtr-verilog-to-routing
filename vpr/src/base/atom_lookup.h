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
     * @brief Returns the leaf pb associated with the atom blk_id
     * @note  this is the lowest level pb which corresponds directly to the atom block
     */
    const t_pb* atom_pb(const AtomBlockId blk_id) const;

    ///@brief Returns the atom block id associated with pb
    AtomBlockId pb_atom(const t_pb* pb) const;

    ///@brief Conveneince wrapper around atom_pb to access the associated graph node
    const t_pb_graph_node* atom_pb_graph_node(const AtomBlockId blk_id) const;

    /**
     * @brief Sets the bidirectional mapping between an atom and pb
     *
     * If either blk_id or pb are not valid any, existing mapping is removed
     */
    void set_atom_pb(const AtomBlockId blk_id, const t_pb* pb);

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
    AtomNetId atom_net(const ClusterNetId clb_net_index) const;

    ///@brief Returns the clb net index associated with net_id
    ClusterNetId clb_net(const AtomNetId net_id) const;

    /**
     * @brief Sets the bidirectional mapping between an atom net and a clb net
     *
     * If either net_id or clb_net_index are not valid any existing mapping is removed
     */
    void set_atom_clb_net(const AtomNetId net_id, const ClusterNetId clb_net_index);

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
    vtr::bimap<AtomBlockId, const t_pb*, vtr::linear_map, std::unordered_map> atom_to_pb_;

    vtr::vector_map<AtomPinId, const t_pb_graph_pin*> atom_pin_to_pb_graph_pin_;

    vtr::vector_map<AtomBlockId, ClusterBlockId> atom_to_clb_;

    vtr::bimap<AtomNetId, ClusterNetId, vtr::linear_map, vtr::linear_map> atom_net_to_clb_net_;

    vtr::linear_map<AtomPinId, tatum::NodeId> atom_pin_tnode_external_;
    vtr::linear_map<AtomPinId, tatum::NodeId> atom_pin_tnode_internal_;
    vtr::linear_map<tatum::NodeId, AtomPinId> tnode_atom_pin_;
};

#endif
