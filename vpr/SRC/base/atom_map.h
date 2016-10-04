#ifndef ATOM_MAP_H
#define ATOM_MAP_H
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "vpr_types.h"

class AtomMap {
    public:
        const t_pb* atom_pb(const AtomBlockId blk_id) const;
        AtomBlockId pb_atom(const t_pb* pb_val) const;

        int atom_clb(const AtomBlockId blk_id) const;
        AtomBlockId clb_atom(const int clb_index_val) const;

        void set_atom_pb(const AtomBlockId blk_id, const t_pb* pb_val);
        void set_atom_clb(const AtomBlockId blk_id, const int clb_index);
    private:
        std::unordered_map<AtomBlockId,const t_pb*> atom_to_pb_;
        std::unordered_map<const t_pb*,AtomBlockId> pb_to_atom_;
        std::unordered_map<AtomBlockId,int> atom_to_clb_;
        std::unordered_map<int,AtomBlockId> clb_to_atom_;
};

#endif
