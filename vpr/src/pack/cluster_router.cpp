/*
 * Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
 *
 * Global Inputs: Architecture and netlist
 * Input arguments: clustering info for one cluster (t_pb info)
 * Working data set: t_routing_data contains intermediate work
 * Output: Routable? true/false.  If true, store/return the routed solution.
 *
 * Routing algorithm used is Pathfinder.
 *
 * Author: Jason Luu
 * Date: July 22, 2013
 */

#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>
#include <map>
#include <queue>
#include <cmath>

#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_error.h"
#include "vpr_types.h"

#include "physical_types.h"
#include "globals.h"
#include "atom_netlist.h"
#include "vpr_utils.h"
#include "pack_types.h"
#include "pb_type_graph.h"
#include "lb_type_rr_graph.h"
#include "cluster_router.h"
#include "atom_pb_bimap.h"

// FIXME: Turn this on and debug.
/* #define PRINT_INTRA_LB_ROUTE */

/*****************************************************************************************
 * Internal functions declarations
 ******************************************************************************************/
static void reset_lb_net_rt(t_lb_trace& lb_trace);

static bool is_skip_route_net(const t_lb_trace& rt,
                              const std::vector<t_lb_type_rr_node>& lb_type_graph,
                              const std::vector<t_lb_rr_node_stats>& lb_rr_node_stats);

static t_lb_trace* find_node_in_rt(t_lb_trace& rt, int rt_index);

/**
 * @brief Recurse through route tree trace to populate pb pin to atom net lookup array.
 * 
 * @param pb_route Array of pb pin to atom net lookup to be populated in this routine.
 * @param net_id Atom net ID of the current net.
 * @param prev_pin_id ID of the previous pin in the route tree trace.
 * @param trace Current trace node in the route tree.
 * @param logic_block_type Logic block type of the current cluster.
 * @param intra_lb_pb_pin_lookup Intra-logic block pin lookup to get t_pb_graph_pin from a pin ID.
 */
static void load_trace_to_pb_route(t_pb_routes& pb_route,
                                   const AtomNetId net_id,
                                   const int prev_pin_id,
                                   const t_lb_trace& trace,
                                   t_logical_block_type_ptr logic_block_type,
                                   const IntraLbPbPinLookup& intra_lb_pb_pin_lookup);

static std::vector<int> find_congested_rr_nodes(const std::vector<t_lb_type_rr_node>& lb_type_graph,
                                                const std::vector<t_lb_rr_node_stats>& lb_rr_node_stats);
static std::vector<int> find_incoming_rr_nodes(int dst_node, const std::vector<t_lb_type_rr_node>& lb_rr_graph);
/*****************************************************************************************
 * Debug functions declarations
 ******************************************************************************************/
#ifdef PRINT_INTRA_LB_ROUTE
static void print_route(const char* filename, t_lb_router_data* router_data);
static void print_route(FILE* fp, t_lb_router_data* router_data);
#endif

/*****************************************************************************************
 * Constructor/Destructor functions
 ******************************************************************************************/

ClusterRouter::ClusterRouter(std::vector<t_lb_type_rr_node>* lb_type_graph,
                             t_logical_block_type_ptr type) {
    lb_type_graph_ = lb_type_graph;
    lb_type_ = type;

    size_t size = lb_type_graph->size();
    lb_rr_node_stats_.resize(size);
    explored_node_tb_.resize(size);

    explore_id_index_ = 1;

    params_.max_iterations = 50;
    params_.pres_fac = 1;
    params_.pres_fac_mult = 2;
    params_.hist_fac = 0.3;

    pres_con_fac_ = 1;

    is_clean_ = false;
    is_valid_ = true;
}

void ClusterRouter::clean_router_data() {
    lb_rr_node_stats_.clear();
    lb_rr_node_stats_.shrink_to_fit();
    explored_node_tb_.clear();
    explored_node_tb_.shrink_to_fit();
    atoms_added_.clear();
    intra_lb_nets_.clear();
    intra_lb_nets_.shrink_to_fit();
    saved_lb_nets_.clear();
    saved_lb_nets_.shrink_to_fit();

    is_clean_ = true;
}

static bool route_has_conflict(const t_lb_trace& rt,
                               const std::vector<t_lb_type_rr_node>& lb_type_graph) {
    int cur_mode = -1;
    for (size_t i = 0; i < rt.next_nodes.size(); i++) {
        int new_mode = get_lb_type_rr_graph_edge_mode(lb_type_graph,
                                                      rt.current_node, rt.next_nodes[i].current_node);
        if (cur_mode != -1 && cur_mode != new_mode) {
            return true;
        }
        if (route_has_conflict(rt.next_nodes[i], lb_type_graph) == true) {
            return true;
        }
        cur_mode = new_mode;
    }

    return false;
}

// Check one edge for mode conflict.
static bool check_edge_for_route_conflicts(std::unordered_map<const t_pb_graph_node*, const t_mode*>& mode_map,
                                           const t_pb_graph_pin* driver_pin,
                                           const t_pb_graph_pin* pin) {
    if (driver_pin == nullptr) {
        return false;
    }

    // Only check pins that are OUT_PORTs.
    if (pin == nullptr || pin->port == nullptr || pin->port->type != OUT_PORT) {
        return false;
    }
    VTR_ASSERT(!pin->port->is_clock);

    auto* pb_graph_node = pin->parent_node;
    VTR_ASSERT(pb_graph_node->pb_type == pin->port->parent_pb_type);

    const t_pb_graph_edge* edge = get_edge_between_pins(driver_pin, pin);
    VTR_ASSERT(edge != nullptr);

    auto mode_of_edge = edge->interconnect->parent_mode_index;
    auto* mode = &pb_graph_node->pb_type->modes[mode_of_edge];

    auto result = mode_map.insert(std::make_pair(pb_graph_node, mode));

    /* Insert unpackable mode to the illegal mode list */
    if (mode->disable_packing) {
        if (std::find(pb_graph_node->illegal_modes.begin(), pb_graph_node->illegal_modes.end(), mode->index) == pb_graph_node->illegal_modes.end()) {
            pb_graph_node->illegal_modes.push_back(mode->index);
        }
        return true;
    }

    if (!result.second) {
        if (result.first->second != mode) {
            std::cout << vtr::string_fmt("Differing modes for block.  Got %s mode, while previously was %s for interconnect %s.",
                                         mode->name, result.first->second->name,
                                         edge->interconnect->name)
                      << std::endl;

            // The illegal mode is added to the pb_graph_node as it resulted in a conflict during atom-to-atom routing. This mode cannot be used in the consequent cluster
            // generation try.
            if (std::find(pb_graph_node->illegal_modes.begin(), pb_graph_node->illegal_modes.end(), result.first->second->index) == pb_graph_node->illegal_modes.end()) {
                pb_graph_node->illegal_modes.push_back(result.first->second->index);
            }

            // If the number of illegal modes equals the number of available mode for a specific pb_graph_node it means that no cluster can be generated. This results
            // in a fatal error.
            if ((int)pb_graph_node->illegal_modes.size() >= pb_graph_node->pb_type->num_modes) {
                VPR_FATAL_ERROR(VPR_ERROR_PACK, "There are no more available modes to be used. Routing Failed!");
            }

            return true;
        }
    }

    return false;
}

/*****************************************************************************************
 * Routing Functions
 ******************************************************************************************/

