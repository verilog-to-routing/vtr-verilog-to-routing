#ifndef ATOM_MAP_H
#define ATOM_MAP_H
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "vpr_types.h"
#include "vtr_bimap.h"
#include "timing_graph_fwd.hpp"
/*
 * The AtomMap class describes the mapping between components in the AtomNetlist
 * and other netlists/entities (i.e. atom block <-> t_pb, atom block <-> clb)
 */
class AtomMap {
    public:
        /*
         * PBs
         */
        //Returns the pb associated with blk_id
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
        const t_pb_graph_pin* atom_pin_pb_graph_pin(AtomPinId atom_pin) const;
        AtomPinId pb_graph_pin_atom_pin(const t_pb_graph_pin* gpin) const;
        void set_atom_pin_pb_graph_pin(AtomPinId atom_pin, const t_pb_graph_pin* gpin) const;

        /*
         * Blocks
         */
        //Returns the clb index associated with blk_id
        int atom_clb(const AtomBlockId blk_id) const;

        //Returns the atom block id associated with clb_block_index
        AtomBlockId clb_atom(const int clb_block_index) const;

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
         * Timing Nodes
         */
        //Returns the tnode index associated with the atom pin
        int atom_pin_tnode(const AtomPinId pin_id) const;
        //Returns the atom pin id associated with the tnode index
        AtomPinId tnode_atom_pin(const int tnode_index) const;

        //Sets the bidirectional mapping between an atom pin and a tnode
        // If either pin_id or tnode_index are not valid any existing mapping
        // is removed
        void set_atom_pin_tnode(const AtomPinId pin_id, const int tnode_index);
        void set_pin_tnode(const AtomPinId pin_id, const tatum::NodeId tnode_index);

        vtr::bimap<AtomPinId,tatum::NodeId> pin_tnode;
    private:
        std::unordered_map<AtomBlockId,const t_pb*> atom_to_pb_;
        std::unordered_map<const t_pb*,AtomBlockId> pb_to_atom_;

        std::unordered_map<AtomBlockId,const t_pb_graph_node*> atom_to_lowest_cost_pb_gnode_;

        std::vector<int> atom_to_clb_;
        std::unordered_map<int,AtomBlockId> clb_to_atom_;

        std::unordered_map<AtomNetId,int> atom_to_clb_net_;
        std::unordered_map<int,AtomNetId> clb_to_atom_net_;

        std::unordered_map<AtomPinId,int> atom_pin_to_tnode_;
        std::unordered_map<int,AtomPinId> tnode_to_atom_pin_;

};

#endif
