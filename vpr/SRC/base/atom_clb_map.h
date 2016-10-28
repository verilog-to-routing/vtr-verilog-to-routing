#ifndef ATOM_CLB_MAP_H
#define ATOM_CLB_MAP_H
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "vpr_types.h"

class AtomClbMap {
    public:
        int clb(const AtomBlockId blk_id) const;
        AtomBlockId atom(const int clb_val) const;
        bool is_mapped(const AtomBlockId blk_id) const;
        bool is_mapped(const int clb_val) const;

        void set_map(const AtomBlockId blk_id, const int clb_val);
        void remove(const AtomBlockId blk_id);
        void remove(const int clb_val);
    private:
        std::unordered_map<AtomBlockId,int> atom_to_clb_;
        std::unordered_map<int,AtomBlockId> clb_to_atom_;
};

#endif