/* Add pins of netlist atom to to current routing drivers/targets */
void ClusterRouter::add_atom_as_target(const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    if (atoms_added_.count(blk_id) > 0) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Atom %s added twice to router\n", atom_ctx.netlist().block_name(blk_id).c_str());
    }

    const t_pb* pb = atom_to_pb.atom_pb(blk_id);

    VTR_ASSERT(pb);

    atoms_added_.insert(blk_id);

    set_reset_pb_modes(pb, true);

    for (auto pin_id : atom_ctx.netlist().block_pins(blk_id)) {
        add_pin_to_rt_terminals_(pin_id, atom_to_pb);
    }

    fix_duplicate_equivalent_pins_(atom_to_pb);
}

/* Remove pins of netlist atom from current routing drivers/targets */
void ClusterRouter::remove_atom_from_target(const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    if (atoms_added_.count(blk_id) == 0) {
        return;
    }

    const t_pb* pb = atom_to_pb.atom_pb(blk_id);
    set_reset_pb_modes(pb, false);

    for (auto pin_id : atom_ctx.netlist().block_pins(blk_id)) {
        remove_pin_from_rt_terminals_(pin_id, atom_to_pb);
    }

    atoms_added_.erase(blk_id);
}

/* Set/Reset mode of rr nodes to the pb used.  If set == true, then set all modes of the rr nodes affected by pb to the mode of the pb.
 * Set all modes related to pb to 0 otherwise */
void ClusterRouter::set_reset_pb_modes(const t_pb* pb, const bool set) {
    int mode = pb->mode;
    VTR_ASSERT(mode >= 0);

    t_pb_graph_node* pb_graph_node = pb->pb_graph_node;
    t_pb_type* pb_type = pb_graph_node->pb_type;

    /* Input and clock pin modes are based on current pb mode */
    for (int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
            int inode = pb_graph_node->input_pins[iport][ipin].pin_count_in_cluster;
            lb_rr_node_stats_[inode].mode = (set == true) ? mode : -1;
        }
    }
    for (int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
            int inode = pb_graph_node->clock_pins[iport][ipin].pin_count_in_cluster;
            lb_rr_node_stats_[inode].mode = (set == true) ? mode : -1;
        }
    }

    /* Output pin modes are based on parent pb, so set children to use new mode
     * Output pin of top-level logic block is also set to mode 0
     */
    if (pb_type->num_modes != 0) {
        for (int ichild_type = 0; ichild_type < pb_type->modes[mode].num_pb_type_children; ichild_type++) {
            for (int ichild = 0; ichild < pb_type->modes[mode].pb_type_children[ichild_type].num_pb; ichild++) {
                t_pb_graph_node* child_pb_graph_node = &pb_graph_node->child_pb_graph_nodes[mode][ichild_type][ichild];
                for (int iport = 0; iport < child_pb_graph_node->num_output_ports; iport++) {
                    for (int ipin = 0; ipin < child_pb_graph_node->num_output_pins[iport]; ipin++) {
                        int inode = child_pb_graph_node->output_pins[iport][ipin].pin_count_in_cluster;
                        lb_rr_node_stats_[inode].mode = (set == true) ? mode : -1;
                    }
                }
            }
        }
    }
}

/* Expand all the nodes for a given lb_net */
bool ClusterRouter::try_expand_nodes_(t_intra_lb_net* lb_net,
                                      t_expansion_node* exp_node,
                                      int itarget,
                                      bool try_other_modes,
                                      int verbosity) {
    bool is_impossible = false;

    do {
        if (pq_.empty()) {
            /* No connection possible */
            is_impossible = true;

            if (verbosity > 3) {
                //Print detailed debug info
                auto& atom_nlist = g_vpr_ctx.atom().netlist();
                AtomNetId net_id = lb_net->atom_net_id;
                AtomPinId driver_pin = lb_net->atom_pins[0];
                AtomPinId sink_pin = lb_net->atom_pins[itarget];
                int driver_rr_node = lb_net->terminals[0];
                int sink_rr_node = lb_net->terminals[itarget];

                VTR_LOG("\t\t\tNo possible routing path from %s to %s: needed for net '%s' from net pin '%s'",
                        describe_lb_type_rr_node_(driver_rr_node).c_str(),
                        describe_lb_type_rr_node_(sink_rr_node).c_str(),
                        atom_nlist.net_name(net_id).c_str(),
                        atom_nlist.pin_name(driver_pin).c_str());
                VTR_LOGV(sink_pin, " to net pin '%s'", atom_nlist.pin_name(sink_pin).c_str());
                VTR_LOG("\n");
            }
        } else {
            *exp_node = pq_.top();
            pq_.pop();
            int exp_inode = exp_node->node_index;

            if (explored_node_tb_[exp_inode].explored_id != explore_id_index_) {
                /* First time node is popped implies path to this node is the lowest cost.
                 * If the node is popped a second time, then the path to that node is higher than this path so
                 * ignore.
                 */
                explored_node_tb_[exp_inode].explored_id = explore_id_index_;
                explored_node_tb_[exp_inode].prev_index = exp_node->prev_index;
                if (exp_inode != lb_net->terminals[itarget]) {
                    if (!try_other_modes) {
                        expand_node_(*exp_node, lb_net->terminals.size() - 1);
                    } else {
                        expand_node_all_modes_(*exp_node, lb_net->terminals.size() - 1);
                    }
                }
            }
        }
    } while (exp_node->node_index != lb_net->terminals[itarget] && !is_impossible);

    return is_impossible;
}

/* Attempt to route routing driver/targets on the current architecture
 * Follows pathfinder negotiated congestion algorithm
 */
