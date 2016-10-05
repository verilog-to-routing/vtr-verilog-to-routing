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

        //Returns the atom block id assoicated with pb_val
        AtomBlockId pb_atom(const t_pb* pb) const;

        //Conveneince wrapper around atom_pb to access the associated graph node
        const t_pb_graph_node* atom_pb_graph_node(const AtomBlockId blk_id) const;

        //Returns the clb index associated with blk_id
        int atom_clb(const AtomBlockId blk_id) const;

        //Returns the atom block id associated with clb_index_val
        AtomBlockId clb_atom(const int clb_indexl) const;

        //Sets the bidirectional mapping between an atom and pb
        // If either blk_id or pb are not valid any existing mapping
        // is removed
        void set_atom_pb(const AtomBlockId blk_id, const t_pb* pb);

        //Sets the bidirectional mapping between an atom and clb
        // If either blk_id or clb_index are not valid any existing mapping
        // is removed
        void set_atom_clb(const AtomBlockId blk_id, const int clb_index);
    private:
        std::unordered_map<AtomBlockId,const t_pb*> atom_to_pb_;
        std::unordered_map<const t_pb*,AtomBlockId> pb_to_atom_;
        std::unordered_map<AtomBlockId,int> atom_to_clb_;
        std::unordered_map<int,AtomBlockId> clb_to_atom_;
};

#endif
