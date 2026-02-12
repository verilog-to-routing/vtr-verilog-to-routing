#pragma once
/*
 * Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
 *
 * Author: Jason Luu
 * Date: July 22, 2013
 *
 * Updated by Alex Singer, February 2026
 */

#include <cstdio>
#include <queue>
#include <unordered_map>
#include <vector>
#include "atom_netlist_fwd.h"
#include "atom_pb_bimap.h"
#include "pack_types.h"
#include "vpr_types.h"
#include "vpr_utils.h"

/**************************************************************************
 * Intra-Logic Block Router Data Structures
 ***************************************************************************/

/**
 * @brief Describes the status of a logic cluster_ctx.blocks routing resource
 *        node for a given logic cluster_ctx.blocks instance.
 */
struct t_lb_rr_node_stats {
    /// @brief The number of nets currently using this lb_rr_node.
    int occ;

    /// @brief The mode that this rr_node is set to.
    int mode;

    /// @brief The historical usage of using this node.
    int historical_usage;

    t_lb_rr_node_stats() noexcept {
        occ = 0;
        mode = -1;
        historical_usage = 0;
    }
};

/**
 * @brief Data structure forming the route tree of a net within one logic
 *        cluster_ctx.blocks.
 *
 * A net is implemented using routing resource nodes. The t_lb_trace data
 * structure records one of the nodes used by the net and the connections
 * to other nodes.
 */
struct t_lb_trace {
    /// @brief The current t_lb_type_rr_node used by net.
    int current_node;

    /// @brief The index of previous edge that drives current node.
    std::vector<t_lb_trace> next_nodes;
};

/**
 * @brief Represents a net used inside a logic cluster_ctx.blocks and the
 *        physical nodes used by the net.
 */
struct t_intra_lb_net {
    /// @brief The index of atom net this intra_lb_net represents.
    AtomNetId atom_net_id;

    /// @brief The endpoints of the intra_lb_net, 0th position is the source, all others are sinks.
    std::vector<int> terminals;

    /// @brief The AtomPins associated with each terminal.
    std::vector<AtomPinId> atom_pins;

    /// @brief Marks a terminal as having a fixed target (i.e. a pin not a sink).
    std::vector<bool> fixed_terminals;

    /// @brief Route tree head.
    t_lb_trace rt_tree;

    t_intra_lb_net() noexcept {
        atom_net_id = AtomNetId::INVALID();
        rt_tree.current_node = UNDEFINED;
    }
};

/**
 * @brief Tuning parameters used by intra-logic cluster_ctx.blocks router.
 */
struct t_lb_router_params {
    int max_iterations;
    float pres_fac;
    float pres_fac_mult;
    float hist_fac;
};

/**
 * @brief Node expanded by router.
 */
struct t_expansion_node {
    /// @brief Index of logic cluster_ctx.blocks rr node this expansion node represents.
    int node_index;

    /// @brief Index of logic cluster_ctx.blocks rr node that drives this expansion node.
    int prev_index;

    float cost;

    t_expansion_node() {
        node_index = UNDEFINED;
        prev_index = UNDEFINED;
        cost = 0;
    }
};

/**
 * @brief Comparitor used by the priority queue to sort the expansion nodes.
 */
class compare_expansion_node {
  public:
    /**
     * @brief Returns true if e1 comes before e2.
     *
     * Note that because a priority queue outputs the largest element first, the
     * element that "comes before" are actually output last.
     *
     * TL;DR lowest cost is popped first.
     */
    bool operator()(t_expansion_node& e1, t_expansion_node& e2)
    {
        if (e1.cost > e2.cost) {
            return true;
        }
        return false;
    }
};

/**
 * @brief Explored node by router (trace-back information).
 */
struct t_explored_node_tb {
    /// @brief Previous node that drives this one.
    int prev_index;

    /// @brief ID used to determine if this node has been explored.
    int explored_id;

    /// @brief Net index of route tree.
    int inet;

    /// @brief ID used to determine if this node has been pushed on exploration priority queue.
    int enqueue_id;

    /// @brief Cost of node pushed on exploration priority queue.
    float enqueue_cost;

    t_explored_node_tb() noexcept {
        prev_index = UNDEFINED;
        explored_id = UNDEFINED;
        enqueue_id = UNDEFINED;
        inet = UNDEFINED;
        enqueue_cost = 0;
    }
};

