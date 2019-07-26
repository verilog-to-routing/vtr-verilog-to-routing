#ifndef PACK_TYPES_H
#define PACK_TYPES_H
/**
 * Jason Luu
 * July 22, 2013
 *
 * Defines core data structures used in packing
 */
#include <map>
#include <unordered_map>
#include <vector>

#include "arch_types.h"
#include "atom_netlist_fwd.h"

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

/* Stores statistical information for a physical cluster_ctx.blocks such as costs and usages */
struct t_pb_stats {
    /* Packing statistics */
    std::map<AtomBlockId, float> gain; /* Attraction (inverse of cost) function */

    std::map<AtomBlockId, float> timinggain;     /* The timing criticality score of this atom cluster_ctx.blocks.
                                                  * Determined by the most critical atom net
                                                  * between this atom cluster_ctx.blocks and any atom cluster_ctx.blocks in
                                                  * the current pb */
    std::map<AtomBlockId, float> connectiongain; /* Weighted sum of connections to attraction function */
    std::map<AtomBlockId, float> sharinggain;    /* How many nets on an atom cluster_ctx.blocks are already in the pb under consideration */

    /* This is the gain used for hill-climbing. It stores*
     * the reduction in the number of pins that adding this atom cluster_ctx.blocks to the the*
     * current pb will have. This reflects the fact that sometimes the *
     * addition of an atom cluster_ctx.blocks to a pb may reduce the number of inputs     *
     * required if it shares inputs with all other BLEs and it's output is  *
     * used by all other child pbs in this parent pb.                               */
    std::map<AtomBlockId, float> hillgain;

    std::vector<AtomNetId> marked_nets;     //List of nets with the num_pins_of_net_in_pb and gain entries altered
    std::vector<AtomBlockId> marked_blocks; //List of blocks with the num_pins_of_net_in_pb and gain entries altered

    int num_child_blocks_in_pb;

    AtomNetId tie_break_high_fanout_net;                                  /* If no marked candidate molecules, use
                                                                           * this high fanout net to determine the
                                                                           * next candidate atom */
    bool explore_transitive_fanout;                                       /* If no marked candidate molecules and no high fanout nets to determine next candidate molecule then explore molecules on transitive fanout */
    std::map<AtomBlockId, t_pack_molecule*> transitive_fanout_candidates; // Holding trasitive fanout candidates key: root block id of the molecule, value: pointer to the molecule

    /* How many pins of each atom net are contained in the *
     * currently open pb?                                  */
    std::map<AtomNetId, int> num_pins_of_net_in_pb;

    /* Record of pins of class used */
    std::vector<std::unordered_map<size_t, AtomNetId>> input_pins_used;  /* [0..pb_graph_node->num_pin_classes-1] nets using this input pin class */
    std::vector<std::unordered_map<size_t, AtomNetId>> output_pins_used; /* [0..pb_graph_node->num_pin_classes-1] nets using this output pin class */

    /* Use vector because array size is expected to be small so runtime should be faster using vector than map despite the O(N) vs O(log(n)) behaviour.*/
    std::vector<std::vector<AtomNetId>> lookahead_input_pins_used;  /* [0..pb_graph_node->num_pin_classes-1] vector of input pins of this class that are speculatively used */
    std::vector<std::vector<AtomNetId>> lookahead_output_pins_used; /* [0..pb_graph_node->num_pin_classes-1] vector of input pins of this class that are speculatively used */

    /* Array of feasible blocks to select from [0..max_array_size-1]
     * Sorted in ascending gain order so that the last cluster_ctx.blocks is the most desirable (this makes it easy to pop blocks off the list
     */
    t_pack_molecule** feasible_blocks;
    int num_feasible_blocks; /* [0..num_marked_models-1] */
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
    short* num_fanout;      /* [0..num_modes - 1] Mode dependant fanout */
    enum e_lb_rr_type type; /* Type of logic cluster_ctx.blocks resource node */

    t_lb_type_rr_node_edge** outedges; /* [0..num_modes - 1][0..num_fanout-1] index and cost of out edges */

    t_pb_graph_pin* pb_graph_pin; /* pb_graph_pin associated with this lb_rr_node if exists, NULL otherwise */
    float intrinsic_cost;         /* cost of this node */

