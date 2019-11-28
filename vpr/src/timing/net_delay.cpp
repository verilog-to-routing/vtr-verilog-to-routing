#include <cstdio>

#include "vtr_memory.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "net_delay.h"
#include "route_tree_timing.h"

/* This module keeps track of the time delays for signals to arrive at       *
 * each pin in every net after timing-driven routing is complete. It         *
 * achieves this by first constructing the skeleton route tree               * 
 * from the traceback. Next, to obtain the completed route tree, it          *
 * calls functions to calculate the resistance, capacitance, and time        *
 * delays associated with each node. Then, it recursively traverses          *
 * the tree, copying the time delay from each node into the                  *
 * net_delay data structure. Essentially, the delays are calculated          *
 * by building the completed route tree from scratch, and is used            *
 * to check against the time delays computed incrementally during           * 
 * timing-driven routing.                                                    */

/********************** Variables local to this module ***********************/

/* Unordered map below stores the pair whose key is the index of the rr_node *
 * that corresponds to the rt_node, and whose value is the time delay        *
 * associated with that node. The map will be used to store delays while     *
 * traversing the nodes of the route tree in load_one_net_delay_recurr.      */

static std::unordered_map<int, float> inode_to_Tdel_map;

/*********************** Subroutines local to this module ********************/

static void load_one_net_delay(vtr::vector<ClusterNetId, float*>& net_delay, ClusterNetId net_id);

static void load_one_net_delay_recurr(t_rt_node* node, ClusterNetId net_id);

static void load_one_constant_net_delay(vtr::vector<ClusterNetId, float*>& net_delay, ClusterNetId net_id, float delay_value);

/*************************** Subroutine definitions **************************/

/* Allocates space for the net_delay data structure   *
 * [0..nets.size()-1][1..num_pins-1]. I chunk the data *
 * to save space on large problems.                    */
vtr::vector<ClusterNetId, float*> alloc_net_delay(vtr::t_chunk* chunk_list_ptr) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    vtr::vector<ClusterNetId, float*> net_delay; /* [0..nets.size()-1][1..num_pins-1] */

    auto nets = cluster_ctx.clb_nlist.nets();
    net_delay.resize(nets.size());

    for (auto net_id : nets) {
        float* tmp_ptr = (float*)vtr::chunk_malloc(cluster_ctx.clb_nlist.net_sinks(net_id).size() * sizeof(float), chunk_list_ptr);

        net_delay[net_id] = tmp_ptr - 1; /* [1..num_pins-1] */

        //Ensure the net delays are initialized with non-garbage values
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            net_delay[net_id][ipin] = std::numeric_limits<float>::quiet_NaN();
        }
    }

    return (net_delay);
}

void free_net_delay(vtr::vector<ClusterNetId, float*>& net_delay,
                    vtr::t_chunk* chunk_list_ptr) {
    net_delay.clear();
    vtr::free_chunk_memory(chunk_list_ptr);
}

void load_net_delay_from_routing(vtr::vector<ClusterNetId, float*>& net_delay) {
    /* This routine loads net_delay[0..nets.size()-1][1..num_pins-1].  Each entry   *
     * is the Elmore delay from the net source to the appropriate sink. Both       *
     * the rr_graph and the routing traceback must be completely constructed        *
     * before this routine is called, and the net_delay array must have been        *
     * allocated.                                                                   */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            load_one_constant_net_delay(net_delay, net_id, 0.);
        } else {
            load_one_net_delay(net_delay, net_id);
        }
    }
}

static void load_one_net_delay(vtr::vector<ClusterNetId, float*>& net_delay, ClusterNetId net_id) {
    /* This routine loads delay values for one net in                            *
     * net_delay[net_id][1..num_pins-1]. First, from the traceback, it           *
     * constructs the route tree and computes its values for R, C, and Tdel.     *
     * Next, it walks the route tree recursively, storing the time delays for    * 
     * each node into the map inode_to_Tdel. Then, while looping through the     *
     * net_delay array we search for the inode corresponding to the pin          * 
     * identifiers, and correspondingly update the entry in net_delay.           *
     * Finally, it frees the route tree and clears the inode_to_Tdel_map         *
     * associated with that net.                                                 */

    auto& route_ctx = g_vpr_ctx.routing();

    if (route_ctx.trace[net_id].head == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_TIMING,
                        "in load_one_net_delay: Traceback for net %lu does not exist.\n", size_t(net_id));
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();
    int inode;

    t_rt_node* rt_root = traceback_to_route_tree(net_id); // obtain the root of the tree constructed from the traceback
    load_new_subtree_R_upstream(rt_root);                 // load in the resistance values for the route tree
    load_new_subtree_C_downstream(rt_root);               // load in the capacitance values for the route tree
    load_route_tree_Tdel(rt_root, 0.);                    // load the time delay values for the route tree
    load_one_net_delay_recurr(rt_root, net_id);           // recursively traverse the tree and load entries into the inode_to_Tdel map

    for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
        inode = route_ctx.net_rr_terminals[net_id][ipin]; // look for the index of the rr node that corresponds to the sink that was used to route a certain connection.
        auto itr = inode_to_Tdel_map.find(inode);
        VTR_ASSERT(itr != inode_to_Tdel_map.end());

        net_delay[net_id][ipin] = itr->second; // search for the value of Tdel in the inode map and load into net_delay
    }
    free_route_tree(rt_root);  // free the route tree
    inode_to_Tdel_map.clear(); // clear the map
}

static void load_one_net_delay_recurr(t_rt_node* node, ClusterNetId net_id) {
    /* This routine recursively traverses the route tree, and copies the Tdel of the node into the map.  */

    inode_to_Tdel_map[node->inode] = node->Tdel; // add to the map, process current node

    for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) { // process children
        load_one_net_delay_recurr(edge->child, net_id);
    }
}

static void load_one_constant_net_delay(vtr::vector<ClusterNetId, float*>& net_delay, ClusterNetId net_id, float delay_value) {
    /* Sets each entry of the net_delay array for net inet to delay_value.     */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++)
        net_delay[net_id][ipin] = delay_value;
}
