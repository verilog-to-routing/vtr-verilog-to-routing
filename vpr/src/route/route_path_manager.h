#pragma once

#include "rr_graph_fwd.h"

#include <set>
#include <vector>

/* Extra path data needed by RCV, separated from RTExploredNode struct for performance reasons
 * Can be accessed by a pointer, won't be initialized unless by RCV
 * Use PathManager class to handle this structure's allocation and deallocation
 *
 * path_rr: The entire partial path up until the route tree with the first node being the SOURCE,
 *          or a part of the route tree that already exists for this net
 *
 * edge: A list of edges from each node in the partial path to reach the next node
 *
 * backward_delay: The delay of the partial path plus the path from route tree to source
 *
 * backward_cong: The congestion estimate of the partial path plus the path from route tree to source */
struct t_heap_path {
    std::vector<RRNodeId> path_rr;
    std::vector<RREdgeId> edge;
    float backward_delay = 0.;
    float backward_cong = 0.;
};

// Forward declaration of RoutingContext needed for traceback insertion
struct RoutingContext;
class RouteTreeNode;

/* A class to manage the extra data required for RCV
 * It manages a set containing all the nodes that currently exist in the route tree
 * This class also manages the extra memory allocation required for the t_heap_path structure
 *
 * When RCV is enabled, the router will not always be looking for minimal cost routing
 * This means nodes that already exist in the current path, or current route tree could be expanded twice.
 * This would result in electrically illegal loops (example below)
 *
 * OPIN--|----|             |-----------Sink 1
 *       |    |--------X----|     <--- The branch intersects with a previous routing
 *       |             |
 *       |-------------|                              Sink 2
 *
 * To stop this, we keep track of the route tree (route_tree_nodes_), and each node keeps track of it's current partial routing up to the route tree
 * Before expanding a node, we check to see if it exists in either the route tree, or the current partial path to eliminate these scenarios
 *
 * The data managed here has two lifetimes:
 *  - Net-level: route_tree_nodes_ holds every RR node in the net's route tree. Within a net the route tree only
 *    grows, so the set is built once (by the first connection of the net that uses RCV) and then extended with
 *    each newly routed branch. It is maintained whether or not RCV is enabled for the current connection, since
 *    RCV is toggled per connection. It is cleared at the start of each net.
 *  - Connection-level: the t_heap_path structures, which are torn down after every path search (empty_heap()).
 *
 *
 * The t_heap_path structure was created to isolate the RCV specific data from the RTExploredNode struct
 * Having these in RTExploredNode creates significant performance issues when RCV is disabled
 * A t_heap_path pointer is instead stored in RTExploredNode, which is selectively allocated only when RCV is enabled
 *
 * If the _is_enabled flag is true, alloc_path_struct allocates t_heap_path structures, otherwise will be a NOOP */
class PathManager {
  public:
    PathManager();
    ~PathManager();

    // Checks if the target node exists in the route tree, or current backwards path variable
    // This is needed for RCV as the non minimum distance pathfinding can lead to illegal loops
    // By keeping a set of the current route tree for a net, as well as checking the current path we can prevent this
    bool node_exists_in_tree(t_heap_path* path_data,
                             RRNodeId to_node);

    // Check if this structure is enabled/is RCV enabled
    bool is_enabled();

    // Enable/disable path manager class and therefore RCV
    void set_enabled(bool enable);

    // Insert the partial path data into the main route context traceback
    void insert_backwards_path_into_traceback(t_heap_path* path_data, float cost, float backward_path_cost, RoutingContext& route_ctx);

    // Dynamically create a t_heap_path structure to be used in the heap
    // Will return unless RCV is enabled
    void alloc_path_struct(t_heap_path*& tptr);

    // Free the path structure pointer if it's initialized
    // This will place it on the freed_nodes vector to be reused
    void free_path_struct(t_heap_path*& tptr);

    // Move the structure from src to dest while also invalidating the src pointer
    void move(t_heap_path*& dest, t_heap_path*& src);

    // Cleanup and free all the allocated memory, called when PathManager is destroyed
    void free_all_memory();

    // Put all currently allocated structures into the free_nodes list
    // This currently does NOT invalidate them
    void empty_heap();

    /* Net-level route tree set. Unlike the functions above, these are not gated on
     * is_enabled(), because RCV is enabled per connection but the set must describe
     * the whole route tree, including branches routed with RCV disabled. */

    // Has the route tree set been built for the net currently being routed?
    bool route_tree_set_valid() const;

    // Build the route tree set from the whole route tree rooted at rt_root.
    // Called once per net, by the first connection that uses RCV
    void build_route_tree_set(const RouteTreeNode& rt_root);

    // Add a newly routed branch (new_branch and all nodes below it) to the route tree set.
    // Call this every time a branch is added to the route tree. No-op until the set is built for this net
    void add_branch_to_route_tree_set(const RouteTreeNode& new_branch);

    // Clear and invalidate the route tree set. Call at the start of each net
    void clear_route_tree_set();

  private:
    // Is RCV enabled for the current connection (gates the per-connection t_heap_path data)
    bool is_enabled_;

    // This is a list of all currently allocated heap path pointers
    std::vector<t_heap_path*> alloc_list_;

    // A list of freed nodes, to be used where possible to avoid unnecessary news
    std::vector<t_heap_path*> freed_nodes_;

    // Set containing the current route tree, for faster lookup
    // Required by RCV so the router doesn't expand already visited nodes
    // Lives for the whole net (see class comment)
    std::set<RRNodeId> route_tree_nodes_;

    // Whether route_tree_nodes_ has been built for the net currently being routed
    bool route_tree_set_valid_ = false;
};