bool ClusterRouter::try_intra_lb_route(int verbosity,
                                       t_mode_selection_status* mode_status) {
    VTR_ASSERT(is_valid_);
    bool is_routed = false;
    bool is_impossible = false;

    mode_status->is_mode_conflict = false;
    mode_status->try_expand_all_modes = false;

    t_expansion_node exp_node;

    reset_explored_node_tb_();

    /* Reset current routing */
    for (size_t inet = 0; inet < intra_lb_nets_.size(); inet++) {
        reset_lb_net_rt(intra_lb_nets_[inet].rt_tree);
    }
    for (size_t inode = 0; inode < lb_type_graph_->size(); inode++) {
        lb_rr_node_stats_[inode].historical_usage = 0;
        lb_rr_node_stats_[inode].occ = 0;
    }

    std::unordered_map<const t_pb_graph_node*, const t_mode*> mode_map;

    /*	Iteratively remove congestion until a successful route is found.
     * Cap the total number of iterations tried so that if a solution does not exist, then the router won't run indefinitely */
    pres_con_fac_ = params_.pres_fac;
    for (int iter = 0; iter < params_.max_iterations && !is_routed && !is_impossible; iter++) {
        size_t inet;
        /* Iterate across all nets internal to logic block */
        for (inet = 0; inet < intra_lb_nets_.size() && !is_impossible; inet++) {
            int idx = inet;
            if (intra_lb_nets_[idx].rt_tree.current_node != UNDEFINED && is_skip_route_net(intra_lb_nets_[idx].rt_tree, *lb_type_graph_, lb_rr_node_stats_)) {
                continue;
            }
            if (intra_lb_nets_[idx].rt_tree.current_node != UNDEFINED) {
                commit_remove_rt_(intra_lb_nets_[idx].rt_tree, RT_REMOVE, mode_map, mode_status);
            }
            reset_lb_net_rt(intra_lb_nets_[idx].rt_tree);
            add_source_to_rt_(idx);

            /* Route each sink of net */
            for (size_t itarget = 1; itarget < intra_lb_nets_[idx].terminals.size() && !is_impossible; itarget++) {
                pq_.clear();
                /* Get lowest cost next node, repeat until a path is found or if it is impossible to route */

                expand_rt_(idx);

                is_impossible = try_expand_nodes_(&intra_lb_nets_[idx], &exp_node, itarget, mode_status->expand_all_modes, verbosity);

                if (is_impossible && !mode_status->expand_all_modes) {
                    mode_status->try_expand_all_modes = true;
                    mode_status->expand_all_modes = true;
                    break;
                }

                if (exp_node.node_index == intra_lb_nets_[idx].terminals[itarget]) {
                    /* Net terminal is routed, add this to the route tree, clear data structures, and keep going */
                    is_impossible = add_to_rt_(intra_lb_nets_[idx].rt_tree, exp_node.node_index, idx);
                }

                if (verbosity > 5) {
                    VTR_LOG("Routing finished\n");
                    VTR_LOG("\tS");
                    print_trace_(stdout, intra_lb_nets_[idx].rt_tree);
                    VTR_LOG("\n");
                }

                if (is_impossible) {
                    VTR_LOGV(verbosity > 5, "Routing was impossible!\n");
                } else if (mode_status->expand_all_modes) {
                    is_impossible = route_has_conflict(intra_lb_nets_[idx].rt_tree, *lb_type_graph_);
                    if (is_impossible) {
                        VTR_LOGV(verbosity > 5, "Routing was impossible due to modes!\n");
                    }
                }

                explore_id_index_++;
                if (explore_id_index_ > 2000000000) {
                    /* overflow protection */
                    for (size_t id = 0; id < lb_type_graph_->size(); id++) {
                        explored_node_tb_[id].explored_id = UNDEFINED;
                        explored_node_tb_[id].enqueue_id = UNDEFINED;
                        explore_id_index_ = 1;
                    }
                }
            }

            if (!is_impossible) {
                VTR_ASSERT(intra_lb_nets_[idx].rt_tree.current_node != UNDEFINED);
                commit_remove_rt_(intra_lb_nets_[idx].rt_tree, RT_COMMIT, mode_map, mode_status);
                if (mode_status->is_mode_conflict) {
                    is_impossible = true;
                }
            }
        }

        if (!is_impossible) {
            is_routed = is_route_success_();
        } else {
            --inet;
            auto& atom_ctx = g_vpr_ctx.atom();
            VTR_LOGV(verbosity > 3, "Net '%s' is impossible to route within proposed %s cluster\n",
                     atom_ctx.netlist().net_name(intra_lb_nets_[inet].atom_net_id).c_str(), lb_type_->name.c_str());
            is_routed = false;
        }
        pres_con_fac_ *= params_.pres_fac_mult;
    }

    if (is_routed) {
        save_and_reset_lb_route_();
    } else {
        //Unroutable
#ifdef PRINT_INTRA_LB_ROUTE
        print_route(getEchoFileName(E_ECHO_INTRA_LB_FAILED_ROUTE), router_data);
#endif

        if (verbosity > 3 && !is_impossible) {
            //Report the congested nodes and associated nets
            auto congested_rr_nodes = find_congested_rr_nodes(*lb_type_graph_, lb_rr_node_stats_);
            if (!congested_rr_nodes.empty()) {
                VTR_LOG("%s\n", describe_congested_rr_nodes_(congested_rr_nodes).c_str());
            }
        }

        //Clean-up
        for (size_t inet = 0; inet < intra_lb_nets_.size(); inet++) {
            reset_lb_net_rt(intra_lb_nets_[inet].rt_tree);
        }
    }
    return is_routed;
}

/*****************************************************************************************
 * Accessor Functions
 ******************************************************************************************/

t_pb_routes ClusterRouter::alloc_and_load_pb_route(const IntraLbPbPinLookup& intra_lb_pb_pin_lookup) {
    t_pb_routes pb_route;

    for (const auto& lb_net : saved_lb_nets_) {
        VTR_ASSERT(lb_net.rt_tree.current_node != UNDEFINED);
        load_trace_to_pb_route(pb_route, lb_net.atom_net_id, UNDEFINED, lb_net.rt_tree, lb_type_, intra_lb_pb_pin_lookup);
    }

    return pb_route;
}

/* Free pin-to-atomic_net array lookup */
void free_pb_route(t_pb_route* pb_route) {
    if (pb_route != nullptr) {
        delete[] pb_route;
    }
}

/***************************************************************************
 * Internal Functions
 ****************************************************************************/

static void load_trace_to_pb_route(t_pb_routes& pb_route,
                                   const AtomNetId net_id,
                                   const int prev_pin_id,
                                   const t_lb_trace& trace,
                                   t_logical_block_type_ptr logic_block_type,
                                   const IntraLbPbPinLookup& intra_lb_pb_pin_lookup) {
    int ipin = trace.current_node;
    int driver_pb_pin_id = prev_pin_id;
    int cur_pin_id = UNDEFINED;
    const int total_pins = logic_block_type->pb_graph_head->total_pb_pins;
    if (ipin < total_pins) {
        /* This routing node corresponds with a pin.  This node is virtual (ie. sink or source node) */
        cur_pin_id = ipin;
        if (!pb_route.count(ipin)) {
            pb_route.insert(std::make_pair(cur_pin_id, t_pb_route()));
            pb_route[cur_pin_id].atom_net_id = net_id;
            pb_route[cur_pin_id].driver_pb_pin_id = driver_pb_pin_id;
            const t_pb_graph_pin* pb_graph_pin = intra_lb_pb_pin_lookup.pb_gpin(logic_block_type->index, cur_pin_id);
            pb_route[cur_pin_id].pb_graph_pin = pb_graph_pin;
        } else {
            VTR_ASSERT(pb_route[cur_pin_id].atom_net_id == net_id);
        }
    }
    for (const auto& nxt_trace : trace.next_nodes) {
        load_trace_to_pb_route(pb_route, net_id, cur_pin_id, nxt_trace, logic_block_type, intra_lb_pb_pin_lookup);
    }
}

/* Free route tree for intra-logic block routing */
static void reset_lb_net_rt(t_lb_trace& lb_trace) {
    lb_trace.current_node = UNDEFINED;
    lb_trace.next_nodes.clear();
}

/* Given a pin of a net, assign route tree terminals for it
 * Assumes that pin is not already assigned
 */
