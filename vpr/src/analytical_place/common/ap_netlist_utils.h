#pragma once

#include "atom_netlist_fwd.h"
#include "ap_netlist_fwd.h"
#include "vtr_vector.h"

class Prepacker;


class AtomBlockAPBlockLookup {
    public:
    AtomBlockAPBlockLookup() = delete;

    AtomBlockAPBlockLookup(const AtomNetlist& atom_netlist, const Prepacker& prepacker, const APNetlist& ap_netlist);

    void verify(const Prepacker& prepacker, const APNetlist& ap_netlist);

    /**
     * @brief Returns the AP block that contains the given atom block.
     *
     * @param atom_block_id Atom block to look up.
     */
    APBlockId get_ap_block(const AtomBlockId atom_block_id) const;

    private:
    bool verified_ = false;
    vtr::vector<AtomBlockId, APBlockId> atom_block_ap_block_;
};
