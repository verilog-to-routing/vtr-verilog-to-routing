#pragma once
/**
 * Jason Luu
 * July 22, 2013
 *
 * Defines core data structures used in packing
 */
#include <vector>

#include "atom_netlist_fwd.h"
#include "lb_type_rr_node_types.h"
#include "physical_types.h"
#include "vpr_types.h"

/**************************************************************************
 * Packing Algorithm Data Structures
 ***************************************************************************/

/**
 * @brief Running count of atoms placed in a pb's subtree during packing.
 *
 * TODO: The pin counting refactor left this struct with a single field.
 *       Worth revisiting whether t_pb_stats still earns its wrapper, but
 *       any change needs a replacement for the pb_stats null first visit
 *       marker used in try_place_atom_block_rec.
 */
struct t_pb_stats {
    /// @brief Running count of atom blocks placed in the subtree rooted at
    ///        this pb. Used during tear down to detect speculatively
    ///        allocated pbs that ended up empty, so they can be freed.
    int num_child_blocks_in_pb;
};
