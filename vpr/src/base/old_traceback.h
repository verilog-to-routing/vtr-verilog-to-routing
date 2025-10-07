#pragma once

#include "route_tree.h"

/* Contains the old traceback structure for compatibility with .route files.
 * Use route/route_tree_type.h for new code. */

/**
 * @brief Legacy element used to store the traceback (routing) of each net.
 *
 *   @param index    Array index (ID) of this routing resource node.
 *   @param net_pin_index:    Net pin index associated with the node. This value
 *                            ranges from 1 to fanout [1..num_pins-1]. For cases when
 *                            different speed paths are taken to the same SINK for
 *                            different pins, node index cannot uniquely identify
 *                            each SINK, so the net pin index guarantees an unique
 *                            identification for each SINK node. For non-SINK nodes
 *                            and for SINK nodes with no associated net pin index
 *                            (i.e. special SINKs like the source of a clock tree
 *                            which do not correspond to an actual netlist connection),
 *                            the value for this member should be set to OPEN (-1).
 *   @param iswitch  Index of the switch type used to go from this rr_node to
 *                   the next one in the routing.  UNDEFINED if there is no next node
 *                   (i.e. this node is the last one (a SINK) in a branch of the
 *                   net's routing).
 *   @param next     Pointer to the next traceback element in this route.
 */
struct t_trace {
    t_trace* next;
    int index;
    int net_pin_index = -1;
    //int net_pin_index = OPEN;
    short iswitch;
};

/* This class is a friend of RouteTree so it can build one and we don't have to clutter route tree code */
class TracebackCompat {
  public:
    static t_trace* traceback_from_route_tree(const RouteTree& tree);
    static vtr::optional<RouteTree> traceback_to_route_tree(t_trace* head);

  private:
    static void traceback_to_route_tree_x(t_trace* trace, RouteTree& tree, RouteTreeNode* parent, RRSwitchId parent_switch);
};

t_trace* alloc_trace_data();
void free_traceback(t_trace* trace);
void print_traceback(const t_trace* trace);

/**
 * @brief Validate the integrity of the traceback rooted a trace: it should contain only valid rr nodes, branches in the routing tree
 *        should link to existing routing, and edges in the traceback should exist in the RRGraph.
 *        If verify_switch_id is true, this routine also checks that the switch types used in the traceback match those in the
 *        RRGraph. If verify_switch_id is false, the switch ids (types) are remapped to those in the RRGraph. This switch remapping is
 *        useful when an RRGraph with a more detailed delay model (and hence more switch types) is used with a prior routing.
 * 
 * @param trace Pointer to the head of the routing trace of the net to validate and update.
 * @param verify_switch_id Whether to verify the switch IDs in the traceback.
 * 
 * @return true if the traceback is valid, false otherwise.
 */
bool validate_and_update_traceback(t_trace* trace, bool verify_switch_id = true);
