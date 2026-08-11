#pragma once
/**
 * Jason Luu
 * July 22, 2013
 *
 * Defines core data structures used in packing
 */
#include <vector>

#include "atom_netlist_fwd.h"
#include "physical_types.h"
#include "vpr_types.h"

/**************************************************************************
 * Packing Algorithm Enumerations
 ***************************************************************************/

/* Describes different types of intra-logic cluster_ctx.blocks routing resource nodes */
enum e_lb_rr_type {
    LB_SOURCE = 0,
    LB_SINK,
    LB_INTERMEDIATE,
    NUM_LB_RR_TYPES
};
const std::vector<const char*> lb_rr_type_str{
    "LB_SOURCE", "LB_SINK", "LB_INTERMEDIATE", "INVALID"};

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

/**************************************************************************
 * Intra-Logic Block Routing Data Structures (by type)
 ***************************************************************************/

/* Output edges of a t_lb_type_rr_node */
struct t_lb_type_rr_node_edge {
    int node_index;
    float intrinsic_cost;
};

/* Describes a routing resource node within a logic cluster_ctx.blocks type */
struct t_lb_type_rr_node {
    short capacity; /* Number of nets that can simultaneously use this node */
    int num_modes;
    short* num_fanout;      /* [0..num_modes - 1] Mode dependent fanout */
    enum e_lb_rr_type type; /* Type of logic cluster_ctx.blocks resource node */

    t_lb_type_rr_node_edge** outedges; /* [0..num_modes - 1][0..num_fanout-1] index and cost of out edges */

    t_pb_graph_pin* pb_graph_pin; /* pb_graph_pin associated with this lb_rr_node if exists, NULL otherwise */
    float intrinsic_cost;         /* cost of this node */

    t_lb_type_rr_node() noexcept {
        capacity = 0;
        num_modes = 0;
        num_fanout = nullptr;
        type = NUM_LB_RR_TYPES;
        outedges = nullptr;
        pb_graph_pin = nullptr;
        intrinsic_cost = 0;
    }
};