void ClusterRouter::add_pin_to_rt_terminals_(const AtomPinId pin_id,
                                             const AtomPBBimap& atom_to_pb) {
    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;
    bool found = false;
    unsigned int ipos;
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.netlist(), atom_to_pb, pin_id);
    VTR_ASSERT(pb_graph_pin);

    AtomPortId port_id = atom_ctx.netlist().pin_port(pin_id);
    AtomNetId net_id = atom_ctx.netlist().pin_net(pin_id);

    if (!net_id) {
        //No net connected to this pin, so nothing to route
        return;
    }

    /* Find if current net is in route tree, if not, then add to rt.
     * Code assumes that # of nets in cluster is small so a linear search through
     * vector is faster than using more complex data structures
     */
    for (ipos = 0; ipos < intra_lb_nets_.size(); ipos++) {
        if (intra_lb_nets_[ipos].atom_net_id == net_id) {
            found = true;
            break;
        }
    }
    if (found == false) {
        struct t_intra_lb_net new_net;

        new_net.atom_net_id = net_id;

        ipos = intra_lb_nets_.size();
        intra_lb_nets_.push_back(new_net);
    }
    VTR_ASSERT(intra_lb_nets_[ipos].atom_net_id == net_id);
    VTR_ASSERT(intra_lb_nets_[ipos].atom_pins.size() == intra_lb_nets_[ipos].terminals.size());

    /*
     * Determine whether or not this is a new intra lb net, if yes, then add to list of intra lb nets
     */
    if (intra_lb_nets_[ipos].terminals.empty()) {
        /* Add terminals */

        //Default assumption is that the source is outside the current cluster (will be overridden later if required)
        int source_terminal = get_lb_type_rr_graph_ext_source_index(lb_type_);
        intra_lb_nets_[ipos].terminals.push_back(source_terminal);

        AtomPinId net_driver_pin_id = atom_ctx.netlist().net_driver(net_id);
        intra_lb_nets_[ipos].atom_pins.push_back(net_driver_pin_id);

        VTR_ASSERT_MSG(lb_type_graph[intra_lb_nets_[ipos].terminals[0]].type == LB_SOURCE, "Driver must be a source");
    }

    VTR_ASSERT(intra_lb_nets_[ipos].atom_pins.size() == intra_lb_nets_[ipos].terminals.size());

    if (atom_ctx.netlist().port_type(port_id) == PortType::OUTPUT) {
        //The current pin is the net driver, overwrite the default driver at index 0
        VTR_ASSERT_MSG(intra_lb_nets_[ipos].terminals[0] == get_lb_type_rr_graph_ext_source_index(lb_type_), "Default driver must be external source");

        VTR_ASSERT(atom_ctx.netlist().pin_type(pin_id) == PinType::DRIVER);

        //Override the default since this is the driver, and it is within the cluster
        intra_lb_nets_[ipos].terminals[0] = pb_graph_pin->pin_count_in_cluster;
        intra_lb_nets_[ipos].atom_pins[0] = pin_id;

        VTR_ASSERT_MSG(lb_type_graph[intra_lb_nets_[ipos].terminals[0]].type == LB_SOURCE, "Driver must be a source");

        int sink_terminal = UNDEFINED;
        if (intra_lb_nets_[ipos].terminals.size() < atom_ctx.netlist().net_pins(net_id).size()) {
            //Not all of the pins are within the cluster
            if (intra_lb_nets_[ipos].terminals.size() == 1) {
                //Only the source has been specified so far, must add cluster-external sink
                sink_terminal = get_lb_type_rr_graph_ext_sink_index(lb_type_);
                intra_lb_nets_[ipos].terminals.push_back(sink_terminal);
                intra_lb_nets_[ipos].atom_pins.push_back(AtomPinId::INVALID());
            } else {
                VTR_ASSERT(intra_lb_nets_[ipos].terminals.size() > 1);

                //TODO: Figure out why we swap terminal 1 here (although it appears to work correctly,
                //      it's not clear why this is needed...)

                //Move current terminal 1 to end
                sink_terminal = intra_lb_nets_[ipos].terminals[1];
                AtomPinId sink_atom_pin = intra_lb_nets_[ipos].atom_pins[1];
                intra_lb_nets_[ipos].terminals.push_back(sink_terminal);
                intra_lb_nets_[ipos].atom_pins.push_back(sink_atom_pin);

                //Create external sink at terminal 1
                sink_terminal = get_lb_type_rr_graph_ext_sink_index(lb_type_);
                intra_lb_nets_[ipos].terminals[1] = sink_terminal;
                intra_lb_nets_[ipos].atom_pins[1] = AtomPinId::INVALID();
            }
            VTR_ASSERT(lb_type_graph[intra_lb_nets_[ipos].terminals[1]].type == LB_SINK);
        }
    } else {
        //This is an input to a primitive
        VTR_ASSERT(atom_ctx.netlist().port_type(port_id) == PortType::INPUT
                   || atom_ctx.netlist().port_type(port_id) == PortType::CLOCK);
        VTR_ASSERT(atom_ctx.netlist().pin_type(pin_id) == PinType::SINK);

        //Get the rr node index associated with the pin
        int pin_index = pb_graph_pin->pin_count_in_cluster;
        VTR_ASSERT(lb_type_graph[pin_index].num_modes == 1);
        VTR_ASSERT(lb_type_graph[pin_index].num_fanout[0] == 1);

        /* We actually route to the sink (to handle logically equivalent pins).
         * The sink is one past the primitive input pin */
        int sink_index = lb_type_graph[pin_index].outedges[0][0].node_index;
        VTR_ASSERT(lb_type_graph[sink_index].type == LB_SINK);

        if (intra_lb_nets_[ipos].terminals.size() == atom_ctx.netlist().net_pins(net_id).size() && intra_lb_nets_[ipos].terminals[1] == get_lb_type_rr_graph_ext_sink_index(lb_type_)) {
            /* If all sinks of net are all contained in the logic block, then the net does
             * not need to route out of the logic block, so can replace the external sink
             * with this last sink terminal */
            intra_lb_nets_[ipos].terminals[1] = sink_index;
            intra_lb_nets_[ipos].atom_pins[1] = pin_id;
        } else {
            //Add as a regular sink
            intra_lb_nets_[ipos].terminals.push_back(sink_index);
            intra_lb_nets_[ipos].atom_pins.push_back(pin_id);
        }
    }
    VTR_ASSERT(intra_lb_nets_[ipos].atom_pins.size() == intra_lb_nets_[ipos].terminals.size());

    int num_lb_terminals = intra_lb_nets_[ipos].terminals.size();
    VTR_ASSERT(num_lb_terminals <= (int)atom_ctx.netlist().net_pins(net_id).size());
    VTR_ASSERT(num_lb_terminals >= 0);

#ifdef VTR_ASSERT_SAFE_ENABLED
    //Sanity checks
    int num_extern_sources = 0;
    int num_extern_sinks = 0;
    for (size_t term_idx = 0; term_idx < intra_lb_nets_[ipos].terminals.size(); ++term_idx) {
        int inode = intra_lb_nets_[ipos].terminals[term_idx];
        AtomPinId atom_pin = intra_lb_nets_[ipos].atom_pins[term_idx];
        if (term_idx == 0) {
            //Net driver
            VTR_ASSERT_SAFE_MSG(lb_type_graph[inode].type == LB_SOURCE, "Driver must be a source RR node");
            VTR_ASSERT_SAFE_MSG(atom_pin, "Driver have an associated atom pin");
            VTR_ASSERT_SAFE_MSG(atom_ctx.netlist().pin_type(atom_pin) == PinType::DRIVER, "Source RR must be associated with a driver pin in atom netlist");
            if (inode == get_lb_type_rr_graph_ext_source_index(lb_type_)) {
                ++num_extern_sources;
            }
        } else {
            //Net sink
            VTR_ASSERT_SAFE_MSG(lb_type_graph[inode].type == LB_SINK, "Non-driver must be a sink");

            if (inode == get_lb_type_rr_graph_ext_sink_index(lb_type_)) {
                //External sink may have multiple potentially matching atom pins, so it's atom pin is left invalid
                VTR_ASSERT_SAFE_MSG(!atom_pin, "Cluster external sink should have no valid atom pin");
                ++num_extern_sinks;
            } else {
                VTR_ASSERT_SAFE_MSG(atom_pin, "Intra-cluster sink must have an associated atom pin");
                VTR_ASSERT_SAFE_MSG(atom_ctx.netlist().pin_type(atom_pin) == PinType::SINK, "Intra-cluster Sink RR must be associated with a sink pin in atom netlist");
            }
        }
    }
    VTR_ASSERT_SAFE_MSG(num_extern_sinks >= 0 && num_extern_sinks <= 1, "Net must have at most one external sink");
    VTR_ASSERT_SAFE_MSG(num_extern_sources >= 0 && num_extern_sources <= 1, "Net must have at most one external source");