/**
 * @brief Stores status of mode selection during clustering.
 */
struct t_mode_selection_status {
    bool is_mode_conflict = false;
    bool try_expand_all_modes = false;
    bool expand_all_modes = false;

    inline bool is_mode_issue() const {
        return is_mode_conflict || try_expand_all_modes;
    }
};

// TODO: check if this hacky class memory reserve thing is still necessary, if not, then delete
/* Packing uses a priority queue that requires a large number of elements.  This backdoor
 * allows me to use a priority queue where I can pre-allocate the # of elements in the underlying container
 * for efficiency reasons.  Note: Must use vector with this */
template<class T, class U, class V>
class reservable_pq : public std::priority_queue<T, U, V> {
  public:
    typedef typename std::priority_queue<T>::size_type size_type;
    reservable_pq(size_type capacity = 0) {
        reserve(capacity);
        cur_cap = capacity;
    }
    void reserve(size_type capacity) {
        this->c.reserve(capacity);
        cur_cap = capacity;
    }
    void clear() {
        this->c.clear();
        this->c.reserve(cur_cap);
    }

  private:
    size_type cur_cap;
};

/// @brief The priority queue type used in the Cluster Router.
typedef reservable_pq<t_expansion_node, std::vector<t_expansion_node>, compare_expansion_node> ClusterRouterPriorityQueue;

/**
 * @brief Enumeration used by the commit_remove_rt_ method below to decide if
 *        we are commiting or removing a route tree.
 */
enum e_commit_remove { RT_COMMIT,
                       RT_REMOVE };

/**
 * @brief Class for routing the connections within a cluster.
 *
 * This class manages all of the state needed to perform pathfinder within a
 * cluster during clustering.
 *
 * Example usage:
 *      // Construct the router object.
 *      ClusterRouter cluster_router(&lb_type_rr_graphs[cluster_type->index], cluster_type);
 *
 *      // Add atoms as targets for the router.
 *      cluster_router.add_atom_as_target(blk_id1, atom_to_pb);
 *      cluster_router.add_atom_as_target(blk_id2, atom_to_pb);
 *      ...
 *
 *      // (optional) Remove atoms from target if necessary
 *      cluster_router.remove_atom_from_target(blk_id1, atom_to_pb);
 *      ...
 *
 *      // Perform a cluster route. success would be true if route is successful.
 *      t_mode_selection_status mode_status;
 *      bool success = cluster_router.try_intra_lb_route(log_verbosity_, &mode_status);
 *
 *      // Output the routing solution.
 *      auto pb_route = cluster_router.alloc_and_load_pb_route(intra_lb_pb_pin_lookup_);
 *
 *      // (optional) Clean the router data to save space if this object will
 *      //            have a long lifetime and no further routes will be performed.
 *      cluster_router.clean_router_data();
 *
 */
class ClusterRouter {
  public:
    /**
     * @brief Default constructor for the ClusterRouter.
     *
     * This is needed to allow this object to be stored within the vtr::vector_map
     * class. When this constructor is used, the cluster router will not be
     * initialized properly and cannot be used.
     */
    ClusterRouter()
        : lb_type_graph_(nullptr)
        , explore_id_index_(0)
        , lb_type_(nullptr)
        , pres_con_fac_(0.0f)
        , is_clean_(true)
        , is_valid_(false) {}

    /**
     * @brief Constructor for the ClusterRouter.
     *
     * Sets up the ClusterRouter for the given logical block type.
     *
     *  @param lb_type_graph    The RR graph for the given lb type.
     *  @param type             The logical block type to route to.
     */
    ClusterRouter(std::vector<t_lb_type_rr_node>* lb_type_graph,
                  t_logical_block_type_ptr type);

    /**
     * @brief Add pins of netlist atom to current routing drivers/targets.
     */
    void add_atom_as_target(const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb);

    /**
     * @brief Remove pins of netlist atom from current routing drivers/targets.
     */
    void remove_atom_from_target(const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb);

    /**
     * @brief Set/reset mode of rr nodes to the pb used.
     *
     * If set == true, then set all modes of the rr nodes affected by pb to the
     * mode of the pb. Set all modes related to pb to 0 otherwise.
     */
    void set_reset_pb_modes(const t_pb* pb, const bool set);

