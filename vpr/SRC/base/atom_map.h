#ifndef ATOM_MAP_H
#define ATOM_MAP_H
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "vpr_types.h"
/*
 * The AtomMap class describes the mapping between components in the AtomNetlist
 * and other netlists/entities (i.e. atom block <-> t_pb, atom block <-> clb)
 */
class AtomMap {
    public:
        //Returns the pb associated with blk_id
        const t_pb* atom_pb(const AtomBlockId blk_id) const;

        //Returns the atom block id assoicated with pb
        AtomBlockId pb_atom(const t_pb* pb) const;

        //Conveneince wrapper around atom_pb to access the associated graph node
        const t_pb_graph_node* atom_pb_graph_node(const AtomBlockId blk_id) const;

        //Returns the clb index associated with blk_id
        int atom_clb(const AtomBlockId blk_id) const;

        //Returns the atom block id associated with clb_block_index
        AtomBlockId clb_atom(const int clb_block_index) const;

        //Returns the atom net id associated with the clb_net_index
        AtomNetId atom_net(const int clb_net_index) const;

        //Returns the clb net index associated with net_id
        int clb_net(const AtomNetId net_id) const;

        //Sets the bidirectional mapping between an atom and pb
        // If either blk_id or pb are not valid any existing mapping
        // is removed
        void set_atom_pb(const AtomBlockId blk_id, const t_pb* pb);

        //Sets the bidirectional mapping between an atom and clb
        // If either blk_id or clb_index are not valid any existing mapping
        // is removed
        void set_atom_clb(const AtomBlockId blk_id, const int clb_index);

        void set_atom_clb_net(const AtomNetId net_id, const int clb_net_index);
    private:
        std::unordered_map<AtomBlockId,const t_pb*> atom_to_pb_;
        std::unordered_map<const t_pb*,AtomBlockId> pb_to_atom_;

        std::unordered_map<AtomBlockId,int> atom_to_clb_;
        std::unordered_map<int,AtomBlockId> clb_to_atom_;

        std::unordered_map<AtomNetId,int> atom_to_clb_net_;
        std::unordered_map<int,AtomNetId> clb_to_atom_net_;
};

#endif
