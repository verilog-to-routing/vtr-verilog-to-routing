#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Declaration of the gen_ap_netlist_from_atoms function which uses the
 *          results of the prepacker to generate an APNetlist.
 */

#include <vector>
#include "constant_nets.h"

// Forward declarations
class APNetlist;
class AtomNetlist;
class Prepacker;
class RamMapper;
class UserPlaceConstraints;
struct t_logical_block_type;

/**
 * @brief Use the results from prepacking the atom netlist to generate an APNetlist.
 *
 * RAM atoms that belong to the same PhysicalRamGroup (as determined by the
 * ram_mapper) are collapsed into a single AP block containing all the
 * group's molecules, so the global placer treats them as one moveable unit.
 *
 *  @param atom_netlist          The atom netlist for the input design.
 *  @param prepacker             The prepacker, initialized on the provided atom netlist.
 *  @param ram_mapper            Used to identify physical RAM groups and create
 *                               multiple molecule blocks for them in the AP netlist.
 *  @param constraints           The placement constraints on the Atom blocks, provided
 *                               by the user.
 *  @param logical_block_types   The logical block types of the architecture. Used to
 *                               find which nets will connect to global tile pins.
 *  @param high_fanout_threshold The threshold above which nets with higher fanout will
 *                               be ignored.
 *  @param constant_net_method   How constant nets (e.g. gnd / vcc) will be handled. If
 *                               these nets will not be routed, they will be ignored.
 *
 *  @return             An APNetlist object, generated from the prepacker results.
 */
APNetlist gen_ap_netlist_from_atoms(const AtomNetlist& atom_netlist,
                                    const Prepacker& prepacker,
                                    const RamMapper& ram_mapper,
                                    const UserPlaceConstraints& constraints,
                                    const std::vector<t_logical_block_type>& logical_block_types,
                                    int high_fanout_threshold,
                                    e_constant_net_method constant_net_method);
