/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Declaration of the gen_ap_netlist_from_atoms function which uses the
 *          results of the prepacker to generate an APNetlist.
 */

#pragma once

// Forward declarations
class APNetlist;
class AtomNetlist;
class Prepacker;
class UserPlaceConstraints;

/**
 * @brief Use the results from prepacking the atom netlist to generate an APNetlist.
 *
 *  @param atom_netlist The atom netlist for the input design.
 *  @param prepacker    The prepacker, initialized on the provided atom netlist.
 *  @param constraints  The placement constraints on the Atom blocks, provided
 *                      by the user.
 *
 *  @return             An APNetlist object, generated from the prepacker results.
 */
APNetlist gen_ap_netlist_from_atoms(const AtomNetlist& atom_netlist,
                                    const Prepacker& prepacker,
                                    const UserPlaceConstraints& constraints);

