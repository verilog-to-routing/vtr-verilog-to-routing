#pragma once

/**
 * @file
 * @brief This file defines the RouteTree and RouteTreeNode, used to keep partial or full routing
 * state and timing information for a net.
 *
 * Overview
 * ========
 * A RouteTree holds a root RouteTreeNode and exposes top level operations on the tree, such as
 * RouteTree::update_from_heap() and RouteTree::prune().
 *
 * Routing itself is not done using this representation. The route tree is pushed to the heap with
 * ConnectionRouterInterface::timing_driven_route_connection_from_route_tree() and the newly found path
 * is committed via RouteTree::update_from_heap(). The timing data is updated with RouteTree::reload_timing() where required.
 *
 * Each net in the netlist given to the router has a single RouteTree, which is kept in RoutingContext::route_trees.
 *
 * Usage
 * =====
 *
 * A RouteTree either requires a RRNodeId or a ParentNetId (as the source node) to construct:
 *
 *      RouteTree tree(inet);
 *      // ...
 *
 * RouteTrees cannot be manually updated. The only way to extend them is to first route a connection and then update from the resulting heap.
 *
 *      std::tie(found_path, cheapest) = router.timing_driven_route_connection_from_route_tree(tree.root(), ...);
 *      if (found_path)
 *           std::tie(std::ignore, rt_node_of_sink) = tree.update_from_heap(&cheapest, ...);
 *
 * Congested paths in a tree can be pruned using RouteTree::prune(). This is done between iterations to keep only the legally routed section.
 * Note that updates to a tree require an update to the global occupancy state via pathfinder_update_cost_from_route_tree().
 * RouteTree::prune() depends on this global data to find congestions, so the flow to prune a tree is somewhat convoluted:
 *
 *      RouteTree tree2 = tree;
 *      // Prune the copy (using congestion data before subtraction)
 *      vtr::optional<RouteTree&> pruned_tree2 = tree2.prune(connections_inf);
 *
 *      // Subtract congestion using the non-pruned original
 *      pathfinder_update_cost_from_route_tree(tree.root(), -1);
 *
 *      if (pruned_tree2) {  // Partially pruned
 *          // Add back congestion for the pruned route tree
 *          pathfinder_update_cost_from_route_tree(pruned_tree2.value().root(), 1);
 *          ...
 *      } else {  // Fully destroyed
 *          ...
 *
 * Most usage of RouteTree outside of the router requires iterating through existing routing. Both RouteTree and RouteTreeNode exposes functions to
 * traverse the tree.
 *
 * To iterate over all nodes in the tree:
 *
 *      RouteTree& tree = route_ctx.route_trees[inet].value();
 *
 *      for (auto& node: tree.all_nodes()) {
 *          // ...
 *      }
 *
 * This will walk the tree in depth-first order. Breadth-first traversal would require recursion:
 *
 *      const RouteTreeNode& root = tree.root();
 *
 *      for (auto& child: root.child_nodes()) {
 *          // recurse...
 *      }
 *
 * To walk a node's subtree in depth-first order:
 *
 *      for (auto& node: root.all_nodes()) {  // doesn't include root!
 *          // ...   
 *      }
 *
 * RouteTree::find_by_rr_id() finds the RouteTreeNode for a given RRNodeId. Note that RRNodeId isn't a unique
 * key for SINK nodes and therefore an external lookup (generated from sink nodes returned by RouteTree::update_from_heap())
 * or a search may be required to find a certain SINK.
 *
 * When the occupancy and timing data is up to date, a tree can be sanity checked using RouteTree::is_valid().
 */

#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <unordered_map>

#include "connection_based_routing_fwd.h"
#include "route_tree_fwd.h"
#include "vtr_assert.h"
#include "spatial_route_tree_lookup.h"
#include "vtr_optional.h"
#include "vtr_range.h"
#include "vtr_vec_id_set.h"

/**
 * @brief A single route tree node
 *
 * Structure describing one node in a RouteTree. */
class RouteTreeNode {
    friend class RouteTree;

  public:
    RouteTreeNode() = delete;
    RouteTreeNode(const RouteTreeNode&) = default;
    RouteTreeNode(RouteTreeNode&&) = default;
    RouteTreeNode& operator=(const RouteTreeNode&) = default;
    RouteTreeNode& operator=(RouteTreeNode&&) = default;

