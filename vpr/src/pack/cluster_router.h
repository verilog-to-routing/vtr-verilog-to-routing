#pragma once
/*
 * Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
 *
 * Author: Jason Luu
 * Date: July 22, 2013
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

/* Describes the status of a logic cluster_ctx.blocks routing resource node for a given logic cluster_ctx.blocks instance */
struct t_lb_rr_node_stats {
    int occ;  /* Number of nets currently using this lb_rr_node */
    int mode; /* Mode that this rr_node is set to */

    int historical_usage; /* Historical usage of using this node */

    t_lb_rr_node_stats() noexcept {
        occ = 0;
        mode = -1;
        historical_usage = 0;
    }
};

/*
 * Data structure forming the route tree of a net within one logic cluster_ctx.blocks.
 *
 * A net is implemented using routing resource nodes.  The t_lb_trace data structure records one of the nodes used by the net and the connections
 * to other nodes
 */
struct t_lb_trace {
    int current_node;                   /* current t_lb_type_rr_node used by net */
    std::vector<t_lb_trace> next_nodes; /* index of previous edge that drives current node */
};

/* Represents a net used inside a logic cluster_ctx.blocks and the physical nodes used by the net */
struct t_intra_lb_net {
    AtomNetId atom_net_id;             /* index of atom net this intra_lb_net represents */
    std::vector<int> terminals;        /* endpoints of the intra_lb_net, 0th position is the source, all others are sinks */
    std::vector<AtomPinId> atom_pins;  /* AtomPin's associated with each terminal */
    std::vector<bool> fixed_terminals; /* Marks a terminal as having a fixed target (i.e. a pin not a sink) */
    t_lb_trace rt_tree;                /* Route tree head */

    t_intra_lb_net() noexcept {
        atom_net_id = AtomNetId::INVALID();
        rt_tree.current_node = UNDEFINED;
    }
};

/* Stores tuning parameters used by intra-logic cluster_ctx.blocks router */
struct t_lb_router_params {
    int max_iterations;
    float pres_fac;
    float pres_fac_mult;
    float hist_fac;
};

/* Node expanded by router */
struct t_expansion_node {
    int node_index; /* Index of logic cluster_ctx.blocks rr node this expansion node represents */
    int prev_index; /* Index of logic cluster_ctx.blocks rr node that drives this expansion node */
    float cost;

    t_expansion_node() {
        node_index = UNDEFINED;
        prev_index = UNDEFINED;
        cost = 0;
    }
};

class compare_expansion_node {
  public:
    bool operator()(t_expansion_node& e1, t_expansion_node& e2) // Returns true if t1 is earlier than t2
    {
        if (e1.cost > e2.cost) {
            return true;
        }
        return false;
    }
};

/* Stores explored nodes by router */
struct t_explored_node_tb {
    int prev_index;     /* Previous node that drives this one */
    int explored_id;    /* ID used to determine if this node has been explored */
    int inet;           /* net index of route tree */
    int enqueue_id;     /* ID used to determine if this node has been pushed on exploration priority queue */
    float enqueue_cost; /* cost of node pused on exploration priority queue */

    t_explored_node_tb() noexcept {
        prev_index = UNDEFINED;
        explored_id = UNDEFINED;
        enqueue_id = UNDEFINED;
        inet = UNDEFINED;
        enqueue_cost = 0;
    }
};

/* Stores status of mode selection during clustering */
struct t_mode_selection_status {
    bool is_mode_conflict = false;
    bool try_expand_all_modes = false;
    bool expand_all_modes = false;