#endif
}

/* Given a pin of a net, remove route tree terminals from it
 */
void ClusterRouter::remove_pin_from_rt_terminals_(const AtomPinId pin_id,
                                                  const AtomPBBimap& atom_to_pb) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();
    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;

    const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_netlist, atom_to_pb, pin_id);

    AtomPortId port_id = atom_netlist.pin_port(pin_id);
    AtomNetId net_id = atom_netlist.pin_net(pin_id);

    if (!net_id) {
        /* This is not a valid net */
        return;
    }

    /* Find current net in route tree
     * Code assumes that # of nets in cluster is small so a linear search through vector is faster than using more complex data structures
     */
    bool found = false;
    unsigned int ipos;
    for (ipos = 0; ipos < intra_lb_nets_.size(); ipos++) {
        if (intra_lb_nets_[ipos].atom_net_id == net_id) {
            found = true;
            break;
        }
    }
    VTR_ASSERT(found == true);
    VTR_ASSERT(intra_lb_nets_[ipos].atom_net_id == net_id);

    VTR_ASSERT(intra_lb_nets_[ipos].atom_pins.size() == intra_lb_nets_[ipos].terminals.size());

    auto port_type = atom_netlist.port_type(port_id);
    if (port_type == PortType::OUTPUT) {
        /* Net driver pin takes 0th position in terminals */
        int sink_terminal;
        VTR_ASSERT(intra_lb_nets_[ipos].terminals[0] == pb_graph_pin->pin_count_in_cluster);
        intra_lb_nets_[ipos].terminals[0] = get_lb_type_rr_graph_ext_source_index(lb_type_);

        /* source terminal is now coming from outside logic block, do not need to route signal out of logic block */
        sink_terminal = get_lb_type_rr_graph_ext_sink_index(lb_type_);
        if (intra_lb_nets_[ipos].terminals[1] == sink_terminal) {
            intra_lb_nets_[ipos].terminals[1] = intra_lb_nets_[ipos].terminals.back();
            intra_lb_nets_[ipos].terminals.pop_back();

            intra_lb_nets_[ipos].atom_pins[1] = intra_lb_nets_[ipos].atom_pins.back();
            intra_lb_nets_[ipos].atom_pins.pop_back();
        }
    } else {
        VTR_ASSERT(port_type == PortType::INPUT || port_type == PortType::CLOCK);

        /* Remove sink from list of terminals */
        int pin_index = pb_graph_pin->pin_count_in_cluster;
        size_t term_idx;

        VTR_ASSERT(lb_type_graph[pin_index].num_modes == 1);
        VTR_ASSERT(lb_type_graph[pin_index].num_fanout[0] == 1);
        int sink_index = lb_type_graph[pin_index].outedges[0][0].node_index;
        VTR_ASSERT(lb_type_graph[sink_index].type == LB_SINK);

        int target_index = -1;
        //Search for the sink
        found = false;
        for (term_idx = 0; term_idx < intra_lb_nets_[ipos].terminals.size(); term_idx++) {
            if (intra_lb_nets_[ipos].terminals[term_idx] == sink_index) {
                target_index = sink_index;
                found = true;
                break;
            }
        }
        if (!found) {
            //Search for the pin
            found = false;
            for (term_idx = 0; term_idx < intra_lb_nets_[ipos].terminals.size(); term_idx++) {
                if (intra_lb_nets_[ipos].terminals[term_idx] == pin_index) {
                    target_index = pin_index;
                    found = true;
                    break;
                }
            }
        }
        VTR_ASSERT(found == true);
        VTR_ASSERT(intra_lb_nets_[ipos].terminals[term_idx] == target_index);

        /* Drop terminal from list */
        intra_lb_nets_[ipos].terminals[term_idx] = intra_lb_nets_[ipos].terminals.back();
        intra_lb_nets_[ipos].terminals.pop_back();

        intra_lb_nets_[ipos].atom_pins[term_idx] = intra_lb_nets_[ipos].atom_pins.back();
        intra_lb_nets_[ipos].atom_pins.pop_back();

        if (intra_lb_nets_[ipos].terminals.size() == 1 && intra_lb_nets_[ipos].terminals[0] != get_lb_type_rr_graph_ext_source_index(lb_type_)) {
            /* The removed sink must be driven by an atom found in the cluster, add in special sink outside of cluster to represent this */
            intra_lb_nets_[ipos].terminals.push_back(get_lb_type_rr_graph_ext_sink_index(lb_type_));
            intra_lb_nets_[ipos].atom_pins.push_back(AtomPinId::INVALID());
        }

        if (intra_lb_nets_[ipos].terminals.size() > 1 && intra_lb_nets_[ipos].terminals[1] != get_lb_type_rr_graph_ext_sink_index(lb_type_) && intra_lb_nets_[ipos].terminals[0] != get_lb_type_rr_graph_ext_source_index(lb_type_)) {
            /* The removed sink must be driven by an atom found in the cluster, add in special sink outside of cluster to represent this */
            int terminal = intra_lb_nets_[ipos].terminals[1];
            intra_lb_nets_[ipos].terminals.push_back(terminal);
            intra_lb_nets_[ipos].terminals[1] = get_lb_type_rr_graph_ext_sink_index(lb_type_);

            AtomPinId pin = intra_lb_nets_[ipos].atom_pins[1];
            intra_lb_nets_[ipos].atom_pins.push_back(pin);
            intra_lb_nets_[ipos].atom_pins[1] = AtomPinId::INVALID();
        }
    }
    VTR_ASSERT(intra_lb_nets_[ipos].atom_pins.size() == intra_lb_nets_[ipos].terminals.size());

    if (intra_lb_nets_[ipos].terminals.size() == 1 && intra_lb_nets_[ipos].terminals[0] == get_lb_type_rr_graph_ext_source_index(lb_type_)) {
        /* This net is not routed, remove from list of nets in lb */
        intra_lb_nets_[ipos] = intra_lb_nets_.back();
        intra_lb_nets_.pop_back();
    }
}