    /** This struct makes little sense outside the context of a RouteTree.
     * This constructor is only public for compatibility purposes. */
    RouteTreeNode(RRNodeId inode, RRSwitchId parent_switch, RouteTreeNode* parent);

    /** ID of the rr_node that corresponds to this node. */
    RRNodeId inode;
    /** Switch type driving this node (by its parent). */
    RRSwitchId parent_switch;
    /** Should this node be put on the heap as part of the partial
     * routing to act as a source for subsequent connections? */
    bool re_expand;

  private:
    /** Is this a leaf? add_node and remove_child_if should be keeping this valid.
     * Needed to know if _next points to the first child */
    bool _is_leaf = true;

  public:
    /** Time delay for the signal to get from the net source to this node.
     * Includes the time to go through this node. */
    float Tdel;
    /** Total upstream resistance from this node to the net
     * source, including any device_ctx.rr_nodes[].R of this node. */
    float R_upstream;
    /** Total downstream capacitance from this node. That is,
     * the total C of the subtree rooted at the current node,
     * including the C of the current node. */
    float C_downstream;
    /** Net pin index associated with the node. This value
     * ranges from 1 to fanout [1..num_pins-1]. For cases when
     * different speed paths are taken to the same SINK for
     * different pins, inode cannot uniquely identify each SINK,
     * so the net pin index guarantees an unique identification
     * for each SINK node. For non-SINK nodes and for SINK
     * nodes with no associated net pin index, (i.e. special 
     * SINKs like the source of a clock tree which do not
     * correspond to an actual netlist connection), the value
     * for this member should be set to OPEN (-1). */
    int net_pin_index;

    /** Iterator implementation for child_nodes(). Walks using
     * _next_sibling ptrs. At the end of the child list, the ptr
     * points up to where the parent's subtree ends, so we know where
     * to stop */
    template<class ref>
    class RTIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = RouteTreeNode;
        using pointer = RouteTreeNode*;
        using reference = ref;

        constexpr RTIterator(RouteTreeNode* p)
            : _p(p) {}

        constexpr reference operator*() const {
            return const_cast<ref>(*_p);
        }
        inline RTIterator& operator++() {
            _p = _p->_next_sibling;
            return *this;
        }
        inline RTIterator operator++(int) {
            RTIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        constexpr bool operator==(const RTIterator& rhs) { return _p == rhs._p; }
        constexpr bool operator!=(const RTIterator& rhs) { return _p != rhs._p; }

      private:
        /** My current node */
        RouteTreeNode* _p;
    };

    /** Recursive iterator implementation for a RouteTreeNode.
     * This walks over all child nodes of a given node without a stack
     * or recursion: we keep the nodes in depth-first order in the
     * linked list managed by RouteTree. Nodes know where their subtree
     * ends, so we can just walk the _next ptrs until we find that */
    template<class ref>
    class RTRecIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = RouteTreeNode;
        using pointer = RouteTreeNode*;
        using reference = ref;

        constexpr RTRecIterator(RouteTreeNode* p)
            : _p(p) {}

        constexpr reference operator*() const {
            return const_cast<ref>(*_p);
        }
        inline RTRecIterator& operator++() {
            _p = _p->_next;
            return *this;
        }
        inline RTRecIterator operator++(int) {
            RTRecIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        constexpr bool operator==(const RTRecIterator& rhs) { return _p == rhs._p; }
        constexpr bool operator!=(const RTRecIterator& rhs) { return _p != rhs._p; }

      private:
        /** My current node */
        RouteTreeNode* _p;
    };

    /** Provide begin and end fns when iterating on this tree.
     * .child_nodes() returns Iterable<RTIterator> while .all_nodes() returns Iterable<RTRecIterator> */
    template<class Iterator>
    class Iterable {
      public:
        constexpr Iterable(RouteTreeNode* node1, RouteTreeNode* node2)
            : _node1(node1)
            , _node2(node2) {}
        constexpr Iterator begin() const { return Iterator(_node1); }
        constexpr Iterator end() const { return Iterator(_node2); }

      private:
        RouteTreeNode* _node1;
        RouteTreeNode* _node2;
    };

    /* Shorthands for different iterator types */
    using iterator = RTIterator<RouteTreeNode&>;
    using const_iterator = RTIterator<const RouteTreeNode&>;
    using rec_iterator = RTRecIterator<RouteTreeNode&>;
    using const_rec_iterator = RTRecIterator<const RouteTreeNode&>;

