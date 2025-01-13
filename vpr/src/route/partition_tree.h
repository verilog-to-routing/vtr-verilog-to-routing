#pragma once

#include "connection_router.h"
#include "netlist_fwd.h"
#include "router_stats.h"

#include <cmath>
#include <fstream>
#include <memory>
#include <thread>

#ifdef VPR_USE_TBB
#    include <tbb/concurrent_vector.h>
#endif

/** Self-descriptive */
enum class Axis { X,
                  Y };

/** Which side of a line? */
enum class Side { LEFT = 0,
                  RIGHT = 1 };

/** Invert side */
inline Side operator!(const Side& rhs) {
    return Side(!size_t(rhs));
}

/** Part of a net in the context of the \ref DecompNetlistRouter. Sinks and routing resources
 * routable/usable by the \ref ConnectionRouter are constrained to ones inside clipped_bb
 * (\see inside_bb()) */
class VirtualNet {
  public:
    /** The net in question (ID into a \ref Netlist). */
    ParentNetId net_id;
    /** The bounding box created by clipping the parent's bbox against a cutline. */
    t_bb clipped_bb;
    /** Times decomposed -- don't decompose vnets too deeply or it disturbs net ordering
     * when it's eventually disabled --> makes routing more difficult.
     * 1 means this vnet was just created by dividing a regular net */
    int times_decomposed = 0;
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
 * its left and right to a task queue, and repeat this for the whole tree. */
class PartitionTreeNode {
  public:
    /** Nets claimed by this node (intersected by cutline if branch, nets in final region if leaf) */
    std::unordered_set<ParentNetId> nets;
    /** Virtual nets assigned by the parent of this node (\see DecompNetlistRouter) */
    std::vector<VirtualNet> vnets;
    /** Left subtree. */
    std::unique_ptr<PartitionTreeNode> left = nullptr;
    /** Right subtree. */
    std::unique_ptr<PartitionTreeNode> right = nullptr;
    /** Parent node. */
    PartitionTreeNode* parent = nullptr;
    /* Axis of the cutline. */
    Axis cutline_axis = Axis::X;
    /* Position of the cutline. It's a float, because cutlines are considered to be "between" integral coordinates. */
    float cutline_pos = std::numeric_limits<float>::quiet_NaN();
    /* Bounding box of *this* node. (The cutline cuts this box) */
    t_bb bb;
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

    /** Handle nets which had a bounding box update.
    * Bounding boxes can only grow, so we should find a new partition tree node for
    * these nets by moving them up until they fit in a node's bounds */
    void update_nets(const std::vector<ParentNetId>& nets);
  
    /** Delete all virtual nets in the tree. Used for the net decomposing router.
     * Virtual nets are invalidated between iterations due to changing bounding
     * boxes. */
    void clear_vnets(void);

  private:
    std::unique_ptr<PartitionTreeNode> _root;
    std::unordered_map<ParentNetId, PartitionTreeNode*> _net_to_ptree_node;
    std::unique_ptr<PartitionTreeNode> build_helper(const Netlist<>& netlist, const std::unordered_set<ParentNetId>& nets, int x1, int y1, int x2, int y2);
};

#ifdef VPR_DEBUG_PARTITION_TREE
/** Log PartitionTree-related messages. Can handle multiple threads. */
class PartitionTreeDebug {
  public:
    static inline tbb::concurrent_vector<std::string> lines;
    /** Add msg to the log buffer (with a thread ID header) */
    static inline void log(std::string msg) {
        auto thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
        lines.push_back("[thread " + std::to_string(thread_id) + "] " + msg);
    }
    /** Write out the log buffer into a file */
    static inline void write(std::string filename) {
        std::ofstream f(filename);
        for (auto& line : lines) {
            f << line << std::endl;
        }
        f.close();
    }
};
#else
class PartitionTreeDebug {
  public:
    static inline void log(std::string /* msg */) {}
    static inline void write(std::string /* filename */) {}
};
#endif