    t_lb_type_rr_node() {
        capacity = 0;
        num_modes = 0;
        num_fanout = nullptr;
        type = NUM_LB_RR_TYPES;
        outedges = nullptr;
        pb_graph_pin = nullptr;
        intrinsic_cost = 0;
    }
};

/**************************************************************************
 * Intra-Logic Block Routing Data Structures (by instance)
 ***************************************************************************/

/* A routing traceback data structure, provides a logic cluster_ctx.blocks instance specific trace lookup directly from the t_lb_type_rr_node array index
 * After packing, routing info for each CLB will have an array of t_lb_traceback to store routing info within the CLB
 */
struct t_lb_traceback {
    int net;             /* net of flat, technology-mapped, netlist using this node */
    int prev_lb_rr_node; /* index of previous node that drives current node */
    int prev_edge;       /* index of previous edge that drives current node */
};

/**************************************************************************
 * Intra-Logic Block Router Data Structures
 ***************************************************************************/

/* Describes the status of a logic cluster_ctx.blocks routing resource node for a given logic cluster_ctx.blocks instance */
struct t_lb_rr_node_stats {
    int occ;  /* Number of nets currently using this lb_rr_node */
    int mode; /* Mode that this rr_node is set to */

    int historical_usage; /* Historical usage of using this node */

    t_lb_rr_node_stats() {
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
    t_lb_trace* rt_tree;               /* Route tree head */

    t_intra_lb_net() {
        atom_net_id = AtomNetId::INVALID();
        rt_tree = nullptr;
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
        node_index = OPEN;
        prev_index = OPEN;
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
    int prev_index;     /* Prevous node that drives this one */
    int explored_id;    /* ID used to determine if this node has been explored */
    int inet;           /* net index of route tree */
    int enqueue_id;     /* ID used ot determine if this node has been pushed on exploration priority queue */
    float enqueue_cost; /* cost of node pused on exploration priority queue */

    t_explored_node_tb() {
        prev_index = OPEN;
        explored_id = OPEN;
        enqueue_id = OPEN;
        inet = OPEN;
        enqueue_cost = 0;
    }
};

/* Stores all data needed by intra-logic cluster_ctx.blocks router */
struct t_lb_router_data {
    /* Physical Architecture Info */
    std::vector<t_lb_type_rr_node>* lb_type_graph; /* Pointer to physical intra-logic cluster_ctx.blocks type rr graph */

    /* Logical Netlist Info */
    std::vector<t_intra_lb_net>* intra_lb_nets; /* Pointer to vector of intra logic cluster_ctx.blocks nets and their connections */

    /* Saved nets */
    std::vector<t_intra_lb_net>* saved_lb_nets; /* Save vector of intra logic cluster_ctx.blocks nets and their connections */

    std::map<AtomBlockId, bool>* atoms_added; /* map that records which atoms are added to cluster router */

    /* Logical-to-physical mapping info */
    t_lb_rr_node_stats* lb_rr_node_stats; /* [0..lb_type_graph->size()-1] Stats for each logic cluster_ctx.blocks rr node instance */
    bool is_routed;                       /* Stores whether or not the current logical-to-physical mapping has a routed solution */

    /* Stores state info during Pathfinder iterative routing */
    t_explored_node_tb* explored_node_tb; /* [0..lb_type_graph->size()-1] Stores mode exploration and traceback info for nodes */
    int explore_id_index;                 /* used in conjunction with node_traceback to determine whether or not a location has been explored.  By using a unique identifier every route, I don't have to clear the previous route exploration */

    /* Current type */
    t_type_ptr lb_type;

    /* Parameters used by router */
    t_lb_router_params params;

    /* current congestion factor */
    float pres_con_fac;

    t_lb_router_data() {
        lb_type_graph = nullptr;
        lb_rr_node_stats = nullptr;
        intra_lb_nets = nullptr;
        saved_lb_nets = nullptr;
        is_routed = false;
        lb_type = nullptr;
        atoms_added = nullptr;
        explored_node_tb = nullptr;
        explore_id_index = 1;

        params.max_iterations = 50;
        params.pres_fac = 1;
        params.pres_fac_mult = 2;
        params.hist_fac = 0.3;

        pres_con_fac = 1;
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

#endif