    /* Shorthands for "iterable" types */
    template<class ref>
    using iterable = Iterable<RTIterator<ref>>;
    template<class ref>
    using rec_iterable = Iterable<RTRecIterator<ref>>;

    /** Traverse child nodes. */
    constexpr iterable<const RouteTreeNode&> child_nodes(void) const {
        return iterable<const RouteTreeNode&>(_next, _subtree_end);
    }

    /** Get parent node if exists. (nullopt if not) */
    constexpr vtr::optional<const RouteTreeNode&> parent(void) const {
        return _parent ? vtr::optional<const RouteTreeNode&>(*_parent) : vtr::nullopt;
    }

    /** Traverse the subtree under this node in depth-first order. Doesn't include this node. */
    constexpr rec_iterable<const RouteTreeNode&> all_nodes(void) const {
        return rec_iterable<const RouteTreeNode&>(_next, _subtree_end);
    }

    /** Print information about this subtree to stdout. */
    void print(void) const;

    /** Is this node a leaf?
     *
     * True if the next node after this is not its child (we jumped up to the next branch)
     * or if it's null. The RouteTree functions keep the books for this. */
    constexpr bool is_leaf(void) const { return _is_leaf; }

    /** Equality operator. For now, just compare the addresses */
    friend bool operator==(const RouteTreeNode& lhs, const RouteTreeNode& rhs) {
        return &lhs == &rhs;
    }
    friend bool operator!=(const RouteTreeNode& lhs, const RouteTreeNode& rhs) {
        return !(lhs == rhs);
    }

  private:
    void print_x(int depth) const;

    /** Traverse child nodes, mutable reference */
    constexpr iterable<RouteTreeNode&> _child_nodes(void) const {
        return iterable<RouteTreeNode&>(_next, _subtree_end);
    }

    /** Traverse subtree, mutable reference */
    constexpr rec_iterable<RouteTreeNode&> _all_nodes(void) const {
        return rec_iterable<RouteTreeNode&>(_next, _subtree_end);
    }

    /** Ptr to parent */
    RouteTreeNode* _parent = nullptr;

    /** In the RouteTree, nodes are stored as a linked list in depth-first order.
     * This points to the next element in that list. If proper ordering is kept,
     * this also points to the first child of this node (given that it's not a leaf) */
    RouteTreeNode* _next = nullptr;

    /** Having a doubly linked list helps maintain the ordering of nodes */
    RouteTreeNode* _prev = nullptr;

    /** Here is the awkward part: these two pointers can become a union. 
     * 1. If there is a next sibling, _subtree_end is equal to it: when we finish walking the subtree we arrive at the next sibling.
     * 2. If there is not, _subtree_end is equal to the parent's _subtree_end.
     * So there is never a case where _next_sibling and _subtree_end are both valid and pointing at different things.
     * Then, the only question is where to stop when walking _next_sibling ptrs, since the final sibling won't point at nullptr.
     * But it will point at parent's _subtree_end, so we can use that as the limiting case */
    union {
        /* Ptr to "next sibling": used when traversing child nodes */
        RouteTreeNode* _next_sibling = nullptr;
        /* Where does the subtree under this node end in the flattened linked list? Needed when recursively iterating */
        RouteTreeNode* _subtree_end;
    };
};

/** fwd definition for compatibility class in old_traceback.h */
class TracebackCompat;

/**
 * @brief Top level route tree used in timing analysis and keeping routing state.
 *
 * Contains the root node and a lookup from RRNodeIds to RouteTreeNode&s in the tree. */
class RouteTree {
    friend class TracebackCompat;

  public:
    RouteTree() = delete;
    RouteTree(const RouteTree&);
    RouteTree(RouteTree&&);
    RouteTree& operator=(const RouteTree&);
    RouteTree& operator=(RouteTree&&);

    /** Return a RouteTree initialized to inode.
     * Note that prune() won't work on a RouteTree initialized this way (see _net_id comments) */
    RouteTree(RRNodeId inode);
    /** Return a RouteTree initialized to the source of nets[inet].
     * Use this constructor where possible (needed for prune() to work) */
    RouteTree(ParentNetId inet);

    ~RouteTree() {
        std::unique_lock<std::mutex> write_lock(_write_mutex);
        free_list(_root);
    }