//It is possible that a net may connect multiple times to a logically equivalent set of primitive pins.
//The cluster router will only route one connection for a particular net to the common sink of the
//equivalent pins.
//
//To work around this, we fix all but one of these duplicate connections to route to specific pins,
//(instead of the common sink). This ensures a legal routing is produced and that the duplicate pins
//are not 'missing' in the clustered netlist.
void ClusterRouter::fix_duplicate_equivalent_pins_(const AtomPBBimap& atom_to_pb) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();

    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;

    for (size_t ilb_net = 0; ilb_net < intra_lb_nets_.size(); ++ilb_net) {
        //Collect all the sink terminals indices which target a particular node
        std::map<int, std::vector<int>> duplicate_terminals;
        for (size_t term_idx = 1; term_idx < intra_lb_nets_[ilb_net].terminals.size(); ++term_idx) {
            int node = intra_lb_nets_[ilb_net].terminals[term_idx];

            duplicate_terminals[node].push_back(term_idx);
        }

        for (auto kv : duplicate_terminals) {
            if (kv.second.size() < 2) continue; //Only process duplicates

            //Remap all the duplicate terminals so they target the pin instead of the sink
            for (size_t idup_term = 0; idup_term < kv.second.size(); ++idup_term) {
                int term_idx = kv.second[idup_term]; //The index in terminals which is duplicated

                VTR_ASSERT(intra_lb_nets_[ilb_net].atom_pins.size() == intra_lb_nets_[ilb_net].terminals.size());
                AtomPinId atom_pin = intra_lb_nets_[ilb_net].atom_pins[term_idx];
                VTR_ASSERT(atom_pin);

                const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_netlist, atom_to_pb, atom_pin);
                VTR_ASSERT(pb_graph_pin);

                if (pb_graph_pin->port->equivalent == PortEquivalence::NONE) continue; //Only need to remap equivalent ports

                //Remap this terminal to an explicit pin instead of the common sink
                int pin_index = pb_graph_pin->pin_count_in_cluster;

                VTR_LOG_WARN(
                    "Found duplicate nets connected to logically equivalent pins. "
                    "Remapping intra lb net %d (atom net %zu '%s') from common sink "
                    "pb_route %d to fixed pin pb_route %d\n",
                    ilb_net, size_t(intra_lb_nets_[ilb_net].atom_net_id), atom_netlist.net_name(intra_lb_nets_[ilb_net].atom_net_id).c_str(),
                    kv.first, pin_index);

                VTR_ASSERT(lb_type_graph[pin_index].type == LB_INTERMEDIATE);
                VTR_ASSERT(lb_type_graph[pin_index].num_fanout[0] == 1);
                int sink_index = lb_type_graph[pin_index].outedges[0][0].node_index;
                VTR_ASSERT(lb_type_graph[sink_index].type == LB_SINK);
                VTR_ASSERT_MSG(sink_index == intra_lb_nets_[ilb_net].terminals[term_idx], "Remapped pin must be connected to original sink");

                //Change the target
                intra_lb_nets_[ilb_net].terminals[term_idx] = pin_index;
            }
        }
    }
}

/* Commit or remove route tree from currently routed solution */
void ClusterRouter::commit_remove_rt_(const t_lb_trace& rt,
                                      e_commit_remove op,
                                      std::unordered_map<const t_pb_graph_node*, const t_mode*>& mode_map,
                                      t_mode_selection_status* mode_status) {
    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;

    int inode = rt.current_node;

    /* Determine if node is being used or removed */
    int incr;
    if (op == RT_COMMIT) {
        incr = 1;
        if (lb_rr_node_stats_[inode].occ >= lb_type_graph[inode].capacity) {
            lb_rr_node_stats_[inode].historical_usage += (lb_rr_node_stats_[inode].occ - lb_type_graph[inode].capacity + 1); /* store historical overuse */
        }
    } else {
        incr = -1;
        explored_node_tb_[inode].inet = UNDEFINED;
    }

    lb_rr_node_stats_[inode].occ += incr;
    VTR_ASSERT(lb_rr_node_stats_[inode].occ >= 0);

    auto& driver_node = lb_type_graph[inode];
    auto* driver_pin = driver_node.pb_graph_pin;

    /* Recursively update route tree */
    for (size_t i = 0; i < rt.next_nodes.size(); i++) {
        // Check to see if there is no mode conflict between previous nets.
        // A conflict is present if there are differing modes between a pb_graph_node
        // and its children.
        if (op == RT_COMMIT && mode_status->try_expand_all_modes) {
            auto& node = lb_type_graph[rt.next_nodes[i].current_node];
            auto* pin = node.pb_graph_pin;

            if (check_edge_for_route_conflicts(mode_map, driver_pin, pin)) {
                mode_status->is_mode_conflict = true;
            }
        }

        commit_remove_rt_(rt.next_nodes[i], op, mode_map, mode_status);
    }
}

/* Should net be skipped?  If the net does not conflict with another net, then skip routing this net */
static bool is_skip_route_net(const t_lb_trace& rt,
                              const std::vector<t_lb_type_rr_node>& lb_type_graph,
                              const std::vector<t_lb_rr_node_stats>& lb_rr_node_stats) {
    int inode = rt.current_node;

    /* Determine if node is overused */
    if (lb_rr_node_stats[inode].occ > lb_type_graph[inode].capacity) {
        /* Conflict between this net and another net at this node, reroute net */
        return false;
    }

    /* Recursively check that rest of route tree does not have a conflict */
    for (size_t i = 0; i < rt.next_nodes.size(); i++) {
        if (!is_skip_route_net(rt.next_nodes[i], lb_type_graph, lb_rr_node_stats)) {
            return false;
        }
    }

    /* No conflict, this net's current route is legal, skip routing this net */
    return true;
}

/* At source mode as starting point to existing route tree */
void ClusterRouter::add_source_to_rt_(int inet) {
    VTR_ASSERT(intra_lb_nets_[inet].rt_tree.current_node == UNDEFINED);
    intra_lb_nets_[inet].rt_tree.current_node = intra_lb_nets_[inet].terminals[0];
}

/* Expand all nodes found in route tree into priority queue */
void ClusterRouter::expand_rt_(int inet) {
    VTR_ASSERT(pq_.empty());

    VTR_ASSERT(intra_lb_nets_[inet].rt_tree.current_node != UNDEFINED);
    expand_rt_rec_(intra_lb_nets_[inet].rt_tree, UNDEFINED, inet);
}

/* Expand all nodes found in route tree into priority queue recursively */
void ClusterRouter::expand_rt_rec_(const t_lb_trace& rt, int prev_index, int irt_net) {
    t_expansion_node enode;

    /* Perhaps should use a cost other than zero */
    enode.cost = 0;
    enode.node_index = rt.current_node;
    enode.prev_index = prev_index;
    pq_.push(enode);
    explored_node_tb_[enode.node_index].inet = irt_net;
    explored_node_tb_[enode.node_index].explored_id = UNDEFINED;
    explored_node_tb_[enode.node_index].enqueue_id = explore_id_index_;
    explored_node_tb_[enode.node_index].enqueue_cost = 0;
    explored_node_tb_[enode.node_index].prev_index = prev_index;

    for (unsigned int i = 0; i < rt.next_nodes.size(); i++) {
        expand_rt_rec_(rt.next_nodes[i], rt.current_node, irt_net);
    }
}

/* Expand all edges of an expantion node */
void ClusterRouter::expand_edges_(int mode,
                                  int cur_inode,
                                  float cur_cost,
                                  int net_fanout) {
    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;
    t_expansion_node enode;

    for (int iedge = 0; iedge < lb_type_graph[cur_inode].num_fanout[mode]; iedge++) {
        /* Init new expansion node */
        enode.prev_index = cur_inode;
        enode.node_index = lb_type_graph[cur_inode].outedges[mode][iedge].node_index;
        enode.cost = cur_cost;

        /* Determine incremental cost of using expansion node */
        int usage = lb_rr_node_stats_[enode.node_index].occ + 1 - lb_type_graph[enode.node_index].capacity;
        float incr_cost = lb_type_graph[enode.node_index].intrinsic_cost;
        incr_cost += lb_type_graph[cur_inode].outedges[mode][iedge].intrinsic_cost;
        incr_cost += params_.hist_fac * lb_rr_node_stats_[enode.node_index].historical_usage;
        if (usage > 0) {
            incr_cost *= (usage * pres_con_fac_);
        }

        /* Adjust cost so that higher fanout nets prefer higher fanout routing nodes while lower fanout nets prefer lower fanout routing nodes */
        float fanout_factor = 1.0;
        int next_mode = lb_rr_node_stats_[enode.node_index].mode;
        /* Assume first mode if a mode hasn't been forced. */
        if (next_mode == -1) {
            next_mode = 0;
        }
        if (lb_type_graph[enode.node_index].num_fanout[next_mode] > 1) {
            fanout_factor = 0.85 + (0.25 / net_fanout);
        } else {
            fanout_factor = 1.15 - (0.25 / net_fanout);
        }

        incr_cost *= fanout_factor;
        enode.cost = cur_cost + incr_cost;

        /* Add to queue if cost is lower than lowest cost path to this enode */
        if (explored_node_tb_[enode.node_index].enqueue_id == explore_id_index_) {
            if (enode.cost < explored_node_tb_[enode.node_index].enqueue_cost) {
                pq_.push(enode);
            }
        } else {
            explored_node_tb_[enode.node_index].enqueue_id = explore_id_index_;
            explored_node_tb_[enode.node_index].enqueue_cost = enode.cost;
            pq_.push(enode);
        }
    }
}