    /**
     * @brief Attempt to route routing driver/targets on the current architecture.
     *
     * Follows pathfinder negotiated congestion algorithm.
     */
    bool try_intra_lb_route(int verbosity, t_mode_selection_status* mode_status);

    /**
     * @brief Clean and free all data within the router.
     *
     * This is used in the packer to keep the cluster router object around
     * without a hit to memory footprint. The router can no longer be used after
     * this function is called.
     */
    void clean_router_data();

    /**
     * @brief Reset the routing state between calls to intra_lb_route for mode
     *        changes.
     *
     * This is used to clean state associated with mode conflicts in routing.
     * This state is stored within the pb_graph_pins.
     *
     * TODO: It may be safer to store this information internally.
     */
    void reset_intra_lb_route();

    /**
     * @brief Creates an array [0..num_pb_graph_pins-1] for intra-logic block routing lookup.
     * Given a pb_graph_pin ID for a CLB, this lookup returns t_pb_route corresponding to that
     * pin.
     *
     * t_pb_route describes a route for a given atom net in the pb graph. This
     * function returns the t_pb_route for all atom nets within the cluster.
     *
     * @param intra_lb_pb_pin_lookup Intra-logic block pin lookup to get t_pb_graph_pin from a pin ID.
     * @return t_pb_routes An array [0..num_pb_graph_pins-1] for intra-logic block routing lookup.
     */
    t_pb_routes alloc_and_load_pb_route(const IntraLbPbPinLookup& intra_lb_pb_pin_lookup);

    /**
     * @brief Returns true if the router data has been cleaned.
     */
    inline bool is_clean() const { return is_clean_; }

  private:
    /**
     * @brief Given a pin of a net, assign route tree terminals for it.
     *
     * This assumes that the pin is not already assigned.
     */
    void add_pin_to_rt_terminals_(const AtomPinId pin_id,
                                  const AtomPBBimap& atom_to_pb);

    /**
     * @brief Given a pin of a net, remove route tree terminal from it.
     */
    void remove_pin_from_rt_terminals_(const AtomPinId pin_id,
                                       const AtomPBBimap& atom_to_pb);

    /**
     * @brief Fixup duplicate connections to a net by a logically equivalent
     *        set of primitive pins.
     */
    void fix_duplicate_equivalent_pins_(const AtomPBBimap& atom_to_pb);

    /**
     * @brief Reset the traceback information used in pathfinder.
     */
    void reset_explored_node_tb_();

    /**
     * @brief Commit or remove route tree from currently routed solution.
     */
    void commit_remove_rt_(const t_lb_trace& rt,
                           e_commit_remove op,
                           std::unordered_map<const t_pb_graph_node*, const t_mode*>& mode_map,
                           t_mode_selection_status* mode_status);

    /**
     * @brief Add source node of net as the starting point to existing route tree.
     */
    void add_source_to_rt_(int inet);

    /**
     * @brief Expand all nodes found in route tree into the priority queue.
     *
     *  @param inet     Index into the intra_lb_nets_ to expand to route tree for.
     */
    void expand_rt_(int inet);

    /**
     * @brief Recursive function used by expand_rt_ to expand all nodes in a
     *        route tree into the priority queue.
     */
    void expand_rt_rec_(const t_lb_trace& rt, int prev_index, int irt_net);

    /**
     * @brief Add new path from existing route tree to target sink.
     *
     * This reads the result of the path search to traceback the path and then
     * adds the path to the route tree.
     */
    bool add_to_rt_(t_lb_trace& rt, int node_index, int irt_net);

    /**
     * @brief Expand all nodes for a given lb_net.
     *
     * This method continuously pops from the priority queue to find the best
     * path to the target.
     */
    bool try_expand_nodes_(const t_intra_lb_net& lb_net,
                           t_expansion_node* exp_node,
                           int itarget,
                           bool try_other_modes,
                           int verbosity);

    /**
     * @brief Explore the given node popped from the priority queue and explore
     *        its edges.
     *
     *  @param exp_node     The node currently being explored.
     *  @param net_fanout   The fanout of the net being routed.
     */
    void expand_node_(const t_expansion_node& exp_node,
                      int net_fanout);

    /**
     * @brief Explore the given node popped from the priority queue using all
     *        possible modes and explore their edges.
     *
     * This will explore every possible mode that the given node can be in and
     * explore their edges.
     *
     *  @param exp_node     The node currently being explored.
     *  @param net_fanout   The fanout of the net being routed.
     */
    void expand_node_all_modes_(const t_expansion_node& exp_node,
                                int net_fanout);