    /** Add the most recently finished wire segment to the routing tree, and
     * update the Tdel, etc. numbers for the rest of the routing tree. hptr
     * is the heap pointer of the SINK that was reached, and target_net_pin_index
     * is the net pin index corresponding to the SINK that was reached. This routine
     * returns a tuple: RouteTreeNode of the branch it adds to the route tree and
     * RouteTreeNode of the SINK it adds to the routing.
     * Locking operation: only one thread can update_from_heap() a RouteTree at a time. */
    std::tuple<vtr::optional<const RouteTreeNode&>, vtr::optional<const RouteTreeNode&>>
    update_from_heap(t_heap* hptr, int target_net_pin_index, SpatialRouteTreeLookup* spatial_rt_lookup, bool is_flat);

    /** Reload timing values (R_upstream, C_downstream, Tdel).
     * Can take a RouteTreeNode& to do an incremental update.
     * Note that update_from_heap already does this, but prune() doesn't.
     * Locking operation: only one thread can reload_timing() for a RouteTree at a time. */
    void reload_timing(vtr::optional<RouteTreeNode&> from_node = vtr::nullopt);

    /** Get the RouteTreeNode corresponding to the RRNodeId. Returns nullopt if not found.
     * SINK nodes may be added to the tree multiple times. In that case, this will return the last one added.
     * Use find_by_isink for a more accurate lookup. */
    vtr::optional<const RouteTreeNode&> find_by_rr_id(RRNodeId rr_node) const;

    /** Get the sink RouteTreeNode associated with the isink. 
     * Will probably segfault if the tree is not constructed with a ParentNetId. */
    inline vtr::optional<const RouteTreeNode&> find_by_isink(int isink) const {
        RouteTreeNode* x = _isink_to_rt_node[isink - 1];
        return x ? vtr::optional<const RouteTreeNode&>(*x) : vtr::nullopt;
    }

    /** Get the number of sinks in associated net. */
    constexpr size_t num_sinks(void) const {
        return _num_sinks;
    }

    /** Check the consistency of this route tree. Looks for:
     * - invalid parent-child links
     * - invalid timing values
     * - congested SINKs
     * Returns true if OK. */
    bool is_valid(void) const;

    /** Check if the tree has any overused nodes (-> the tree is congested).
     * Returns true if not congested. */
    bool is_uncongested(void) const;

    /** Print information about this route tree to stdout. */
    void print(void) const;

    /** Prune overused nodes from the tree.
     * Also prune unused non-configurable nodes if non_config_node_set_usage is provided (see get_non_config_node_set_usage)
     * Returns nullopt if the entire tree is pruned.
     * Locking operation: only one thread can prune() a RouteTree at a time. */
    vtr::optional<RouteTree&> prune(CBRR& connections_inf, std::vector<int>* non_config_node_set_usage = nullptr);

    /** Remove all sinks and mark the remaining nodes as un-expandable.
     * This is used after routing a clock net.
     * TODO: is this function doing anything? Try running without it
     * Locking operation: only one thread can freeze() a RouteTree at a time. */
    void freeze(void);

    /** Count configurable edges to non-configurable node sets. (rr_nonconf_node_sets index -> int)
     * Required when using prune() to remove non-configurable nodes. */
    std::vector<int> get_non_config_node_set_usage(void) const;

    /* Wrapper for the recursive iterator. */
    using iterator = RouteTreeNode::RTRecIterator<const RouteTreeNode&>;
    using iterable = RouteTreeNode::Iterable<RouteTreeNode::const_rec_iterator>;

    /** Get an iterable for all nodes in this RouteTree. */
    constexpr iterable all_nodes(void) const { return iterable(_root, nullptr); }

    /** Get a reference to the root RouteTreeNode. */
    constexpr const RouteTreeNode& root(void) const { return *_root; } /* this file is 90% const and 10% code */

    /** Iterator implementation for remaining or reached isinks. Goes over [1..num_sinks]
     * and only returns a value when the sink state is right */
    template<bool sink_state>
    class IsinkIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = int;
        using pointer = int*;
        using reference = int&;

