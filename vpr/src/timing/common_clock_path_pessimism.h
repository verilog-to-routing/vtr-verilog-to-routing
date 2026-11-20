#pragma once
/**
 * @file
 * @brief Computes the common clock path pessimism removal (CCPP) credit of each clock domain.
 *
 * The timing analysis takes the launch and capture clock paths at opposite ends of their min/max
 * delay range. Where the two paths share the same physical clock path, a delay can't be both early
 * and late, so the analysis credits back the variation of that shared part. The credit is
 * conservative: it may miss some of the shared variation, but never credits more than is shared.
 */

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/TimingGraphFwd.hpp"

class AtomNetlist;
class AtomLookup;
class ClusteredNetlist;

/**
 * @brief Sets the common clock path pessimism credit of every clock domain in timing_constraints.
 *
 * Must be called once the clustering is known.
 *
 *   @param timing_constraints  The timing constraints to update.
 *   @param timing_graph        The timing graph.
 *   @param atom_netlist        The atom netlist.
 *   @param atom_lookup         The atom to cluster and timing graph lookup.
 *   @param clb_nlist           The clustered netlist.
 *   @param is_flat             Whether flat routing is used.
 */
void set_common_clock_path_pessimism(tatum::TimingConstraints& timing_constraints,
                                     const tatum::TimingGraph& timing_graph,
                                     const AtomNetlist& atom_netlist,
                                     const AtomLookup& atom_lookup,
                                     const ClusteredNetlist& clb_nlist,
                                     bool is_flat);