    bool is_mode_issue() {
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

enum e_commit_remove { RT_COMMIT,
                       RT_REMOVE };

class ClusterRouter {
  public:
    // TODO: Consider adding a move constructor.
    ClusterRouter() { is_valid_ = false; }
    ClusterRouter(std::vector<t_lb_type_rr_node>* lb_type_graph,
                  t_logical_block_type_ptr type);

    void add_atom_as_target(const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb);

    void remove_atom_from_target(const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb);

    void set_reset_pb_modes(const t_pb* pb, const bool set);

    bool try_intra_lb_route(int verbosity, t_mode_selection_status* mode_status);

    void clean_router_data();

    void reset_intra_lb_route();

    /**
     * @brief Creates an array [0..num_pb_graph_pins-1] for intra-logic block routing lookup. 
     * Given a pb_graph_pin ID for a CLB, this lookup returns t_pb_route corresponding to that
     * pin.
     *
     * @param intra_lb_pb_pin_lookup Intra-logic block pin lookup to get t_pb_graph_pin from a pin ID.
     * @return t_pb_routes An array [0..num_pb_graph_pins-1] for intra-logic block routing lookup.
     */
    t_pb_routes alloc_and_load_pb_route(const IntraLbPbPinLookup& intra_lb_pb_pin_lookup);

    inline bool is_clean() const { return is_clean_; }

  private:
    void add_pin_to_rt_terminals_(const AtomPinId pin_id,
                                  const AtomPBBimap& atom_to_pb);

    void remove_pin_from_rt_terminals_(const AtomPinId pin_id,
                                       const AtomPBBimap& atom_to_pb);

    void fix_duplicate_equivalent_pins_(const AtomPBBimap& atom_to_pb);

    void reset_explored_node_tb_();

    void commit_remove_rt_(const t_lb_trace& rt,
                           e_commit_remove op,
                           std::unordered_map<const t_pb_graph_node*, const t_mode*>& mode_map,
                           t_mode_selection_status* mode_status);

    void add_source_to_rt_(int inet);

    void expand_rt_(int inet);

    void expand_rt_rec_(const t_lb_trace& rt, int prev_index, int irt_net);

    bool add_to_rt_(t_lb_trace& rt, int node_index, int irt_net);

    bool try_expand_nodes_(t_intra_lb_net* lb_net,
                           t_expansion_node* exp_node,
                           int itarget,
                           bool try_other_modes,
                           int verbosity);

    void expand_node_(const t_expansion_node& exp_node,
                      int net_fanout);

    void expand_node_all_modes_(const t_expansion_node& exp_node,
                                int net_fanout);

    void expand_edges_(int mode,
                       int cur_inode,
                       float cur_cost,
                       int net_fanout);

    bool is_route_success_();

    void save_and_reset_lb_route_();

    std::string describe_lb_type_rr_node_(int inode);

    std::string describe_congested_rr_nodes_(const std::vector<int>& congested_rr_nodes);

    void print_trace_(FILE* fp, const t_lb_trace& trace);

    /* Physical Architecture Info */
    std::vector<t_lb_type_rr_node>* lb_type_graph_; /* Pointer to physical intra-logic cluster_ctx.blocks type rr graph */

    /* Logical Netlist Info */
    std::vector<t_intra_lb_net> intra_lb_nets_; /* Pointer to vector of intra logic cluster_ctx.blocks nets and their connections */

    /* Saved nets */
    std::vector<t_intra_lb_net> saved_lb_nets_; /* Save vector of intra logic cluster_ctx.blocks nets and their connections */

    std::unordered_set<AtomBlockId> atoms_added_; /* map that records which atoms are added to cluster router */

    /* Logical-to-physical mapping info */
    std::vector<t_lb_rr_node_stats> lb_rr_node_stats_; /* [0..lb_type_graph->size()-1] Stats for each logic cluster_ctx.blocks rr node instance */

    /* Stores state info during Pathfinder iterative routing */
    std::vector<t_explored_node_tb> explored_node_tb_; /* [0..lb_type_graph->size()-1] Stores mode exploration and lb_traceback info for nodes */
    int explore_id_index_;                 /* used in conjunction with node_traceback to determine whether or not a location has been explored.  By using a unique identifier every route, I don't have to clear the previous route exploration */

    /* Current type */
    t_logical_block_type_ptr lb_type_;

    /* Parameters used by router */
    t_lb_router_params params_;

    /* current congestion factor */
    float pres_con_fac_;

    // TODO: make this a typedef.
    reservable_pq<t_expansion_node, std::vector<t_expansion_node>, compare_expansion_node> pq_;

    bool is_clean_;

    bool is_valid_;
};

void free_pb_route(t_pb_route* free_pb_route);
