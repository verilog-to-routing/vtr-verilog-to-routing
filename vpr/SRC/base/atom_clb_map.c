#include "vtr_assert.h"

#include "atom_clb_map.h"

int AtomClbMap::clb(const AtomBlockId blk_id) const {
    auto iter = atom_to_clb_.find(blk_id);
    if(iter == atom_to_clb_.end()) {
        //Not found
        VTR_ASSERT(false);
        return -1;
    }
    return iter->second;
}

AtomBlockId AtomClbMap::atom(const int clb_val) const {
    auto iter = clb_to_atom_.find(clb_val);
    if(iter == clb_to_atom_.end()) {
        //Not found
        return AtomBlockId::INVALID();
    }
    return iter->second;
}

bool AtomClbMap::is_mapped(const AtomBlockId blk_id) const {
    return atom_to_clb_.find(blk_id) != atom_to_clb_.end();
}

bool AtomClbMap::is_mapped(const int clb_val) const {
    return clb_to_atom_.find(clb_val) != clb_to_atom_.end();
}

void AtomClbMap::set_map(const AtomBlockId blk_id, const int clb_val) {
    atom_to_clb_[blk_id] = clb_val;
    clb_to_atom_[clb_val] = blk_id;
}

void AtomClbMap::remove(const AtomBlockId blk_id) {
    auto iter_forward = atom_to_clb_.find(blk_id);
    if(iter_forward != atom_to_clb_.end()) {
        auto iter_inverse = clb_to_atom_.find(iter_forward->second);
        VTR_ASSERT(iter_inverse != clb_to_atom_.end());
        clb_to_atom_.erase(iter_inverse);
        atom_to_clb_.erase(iter_forward);
    }
}

void AtomClbMap::remove(const int clb_val) {
    auto iter_inverse = clb_to_atom_.find(clb_val);
    if(iter_inverse != clb_to_atom_.end()) {
        auto iter_forward = atom_to_clb_.find(iter_inverse->second);
        VTR_ASSERT(iter_forward != atom_to_clb_.end());
        atom_to_clb_.erase(iter_forward);
        clb_to_atom_.erase(iter_inverse);
    }
}
