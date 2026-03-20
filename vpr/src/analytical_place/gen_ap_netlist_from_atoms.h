#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Declaration of the gen_ap_netlist_from_atoms function which uses the
 *          results of the prepacker to generate an APNetlist.
 */

// Forward declarations
class APNetlist;
class AtomNetlist;
class Prepacker;
class RamMapper;
class UserPlaceConstraints;

/**
 * @brief Use the results from prepacking the atom netlist to generate an APNetlist.
 *
 * RAM atoms that belong to the same PhysicalRamGroup (as determined by the
 * ram_mapper) are collapsed into a single AP block containing all the
 * group's molecules, so the global placer treats them as one moveable unit.
 * Non-RAM atoms each get their own single-molecule AP block, as before.
 *
 *  @param atom_netlist          The atom netlist for the input design.
 *  @param prepacker             The prepacker, initialized on the provided atom netlist.
 *  @param ram_mapper            Used to identify physical RAM groups and create
 *                               super-blocks for them in the AP netlist.
 *  @param constraints           The placement constraints on the Atom blocks, provided
 *                               by the user.
 *  @param high_fanout_threshold The threshold above which nets with higher fanout will
 *                               be ignored.
 *
 *  @return             An APNetlist object, generated from the prepacker results.
 */
APNetlist gen_ap_netlist_from_atoms(const AtomNetlist& atom_netlist,
                                    const Prepacker& prepacker,
                                    const RamMapper& ram_mapper,
                                    const UserPlaceConstraints& constraints,
                                    int high_fanout_threshold);
