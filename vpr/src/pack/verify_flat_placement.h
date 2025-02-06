/**
 * @file
 * @author  Alex Singer
 * @date    January 2025
 * @brief   Independent verify methods to check invariants on the flat
 *          placement that has been passed into the packer. This checks for
 *          invalid data so this does not have to be checked during packing.
 */

#pragma once

// Forward declarations
class FlatPlacementInfo;
class AtomNetlist;
class Prepacker;

/**
 * @brief Verify the flat placement for use in the packer.
 *
 * This method will check the following invariants:
 *  1. Every atom has a defined x and y position.
 *  2. Every atom has non-negative placement information values.
 *  3. Every molecule has atoms that have the same placement information.
 *
 * This method will log error messages for each issue it finds and will return
 * a count of the number of errors.
 *
 *  @param flat_placement_info
 *              The flat placement to verify.
 *  @param atom_netlist
 *              The netlist of atoms in the circuits.
 *  @param prepacker
 *              The prepacker object used to prepack the atoms into molecules.
 */
unsigned verify_flat_placement_for_packing(const FlatPlacementInfo& flat_placement_info,
                                           const AtomNetlist& atom_netlist,
                                           const Prepacker& prepacker);