/* Expand all nodes found in route tree into priority queue */
void ClusterRouter::expand_node_(const t_expansion_node& exp_node, int net_fanout) {
    int cur_node = exp_node.node_index;
    float cur_cost = exp_node.cost;
    int mode = lb_rr_node_stats_[cur_node].mode;
    if (mode == -1) {
        mode = 0;
    }

    expand_edges_(mode, cur_node, cur_cost, net_fanout);
}

/* Expand all nodes using all possible modes found in route tree into priority queue */
void ClusterRouter::expand_node_all_modes_(const t_expansion_node& exp_node, int net_fanout) {
    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;

    int cur_inode = exp_node.node_index;
    float cur_cost = exp_node.cost;
    int cur_mode = lb_rr_node_stats_[cur_inode].mode;
    auto& node = lb_type_graph[cur_inode];
    auto* pin = node.pb_graph_pin;

    for (int mode = 0; mode < lb_type_graph[cur_inode].num_modes; mode++) {
        /* If a mode has been forced, only add edges from that mode, otherwise add edges from all modes. */
        if (cur_mode != -1 && mode != cur_mode) {
            continue;
        }

        /* Check whether a mode is illegal. If it is then the node will not be expanded */
        bool is_illegal = false;
        if (pin != nullptr) {
            auto* pb_graph_node = pin->parent_node;
            for (auto illegal_mode : pb_graph_node->illegal_modes) {
                if (mode == illegal_mode) {
                    is_illegal = true;
                    break;
                }
            }
        }

        if (is_illegal == true) {
            continue;
        }

        expand_edges_(mode, cur_inode, cur_cost, net_fanout);
    }
}

/* Add new path from existing route tree to target sink */
bool ClusterRouter::add_to_rt_(t_lb_trace& rt, int node_index, int irt_net) {
    std::vector<int> trace_forward;

    /* Store path all the way back to route tree */
    int rt_index = node_index;
    while (explored_node_tb_[rt_index].inet != irt_net) {
        trace_forward.push_back(rt_index);
        rt_index = explored_node_tb_[rt_index].prev_index;
        VTR_ASSERT(rt_index != UNDEFINED);
    }

    /* Find rt_index on the route tree */
    // FIXME: We should never get a pointer to an element in a vector.
    //        This algorithm should be rethought. Maybe create a queue of references.
    t_lb_trace* link_node = find_node_in_rt(rt, rt_index);
    if (link_node == nullptr) {
        VTR_LOG("Link node is nullptr. Routing impossible");
        return true;
    }

    /* Add path to root tree */
    while (!trace_forward.empty()) {
        int trace_index = trace_forward.back();
        t_lb_trace curr_node;
        curr_node.current_node = trace_index;
        link_node->next_nodes.push_back(curr_node);
        link_node = &link_node->next_nodes.back();
        trace_forward.pop_back();
    }

    return false;
}

/* Determine if a completed route is valid.  A successful route has no congestion (ie. no routing resource is used by two nets). */
bool ClusterRouter::is_route_success_() {
    std::vector<t_lb_type_rr_node>& lb_type_graph = *lb_type_graph_;

    for (size_t inode = 0; inode < lb_type_graph.size(); inode++) {
        if (lb_rr_node_stats_[inode].occ > lb_type_graph[inode].capacity) {
            return false;
        }
    }

    return true;
}

/* Given a route tree and an index of a node on the route tree, return a pointer to the trace corresponding to that index */
static t_lb_trace* find_node_in_rt(t_lb_trace& rt, int rt_index) {
    if (rt.current_node == rt_index) {
        return &rt;
    } else {
        for (unsigned int i = 0; i < rt.next_nodes.size(); i++) {
            t_lb_trace* cur = find_node_in_rt(rt.next_nodes[i], rt_index);
            if (cur != nullptr) {
                return cur;
            }
        }
    }
    return nullptr;
}

#ifdef PRINT_INTRA_LB_ROUTE
/* Debug routine, print out current intra logic block route */
static void print_route(const char* filename, t_lb_router_data* router_data) {
    FILE* fp;
    std::vector<t_lb_type_rr_node>& lb_type_graph = *router_data->lb_type_graph;

    fp = fopen(filename, "w");
    for (unsigned int inode = 0; inode < lb_type_graph.size(); inode++) {
        fprintf(fp, "node %d occ %d cap %d\n", inode, router_data->lb_rr_node_stats[inode].occ, lb_type_graph[inode].capacity);
    }

    print_route(fp, router_data);
    fclose(fp);
}

static void print_route(FILE* fp, t_lb_router_data* router_data) {
    std::vector<t_intra_lb_net>& lb_nets = *router_data->intra_lb_nets;
    fprintf(fp, "\n\n----------------------------------------------------\n\n");

    auto& atom_ctx = g_vpr_ctx.atom();

    for (unsigned int inet = 0; inet < lb_nets.size(); inet++) {
        AtomNetId net_id = lb_nets[inet].atom_net_id;
        fprintf(fp, "net %s num targets %d \n", atom_ctx.netlist().net_name(net_id).c_str(), (int)lb_nets[inet].terminals.size());
        fprintf(fp, "\tS");
        print_trace(fp, lb_nets[inet].rt_tree, router_data);
        fprintf(fp, "\n\n");
    }
}
#endif

/* Debug routine, print out trace of net */
void ClusterRouter::print_trace_(FILE* fp, const t_lb_trace& trace) {
    if (trace.current_node == UNDEFINED) {
        fprintf(fp, "NULL");
        return;
    }
    for (unsigned int ibranch = 0; ibranch < trace.next_nodes.size(); ibranch++) {
        auto current_node = trace.current_node;
        auto current_str = describe_lb_type_rr_node_(current_node);
        auto next_node = trace.next_nodes[ibranch].current_node;
        auto next_str = describe_lb_type_rr_node_(next_node);
        if (trace.next_nodes.size() > 1) {
            fprintf(fp, "\n\tB");
        }
        fprintf(fp, "(%d:%s-->%d:%s) ", current_node, current_str.c_str(), next_node, next_str.c_str());
        print_trace_(fp, trace.next_nodes[ibranch]);
    }
}

