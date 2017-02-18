#ifndef ATOM_MAP_H
#define ATOM_MAP_H
#include <unordered_map>

#include "vtr_bimap.h"
#include "vtr_vector_map.h"
#include "vtr_range.h"

#include "atom_netlist_fwd.h"
#include "vpr_types.h"
#include "timing_graph_fwd.hpp"
/*
 * The AtomMap class describes the mapping between components in the AtomNetlist
 * and other netlists/entities (i.e. atom block <-> t_pb, atom block <-> clb)
 */
class AtomMap {
    public:
        typedef vtr::linear_bimap<AtomPinId,tatum::NodeId>::iterator pin_tnode_iterator;

        typedef vtr::Range<pin_tnode_iterator> pin_tnode_range;
    public:
        /*
         * PBs
         */
        //Returns the leaf pb associated with the atom blk_id
        //  Note: this is the lowest level pb which corresponds directly to the atom block
        const t_pb* atom_pb(const AtomBlockId blk_id) const;

        //Returns the atom block id assoicated with pb
        AtomBlockId pb_atom(const t_pb* pb) const;

        //Conveneince wrapper around atom_pb to access the associated graph node
        const t_pb_graph_node* atom_pb_graph_node(const AtomBlockId blk_id) const;

        //Sets the bidirectional mapping between an atom and pb
        // If either blk_id or pb are not valid any existing mapping
        // is removed
        void set_atom_pb(const AtomBlockId blk_id, const t_pb* pb);

        /*
         * PB Pins
         */
        //Returns the pb graph pin associated with the specified atom pin
        const t_pb_graph_pin* atom_pin_pb_graph_pin(AtomPinId atom_pin) const;

        //Sets the mapping between an atom pin and pb graph pin
        void set_atom_pin_pb_graph_pin(AtomPinId atom_pin, const t_pb_graph_pin* gpin);

        /*
         * Blocks
         */
        //Returns the clb index associated with blk_id
        int atom_clb(const AtomBlockId blk_id) const;

        //Sets the bidirectional mapping between an atom and clb
        // If either blk_id or clb_index are not valid any existing mapping
        // is removed
        void set_atom_clb(const AtomBlockId blk_id, const int clb_index);

        /*
         * Nets
         */
        //Returns the atom net id associated with the clb_net_index
        AtomNetId atom_net(const int clb_net_index) const;

        //Returns the clb net index associated with net_id
        int clb_net(const AtomNetId net_id) const;

        //Sets the bidirectional mapping between an atom net and a clb net
        // If either net_id or clb_net_index are not valid any existing mapping
        // is removed
        void set_atom_clb_net(const AtomNetId net_id, const int clb_net_index);

        /*
         * Classic Timing Nodes
         */
        //Returns the tnode index associated with the atom pin
        int atom_pin_classic_tnode(const AtomPinId pin_id) const;
        //Returns the atom pin id associated with the tnode index
        AtomPinId classic_tnode_atom_pin(const int tnode_index) const;

        //Sets the bidirectional mapping between an atom pin and a tnode
        // If either pin_id or tnode_index are not valid any existing mapping
        // is removed
        void set_atom_pin_classic_tnode(const AtomPinId pin_id, const int tnode_index);

        /*
         * Timing Nodes
         */
        //Returns the timing graph node associated with the specified atom netlist pin
        tatum::NodeId pin_tnode(const AtomPinId pin) const;

        //Returns the atom netlist pin associated with the specified timing graph node 
        AtomPinId tnode_pin(const tatum::NodeId tnode) const;

        //Returns a range of all pin to tnode mappings
        pin_tnode_range pin_tnodes() const;

        //Sets the bi-directional mapping between an atom netlist pin and timing graph node
        void set_pin_tnode(const AtomPinId pin, const tatum::NodeId node);
    private:
        template<class K, class V>
        using linear_map_int = vtr::linear_map<K,V,vtr::MinusOneSentinel<K>>;

        vtr::bimap<AtomBlockId,const t_pb*, vtr::linear_map, std::unordered_map> atom_to_pb_;

        vtr::vector_map<AtomPinId,const t_pb_graph_pin*> atom_to_pb_graph_pin_;

        vtr::vector_map<AtomBlockId,int> atom_to_clb_;

        std::unordered_map<AtomNetId,int> atom_to_clb_net_;
        std::unordered_map<int,AtomNetId> clb_to_atom_net_;

        std::unordered_map<AtomPinId,int> atom_pin_to_tnode_;
        std::unordered_map<int,AtomPinId> tnode_to_atom_pin_;

        vtr::linear_bimap<AtomPinId,tatum::NodeId> pin_tnode_;
};

#endif