    /**
     * @brief Explore all edges of an expansion node and insert connected nodes
     *        into the priority queue (i.e. connection routing).
     *
     *  @param mode         The mode the node that is being explored is in.
     *  @param cur_inode    The node that is being explored.
     *  @param cur_cost     The cost of the node that is being explored.
     *  @param net_fanout   The fanout of the net being routed.
     */
    void expand_edges_(int mode,
                       int cur_inode,
                       float cur_cost,
                       int net_fanout);

    /**
     * @brief Determine if a completed route is valid.
     *
     * A successful route has no congestion (i.e. no routing resource is used by
     * two nets).
     */
    bool is_route_success_();

    /**
     * @brief Save the last successful intra-logic block route and reset current
     *        lb_traceback.
     */
    void save_and_reset_lb_route_();

    /**
     * @brief Debugging function, used to print the description of the given
     *        lb-type RR node.
     */
    std::string describe_lb_type_rr_node_(int inode);

    /**
     * @brief Debugging function, used to print the given congested RR.
     */
    std::string describe_congested_rr_nodes_(const std::vector<int>& congested_rr_nodes);

    /**
     * @brief Debugging function, used to print out the trace of a net.
     */
    void print_trace_(FILE* fp, const t_lb_trace& trace);

    /**
     * @brief Debugging function, print out current intra logic block route.
     */
    void print_route_(const char* filename);

  private:
    // =========================================================================
    // Physical Architecture Info
    // =========================================================================

    /// @brief Pointer to physical intra-logic cluster_ctx.blocks type rr graph.
    std::vector<t_lb_type_rr_node>* lb_type_graph_;

    // =========================================================================
    // Logical Netlist Info
    // =========================================================================

    /// @brief Vector of intra logic cluster_ctx.blocks nets and their connections.
    std::vector<t_intra_lb_net> intra_lb_nets_;

    /// @brief Save vector of intra logic cluster_ctx.blocks nets and their connections.
    ///        This is used to save the solution for each successful route. This is used
    ///        to allow for other routes to be attempted, while allowing us to roll-back
    ///        to a known-legal routing. This is ultimately used by alloc_and_load_pb_route.
    std::vector<t_intra_lb_net> saved_lb_nets_;

    /// @brief Set that records which atoms are added to cluster router.
    ///        The cluster legalizer will be adding and removing atoms from the
    ///        cluster. This set keeps track of which atoms the Cluster Router is
    ///        aware of being currently in the cluster.
    std::unordered_set<AtomBlockId> atoms_added_;

    // =========================================================================
    // Logical-to-Physical Mapping Info
    // =========================================================================

    /// @brief [0..lb_type_graph->size()-1] Stats for each logic cluster_ctx.blocks rr node instance.
    std::vector<t_lb_rr_node_stats> lb_rr_node_stats_;

    // =========================================================================
    // Stored State Info During Pathfinder Iterative Routing
    // =========================================================================

    /// @brief [0..lb_type_graph->size()-1] Stores mode exploration and lb_traceback info for nodes.
    std::vector<t_explored_node_tb> explored_node_tb_;

    /// @brief Used in conjunction with explored_node_tb_ to determine whether or not a location has been explored.
    ///        By using a unique identifier every route, I don't have to clear the previous route exploration.
    ///        The idea is that for a given route, a specific explore_id is used.
    ///        When exploring nodes, if the ID does not match, we know this is from
    ///        another route. This is just an optimization to prevent clearing the
    ///        global routing data each connection route.
    int explore_id_index_;

    /// @brief The logical block type that is being routed.
    t_logical_block_type_ptr lb_type_;

    /// @brief Parameters used by the router.
    t_lb_router_params params_;

    /// @brief Current congestion factor.
    float pres_con_fac_;

    /// @brief Priority queue used during path search.
    ClusterRouterPriorityQueue pq_;

    /// @brief Flag that indicates if this object has been cleaned or not.
    bool is_clean_;

    /// @brief Flag that indicates if this object has valid state or not. If the
    ///        router is invalid, none of the methods should be used.
    bool is_valid_;
};

/**
 * @brief Free the given pb route object.
 */
void free_pb_route(t_pb_route* free_pb_route);