void ClusterRouter::reset_explored_node_tb_() {
    for (unsigned int inode = 0; inode < lb_type_graph_->size(); inode++) {
        explored_node_tb_[inode].prev_index = UNDEFINED;
        explored_node_tb_[inode].explored_id = UNDEFINED;
        explored_node_tb_[inode].inet = UNDEFINED;
        explored_node_tb_[inode].enqueue_id = UNDEFINED;
        explored_node_tb_[inode].enqueue_cost = 0;
    }
}

/* Save last successful intra-logic block route and reset current lb_traceback */
void ClusterRouter::save_and_reset_lb_route_() {
    /* Free old saved lb nets if exist */
    if (!saved_lb_nets_.empty()) {
        saved_lb_nets_.clear();
    }

    /* Save current routed solution */
    saved_lb_nets_.resize(intra_lb_nets_.size());
    for (size_t inet = 0; inet < saved_lb_nets_.size(); inet++) {
        /*
         * Save and reset route tree data
         */
        saved_lb_nets_[inet].atom_net_id = intra_lb_nets_[inet].atom_net_id;
        saved_lb_nets_[inet].terminals.resize(intra_lb_nets_[inet].terminals.size());
        for (int term_idx = 0; term_idx < (int)intra_lb_nets_[inet].terminals.size(); term_idx++) {
            saved_lb_nets_[inet].terminals[term_idx] = intra_lb_nets_[inet].terminals[term_idx];
        }
        saved_lb_nets_[inet].rt_tree = std::move(intra_lb_nets_[inet].rt_tree);
        reset_lb_net_rt(intra_lb_nets_[inet].rt_tree);
    }
}

static std::vector<int> find_congested_rr_nodes(const std::vector<t_lb_type_rr_node>& lb_type_graph,
                                                const std::vector<t_lb_rr_node_stats>& lb_rr_node_stats) {
    std::vector<int> congested_rr_nodes;
    for (size_t inode = 0; inode < lb_type_graph.size(); ++inode) {
        const t_lb_type_rr_node& rr_node = lb_type_graph[inode];
        const t_lb_rr_node_stats& rr_node_stats = lb_rr_node_stats[inode];

        if (rr_node_stats.occ > rr_node.capacity) {
            congested_rr_nodes.push_back(inode);
        }
    }

    return congested_rr_nodes;
}

std::string ClusterRouter::describe_lb_type_rr_node_(int inode) {
    std::string description;

    const t_lb_type_rr_node& rr_node = (*lb_type_graph_)[inode];
    const t_pb_graph_pin* pb_graph_pin = rr_node.pb_graph_pin;

    if (pb_graph_pin) {
        description += "'" + pb_graph_pin->to_string(false) + "'";
    } else if (inode == get_lb_type_rr_graph_ext_source_index(lb_type_)) {
        VTR_ASSERT(rr_node.type == LB_SOURCE);
        description = "cluster-external source (LB_SOURCE)";
    } else if (inode == get_lb_type_rr_graph_ext_sink_index(lb_type_)) {
        VTR_ASSERT(rr_node.type == LB_SINK);
        description = "cluster-external sink (LB_SINK)";
    } else if (rr_node.type == LB_SINK) {
        description = "cluster-internal sink (LB_SINK accessible via architecture pins: ";

        //To account for equivalent pins multiple pins may route to a single sink.
        //As a result we need to fin all the nodes which connect to this sink in order
        //to give user-friendly pin names
        std::vector<std::string> pin_descriptions;
        std::vector<int> pin_rrs = find_incoming_rr_nodes(inode, *lb_type_graph_);
        for (int pin_rr_idx : pin_rrs) {
            const t_pb_graph_pin* pin_pb_gpin = (*lb_type_graph_)[pin_rr_idx].pb_graph_pin;
            pin_descriptions.push_back(pin_pb_gpin->to_string());
        }

        description += vtr::join(pin_descriptions, ", ");
        description += ")";

    } else if (rr_node.type == LB_SOURCE) {
        description = "cluster-internal source (LB_SOURCE)";
    } else if (rr_node.type == LB_INTERMEDIATE) {
        description = "cluster-internal intermediate?";
    } else {
        description = "<unknown lb_type_rr_node>";
    }

    return description;
}

static std::vector<int> find_incoming_rr_nodes(int dst_node, const std::vector<t_lb_type_rr_node>& lb_rr_graph) {
    std::vector<int> incoming_rr_nodes;
    for (size_t inode = 0; inode < lb_rr_graph.size(); ++inode) {
        const t_lb_type_rr_node& rr_node = lb_rr_graph[inode];
        for (int mode = 0; mode < rr_node.num_modes; mode++) {
            for (int iedge = 0; iedge < rr_node.num_fanout[mode]; ++iedge) {
                const t_lb_type_rr_node_edge& rr_edge = rr_node.outedges[mode][iedge];

                if (rr_edge.node_index == dst_node) {
                    //The current node connects to the destination node
                    incoming_rr_nodes.push_back(inode);
                }
            }
        }
    }
    return incoming_rr_nodes;
}

std::string ClusterRouter::describe_congested_rr_nodes_(const std::vector<int>& congested_rr_nodes) {
    std::string description;

    const auto& lb_type_graph = *lb_type_graph_;

    std::multimap<size_t, AtomNetId> congested_rr_node_to_nets; //From rr_node to net
    for (unsigned int inet = 0; inet < intra_lb_nets_.size(); inet++) {
        AtomNetId atom_net = intra_lb_nets_[inet].atom_net_id;

        //Walk the lb_traceback to find congested RR nodes for each net
        std::queue<t_lb_trace> q;

        if (intra_lb_nets_[inet].rt_tree.current_node != UNDEFINED) {
            q.push(intra_lb_nets_[inet].rt_tree);
        }
        while (!q.empty()) {
            t_lb_trace curr = q.front();
            q.pop();

            for (const t_lb_trace& next_trace : curr.next_nodes) {
                q.push(next_trace);
            }

            int inode = curr.current_node;
            const t_lb_type_rr_node& rr_node = lb_type_graph[inode];
            const t_lb_rr_node_stats& rr_node_stats = lb_rr_node_stats_[inode];

            if (rr_node_stats.occ > rr_node.capacity) {
                //Congested
                congested_rr_node_to_nets.insert({inode, atom_net});
            }
        }
    }

    VTR_ASSERT(!congested_rr_node_to_nets.empty());
    VTR_ASSERT(!congested_rr_nodes.empty());
    auto& atom_ctx = g_vpr_ctx.atom();
    for (int inode : congested_rr_nodes) {
        const t_lb_type_rr_node& rr_node = lb_type_graph[inode];
        const t_lb_rr_node_stats& rr_node_stats = lb_rr_node_stats_[inode];
        description += vtr::string_fmt("RR Node %d (%s) is congested (occ: %d > capacity: %d) with the following nets:\n",
                                       inode,
                                       describe_lb_type_rr_node_(inode).c_str(),
                                       rr_node_stats.occ,
                                       rr_node.capacity);
        auto range = congested_rr_node_to_nets.equal_range(inode);
        for (auto itr = range.first; itr != range.second; ++itr) {
            AtomNetId net = itr->second;
            description += vtr::string_fmt("\tNet: %s\n",
                                           atom_ctx.netlist().net_name(net).c_str());
        }
    }

    return description;
}

void ClusterRouter::reset_intra_lb_route() {
    for (auto& node : *lb_type_graph_) {
        auto* pin = node.pb_graph_pin;
        if (pin == nullptr) {
            continue;
        }
        VTR_ASSERT(pin->parent_node != nullptr);
        pin->parent_node->illegal_modes.clear();
    }
}
