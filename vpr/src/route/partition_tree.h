#pragma once

#include "connection_router.h"
#include "router_stats.h"

/** Routing iteration results per thread. (for a subset of the input netlist) */
struct RouteIterResults {
    /** Are there any connections impossible to route due to a disconnected rr_graph? */
    bool is_routable = true;
    /** Net IDs for which timing_driven_route_net() actually got called */
    std::vector<ParentNetId> rerouted_nets;
    /** RouterStats collected from my subset of nets */
    RouterStats stats;
};

/** Spatial partition tree for routing.
 *
 * This divides the netlist into a tree of regions, so that nets with non-overlapping
 * bounding boxes can be routed in parallel.
 *
 * Branch nodes represent a cutline and their nets vector includes only the nets intersected
 * by the cutline. Leaf nodes represent a final set of nets reached by partitioning.
 *
 * To route this in parallel, we first route the nets in the root node, then add
 * its left and right to a task queue, and repeat this for the whole tree.
 * 
 * The tree stores some routing results to be later combined, such as is_routable and
 * rerouted_nets. (TODO: do this per thread instead of per node) */
class PartitionTreeNode {
  public:
    /** Nets claimed by this node (intersected by cutline if branch, nets in final region if leaf) */
    std::vector<ParentNetId> nets;
    /** Left subtree. */
    std::unique_ptr<PartitionTreeNode> left = nullptr;
    /** Right subtree. */
    std::unique_ptr<PartitionTreeNode> right = nullptr;
    /** Are there any connections impossible to route due to a disconnected rr_graph? */
    bool is_routable = false;
    /** Net IDs for which timing_driven_route_net() actually got called */
    std::vector<ParentNetId> rerouted_nets;

    /* debug stuff */
    int cutline_axis = -1;
    int cutline_pos = -1;
    std::vector<float> exec_times;
};

/** Holds the root PartitionTreeNode and exposes top level operations. */
class PartitionTree {
  public:
    PartitionTree() = delete;
    PartitionTree(const PartitionTree&) = delete;
    PartitionTree(PartitionTree&&) = default;
    PartitionTree& operator=(const PartitionTree&) = delete;
    PartitionTree& operator=(PartitionTree&&) = default;

    /** Can only be built from a netlist */
    PartitionTree(const Netlist<>& netlist);

    /** Access root. Shouldn't cause a segfault, because PartitionTree constructor always makes a _root */
    inline PartitionTreeNode& root(void) { return *_root; }

  private:
    std::unique_ptr<PartitionTreeNode> _root;
    std::unique_ptr<PartitionTreeNode> build_helper(const Netlist<>& netlist, const std::vector<ParentNetId>& nets, int x1, int y1, int x2, int y2);
};