        constexpr IsinkIterator(const std::vector<bool>& bitset, size_t x)
            : _bitset(bitset)
            , _x(x) {
            if (_x < _bitset.size() && _bitset[_x] != sink_state) /* Iterate forward to a valid state */
                ++(*this);
        }
        constexpr value_type operator*() const {
            return _x;
        }
        inline IsinkIterator& operator++() {
            _x++;
            for (; _x < _bitset.size() && _bitset[_x] != sink_state; _x++)
                ;
            return *this;
        }
        inline IsinkIterator operator++(int) {
            IsinkIterator tmp = *this;
            ++(*this);
            return tmp;
        }
        constexpr bool operator==(const IsinkIterator& rhs) { return _x == rhs._x; }
        constexpr bool operator!=(const IsinkIterator& rhs) { return _x != rhs._x; }

      private:
        /** Ref to the bitset */
        const std::vector<bool>& _bitset;
        /** Current position */
        size_t _x;
    };

    typedef vtr::Range<IsinkIterator<true>> reached_isink_range;
    typedef vtr::Range<IsinkIterator<false>> remaining_isink_range;

    /** Get a lookup which contains the "isink reached state".
     * It's a 1-indexed! bitset of "pin indices". True if the nth sink has been reached, false otherwise.
     * If you call it before prune() and after routing, there's no guarantee on whether the reached sinks
     * are reached legally. */
    constexpr const std::vector<bool>& get_is_isink_reached(void) const { return _is_isink_reached; }

    /** Get reached isinks: 1-indexed pin indices enumerating the sinks in this net.
     * "Reached" means "reached legally" if you call this after prune() and not before any routing.
     * Otherwise it doesn't guarantee legality.
     * Builds and returns a value: use get_is_isink_reached directly if you want speed. */
    constexpr reached_isink_range get_reached_isinks(void) const {
        return vtr::make_range(IsinkIterator<true>(_is_isink_reached, 1), IsinkIterator<true>(_is_isink_reached, _num_sinks + 1));
    }

    /** Get remaining (not routed (legally?)) isinks:
     * 1-indexed pin indices enumerating the sinks in this net.
     * Caveats in get_reached_isinks() apply. */
    constexpr remaining_isink_range get_remaining_isinks(void) const {
        return vtr::make_range(IsinkIterator<false>(_is_isink_reached, 1), IsinkIterator<false>(_is_isink_reached, _num_sinks + 1));
    }

  private:
    std::tuple<vtr::optional<RouteTreeNode&>, vtr::optional<RouteTreeNode&>>
    add_subtree_from_heap(t_heap* hptr, int target_net_pin_index, bool is_flat);

    void add_non_configurable_nodes(RouteTreeNode* rt_node,
                                    bool reached_by_non_configurable_edge,
                                    std::unordered_set<RRNodeId>& visited,
                                    bool is_flat);

    void reload_timing_unlocked(vtr::optional<RouteTreeNode&> from_node = vtr::nullopt);
    void load_new_subtree_R_upstream(RouteTreeNode& from_node);
    float load_new_subtree_C_downstream(RouteTreeNode& from_node);
    RouteTreeNode& update_unbuffered_ancestors_C_downstream(RouteTreeNode& from_node);
    void load_route_tree_Tdel(RouteTreeNode& from_node, float Tarrival);

    bool is_valid_x(const RouteTreeNode& rt_node) const;
    bool is_uncongested_x(const RouteTreeNode& rt_node) const;

    vtr::optional<RouteTreeNode&>
    prune_x(RouteTreeNode& rt_node,
            CBRR& connections_inf,
            bool force_prune,
            std::vector<int>* non_config_node_set_usage);

    void freeze_x(RouteTreeNode& rt_node);

    /** Add node to parent, set up prev/next links, set up parent-child links and update lookup. */
    inline void add_node(RouteTreeNode* parent, RouteTreeNode* node) {
        node->_prev = parent;
        node->_next = parent->_next;
        if (parent->_next)
            parent->_next->_prev = node;

        node->_parent = parent;
        /* If parent is a leaf, its _next ptr isn't a child node. Update _subtree_end
         * Otherwise, update the sibling ptr, since a sibling exists, it's the end of our subtree.
         * These two pointers are a union, see RouteTreeNode definition for details */
        if (parent->is_leaf()) {
            node->_subtree_end = parent->_subtree_end;
        } else {
            node->_next_sibling = parent->_next;
        }
        parent->_next = node;

        /** Add node to RR to RT lookup */
        _rr_node_to_rt_node[node->inode] = node;
        /** If node is a SINK (net_pin_index > 0), also add it to sink RT lookup */
        if (node->net_pin_index > 0 && _net_id.is_valid())
            _isink_to_rt_node[node->net_pin_index - 1] = node;

        /* Now it's a branch */
        parent->_is_leaf = false;
    }

    /** Free all nodes after node */
    inline void free_list(RouteTreeNode* node) {
        RouteTreeNode* p = node;
        while (p) {
            RouteTreeNode* x = p;
            p = p->_next;
            free_node(x);
        }
    }

    /** Make a copy of rhs and return it. Updates _rr_node_to_rt_node */
    RouteTreeNode* copy_tree(const RouteTreeNode* rhs);

    /** Helper for copy_list: copy child nodes of rhs into lhs */
    void copy_tree_x(RouteTreeNode* lhs, const RouteTreeNode& rhs);

    /** Free a node. Only keeps the linked list valid (not the tree ptrs) */
    inline void free_node(RouteTreeNode* node) {
        if (node->_prev)
            node->_prev->_next = node->_next;
        if (node->_next)
            node->_next->_prev = node->_prev;
        delete node;
    }

    /** Iterate through parent's child nodes and remove if p returns true.
     * Updates tree ptrs and _rr_node_to_rt_node. */
    inline void remove_child_if(RouteTreeNode& parent, const std::function<bool(RouteTreeNode&)>& p) {
        RouteTreeNode* curr = parent._next;
        RouteTreeNode* prev = nullptr;
        RouteTreeNode* end = parent._subtree_end;
        while (curr != end) {
            if (p(*curr)) {
                _rr_node_to_rt_node.erase(curr->inode);
                /* Here is a tricky part: when updating prev's _next_sibling, we need to also
                 * update the _subtree_end/_next_sibling ptrs of prev's subtree. We know that
                 * _next_sibling points to _subtree_end in the last ("rightmost") node of every
                 * level. _subtree_end's _prev will take us to the rightmost leaf in the subtree,
                 * and from there we can walk up via _parent ptrs and update along the way */
                if (prev) {
                    RouteTreeNode* q = prev->_subtree_end->_prev;
                    while (q != prev) {
                        q->_subtree_end = curr->_subtree_end;
                        q = q->_parent;
                    }
                    prev->_next_sibling = curr->_next_sibling;
                }
                RouteTreeNode* x = curr;
                curr = curr->_next_sibling;
                free_node(x);
            } else {
                prev = curr;
                curr = curr->_next_sibling;
            }
        }
        /* did this node become a leaf? */
        if (parent._next == nullptr || parent._next->_parent != &parent)
            parent._is_leaf = true;
    }

    /** Root node.
     * This is also the internal node list via the ptrs in RouteTreeNode. */
    RouteTreeNode* _root;

    /** Net ID.
     * A RouteTree does not have to be connected to a net, but if it isn't
     * constructed using a ParentNetId prune() won't work. This is due to
     * a data dependency through "Connection_based_routing_resources". Should
     * be refactored when possible. */
    ParentNetId _net_id;

    /** Lookup from RRNodeIds to RouteTreeNodes in the tree.
     * In some cases the same SINK node is put into the tree multiple times in a
     * single route. To model this, we are putting in separate rt_nodes in the route
     * tree if we go to the same SINK more than once. rr_node_to_rt_node[inode] will
     * therefore store the last rt_node created of all the SINK nodes with the same
     * index "inode". */
    std::unordered_map<RRNodeId, RouteTreeNode*> _rr_node_to_rt_node;

    /** RRNodeId is not a unique lookup for sink RouteTreeNodes, but net_pin_index
     * is. Store a 0-indexed lookup here for users who need to look up a sink from
     * a net_pin_index, ipin, isink, etc. */
    std::vector<RouteTreeNode*> _isink_to_rt_node;

    /** Is Nth sink in this net reached?
     * Bitset of [1..num_sinks]. (1-indexed!)
     * We work with these indices, because they are used in a bunch of lookups in
     * the router. Looking these back up from sink RR nodes would require looking
     * up its RouteTreeNode and then the net_pin_index from that. */
    std::vector<bool> _is_isink_reached;

    /** Number of sinks in this tree's net. Useful for iteration. */
    size_t _num_sinks;

    /** Write mutex on this RouteTree. Acquired by the write operations automatically:
     * the caller does not need to know about a lock. */
    std::mutex _write_mutex;
};
