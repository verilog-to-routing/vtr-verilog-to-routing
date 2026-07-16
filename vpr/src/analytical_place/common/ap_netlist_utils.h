#pragma once
/**
 * @file
 * @author  Kevin Xing
 * @date    July 2026
 * @brief   Declaration of a utility class used to perform atom block id to AP block id lookup.
 */

#include "atom_netlist_fwd.h"
#include "ap_netlist_fwd.h"
#include "vtr_vector.h"

// Forward declaration
class Prepacker;

/**
 * @brief A lookup used to map an atom block id to the AP block id that contains it.
 * 
 * The class constructs and owns a vtr::vector storing (AtomBlockId, APBlockId) pairs by iterating through the APNetlist.
 * This vector is where the lookup essentially happens. After construction, it is required to call the verify() method
 * to ensure consistency between the lookup and the APNetlist. Otherwise, use of the getter get_ap_block()
 * will be rejected by a VTR_ASSERT (unless a lower assert level is set).
 */
class AtomBlockAPBlockLookup {
  public:
    AtomBlockAPBlockLookup() = delete;

    /**
     * @brief Constructs the lookup by going through the molecules contained by each AP block in the APNetlist.
     * 
     * @param atom_netlist AtomNetlist used to precisely resize the lookup vector to the number of atom blocks.
     * @param ap_netlist APNetlist used to go through all AP blocks.
     * @param prepacker Prepacker used to get the molecules in an AP block so that we can process the grouped atom blocks.
     */
    AtomBlockAPBlockLookup(const AtomNetlist& atom_netlist, const APNetlist& ap_netlist, const Prepacker& prepacker);

    /**
     * @brief Verifies that the lookup is consistent with the AP netlist.
     *
     * This method checks that every mapped atom block is still present in the expected AP block
     * by actually going into the APNetlist and insepecting the associated AP blocks.
     * The "verified_" flag is set to true upon a successful return.
     *
     * @param ap_netlist APNetlist used to go through all AP blocks.
     * @param prepacker Prepacker used to get the molecules in an AP block so that we can directly look for the expected atom block.
     */
    void verify(const APNetlist& ap_netlist, const Prepacker& prepacker);

    /**
     * @brief Returns the AP block that contains the given atom block.
     *
     * @param atom_block_id Atom block to look up.
     */
    APBlockId get_ap_block(const AtomBlockId atom_block_id) const;

  private:
    /**
     * @brief Reflects whether the lookup has been verified after construction.
     */
    bool verified_ = false;

    /**
     * @brief Maps each atom block ID to the AP block ID that contains it.
     */
    vtr::vector<AtomBlockId, APBlockId> atom_block_ap_block_;
};
