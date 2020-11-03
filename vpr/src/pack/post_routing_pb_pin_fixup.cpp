/********************************************************************
 * This file includes functions to fix up the pb pin mapping results 
 * after routing optimization
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_time.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_error.h"
#include "vpr_utils.h"
#include "rr_graph2.h"

#include "annotate_routing.h"

#include "post_routing_pb_pin_fixup.h"

/* Include global variables of VPR */
#include "globals.h"

/********************************************************************
 * Give a given pin index, find the side where this pin is located 
 * on the physical tile
 * Note:
 *   - Need to check if the pin_width_offset and pin_height_offset
 *     are properly set in VPR!!!
 *******************************************************************/
static std::vector<e_side> find_physical_tile_pin_side(t_physical_tile_type_ptr physical_tile,
                                                       const int& physical_pin) {
    std::vector<e_side> pin_sides;
    for (const e_side& side_cand : {TOP, RIGHT, BOTTOM, LEFT}) {
        int pin_width_offset = physical_tile->pin_width_offset[physical_pin];
        int pin_height_offset = physical_tile->pin_height_offset[physical_pin];
        if (physical_tile->pinloc[pin_width_offset][pin_height_offset][side_cand][physical_pin]) {
            pin_sides.push_back(side_cand);
        }
    }

    return pin_sides;
}

/********************************************************************
 * Fix up the pb pin mapping results for a given clustered block
 * 1. For each input/output pin of a clustered pb, 
 *    - find a corresponding node in RRGraph object
 *    - find the net id for the node in routing context
 *    - find the net id for the node in clustering context
 *    - if the net id does not match, we update the clustering context
 *******************************************************************/
static void update_cluster_pin_with_post_routing_results(const DeviceContext& device_ctx,
                                                         ClusteringContext& clustering_ctx,
                                                         const vtr::vector<RRNodeId, ClusterNetId>& rr_node_nets,
                                                         const vtr::Point<size_t>& grid_coord,
                                                         const ClusterBlockId& blk_id,
                                                         const int& sub_tile_z,
                                                         size_t& num_mismatches,
                                                         const bool& verbose) {
    /* Handle each pin */
    auto logical_block = clustering_ctx.clb_nlist.block_type(blk_id);
    auto physical_tile = device_ctx.grid[grid_coord.x()][grid_coord.y()].type;

    /* Narrow down side search for grids
     *   The wanted side depends on the location of the grid.
     *   In particular for perimeter grid, 
     *   -------------------------------------------------------
     *   Grid location |  IPIN side
     *   -------------------------------------------------------
     *   TOP           |  BOTTOM     
     *   -------------------------------------------------------
     *   RIGHT         |  LEFT     
     *   -------------------------------------------------------
     *   BOTTOM        |  TOP   
     *   -------------------------------------------------------
     *   LEFT          |  RIGHT
     *   -------------------------------------------------------
     *   TOP-LEFT      |  BOTTOM & RIGHT
     *   -------------------------------------------------------
     *   TOP-RIGHT     |  BOTTOM & LEFT
     *   -------------------------------------------------------
     *   BOTTOM-LEFT   |  TOP & RIGHT
     *   -------------------------------------------------------
     *   BOTTOM-RIGHT  |  TOP & LEFT
     *   -------------------------------------------------------
     */
    std::vector<e_side> wanted_sides;
    if (device_ctx.grid.height() - 1 == grid_coord.y()) { /* TOP side */
        wanted_sides.push_back(BOTTOM);
    }
    if (device_ctx.grid.width() - 1 == grid_coord.x()) { /* RIGHT side */
        wanted_sides.push_back(LEFT);
    }
    if (0 == grid_coord.y()) { /* BOTTOM side */
        wanted_sides.push_back(TOP);
    }
    if (0 == grid_coord.x()) { /* LEFT side */
        wanted_sides.push_back(RIGHT);
    }

    /* If wanted sides is empty still, this block does not have specific wanted sides,
     * Deposit all the sides
     */
    if (wanted_sides.empty()) {
        for (e_side side : {TOP, BOTTOM, LEFT, RIGHT}) {
            wanted_sides.push_back(side);
        }
    }

    for (int pb_type_pin = 0; pb_type_pin < logical_block->pb_type->num_pins; ++pb_type_pin) {
        /* Skip non-equivalent ports, no need to do fix-up */
        const t_pb_graph_pin* pb_graph_pin = get_pb_graph_node_pin_from_block_pin(blk_id, pb_type_pin);
        if (PortEquivalence::FULL != pb_graph_pin->port->equivalent) {
            continue;
        }

        /* Get the ptc num for the pin in rr_graph, we need to consider the sub tile offset here
         * sub tile offset is the location in a sub tile whose capacity is larger than zero
         */
        int physical_pin = get_post_placement_physical_pin(physical_tile, logical_block, sub_tile_z, pb_type_pin);
        VTR_ASSERT(physical_pin < physical_tile->num_pins);

        auto pin_class = physical_tile->pin_class[physical_pin];
        auto class_inf = physical_tile->class_inf[pin_class];

        t_rr_type rr_node_type;
        if (class_inf.type == DRIVER) {
            rr_node_type = OPIN;
        } else {
            VTR_ASSERT(class_inf.type == RECEIVER);
            rr_node_type = IPIN;
        }

        std::vector<e_side> pinloc_sides = find_physical_tile_pin_side(physical_tile, physical_pin);
        /* As some grid has height/width offset, we may not have the pin on any side */
        if (0 == pinloc_sides.size()) {
            continue;
        }

        /* Merge common part of the pin_sides and the wanted sides,
         * which are sides we should iterate over
         */
        std::vector<e_side> pin_sides;
        for (const e_side& pinloc_side : pinloc_sides) {
            if (wanted_sides.end() != std::find(wanted_sides.begin(), wanted_sides.end(), pinloc_side)) {
                pin_sides.push_back(pinloc_side);
            }
        }
        /* We should have at least one side now after merging */
        VTR_ASSERT(!pin_sides.empty());

        ClusterNetId routing_net_id = ClusterNetId::INVALID();
        std::vector<RRNodeId> visited_rr_nodes;
        short valid_routing_net_cnt = 0;
        for (const e_side& pin_side : pin_sides) {
            /* Find the net mapped to this pin in routing results */
            const int& rr_node = get_rr_node_index(device_ctx.rr_node_indices,
                                                   grid_coord.x(), grid_coord.y(),
                                                   rr_node_type, physical_pin, pin_side);

            /* Bypass invalid nodes, after that we must have a valid rr_node id */
            if (OPEN == rr_node) {
                continue;
            }
            VTR_ASSERT((size_t)rr_node < device_ctx.rr_nodes.size());

            /* If the node has been visited on the other side, we just skip it */
            if (visited_rr_nodes.end() != std::find(visited_rr_nodes.begin(), visited_rr_nodes.end(), RRNodeId(rr_node))) {
                continue;
            }

            /* Get the cluster net id which has been mapped to this net
             * In general, there is only one valid rr_node among all the sides.
             * However, we have an exception in the Stratix-IV arch modeling,
             * where a pb_pin may exist in two different sides but
             * router will only map to 1 rr_node 
             * Therefore, it is better to compare the routing nets
             * for all the sides and pick
             * - The unique valid net id (others should be all invalid)
             *   assume that this pin is used by router
             * - A invalid net id (others should be all invalid as well)
             *   assume that this pin is not used by router
             */
            if (rr_node_nets[RRNodeId(rr_node)]) {
                if (routing_net_id) {
                    if (routing_net_id != rr_node_nets[RRNodeId(rr_node)]) {
                        VTR_LOG_ERROR("Pin '%s' is mapped to two nets: '%s' and '%s'\n",
                                      pb_graph_pin->to_string().c_str(),
                                      clustering_ctx.clb_nlist.net_name(routing_net_id).c_str(),
                                      clustering_ctx.clb_nlist.net_name(rr_node_nets[RRNodeId(rr_node)]).c_str());
                    }
                    VTR_ASSERT(routing_net_id == rr_node_nets[RRNodeId(rr_node)]);
                }
                routing_net_id = rr_node_nets[RRNodeId(rr_node)];
                valid_routing_net_cnt++;
                visited_rr_nodes.push_back(RRNodeId(rr_node));
            }
        }

        VTR_ASSERT((0 == valid_routing_net_cnt) || (1 == valid_routing_net_cnt));

        /* Find the net mapped to this pin in clustering results*/
        ClusterNetId cluster_net_id = clustering_ctx.clb_nlist.block_net(blk_id, pb_type_pin);

        /* Ignore used in local cluster only, reserved one CLB pin */
        if ((clustering_ctx.clb_nlist.valid_net_id(cluster_net_id))
            && (0 == clustering_ctx.clb_nlist.net_sinks(cluster_net_id).size())) {
            continue;
        }

        /* If the net from the routing results matches the net from the packing results,
         * nothing to be changed. Move on to the next net.
         */
        if (routing_net_id == cluster_net_id) {
            continue;
        }

        /* Update the clustering context with net modification */
        clustering_ctx.post_routing_clb_pin_nets[blk_id][pb_graph_pin->pin_count_in_cluster] = routing_net_id;

        std::string routing_net_name("unmapped");
        if (clustering_ctx.clb_nlist.valid_net_id(routing_net_id)) {
            routing_net_name = clustering_ctx.clb_nlist.net_name(routing_net_id);
        }

        std::string cluster_net_name("unmapped");
        if (clustering_ctx.clb_nlist.valid_net_id(cluster_net_id)) {
            cluster_net_name = clustering_ctx.clb_nlist.net_name(cluster_net_id);
        }

        VTR_LOGV(verbose,
                 "Fixed up net '%s' mapping mismatch at clustered block '%s' pin 'grid[%ld][%ld].%s.%s[%d]' (was net '%s')\n",
                 routing_net_name.c_str(),
                 clustering_ctx.clb_nlist.block_pb(blk_id)->name,
                 grid_coord.x(), grid_coord.y(),
                 clustering_ctx.clb_nlist.block_pb(blk_id)->pb_graph_node->pb_type->name,
                 get_pb_graph_node_pin_from_block_pin(blk_id, physical_pin)->port->name,
                 get_pb_graph_node_pin_from_block_pin(blk_id, physical_pin)->pin_number,
                 cluster_net_name.c_str());

        /* Update counter */
        num_mismatches++;
    }
}

/********************************************************************
 * Find an unused pb_route from the other pins in this port 
 * The pb_route should be remapped to an invalid net, becoming unused
 * at post routing stage.
 *
 * This function will return the first one we can find.
 *******************************************************************/
static int find_target_pb_route_from_equivalent_pins(const AtomContext& atom_ctx,
                                                     const ClusteringContext& clustering_ctx,
                                                     const ClusterBlockId& blk_id,
                                                     t_pb* pb,
                                                     const t_pb_graph_pin* source_pb_graph_pin,
                                                     const AtomNetId& target_net,
                                                     const bool& verbose) {
    VTR_ASSERT(source_pb_graph_pin->parent_node->is_root());

    std::vector<int> pb_route_indices;

    for (int pb_type_pin = 0; pb_type_pin < pb->pb_graph_node->pb_type->num_pins; ++pb_type_pin) {
        const t_pb_graph_pin* pb_graph_pin = get_pb_graph_node_pin_from_block_pin(blk_id, pb_type_pin);

        if (PortEquivalence::FULL != pb_graph_pin->port->equivalent) {
            continue;
        }

        /* Limitation: bypass output pins now
         * TODO: This is due to the 'instance' equivalence port 
         * where outputs may be swapped. This definitely requires re-run of packing
         * It can not be solved by swapping routing traces now
         */
        if (OUT_PORT == pb_graph_pin->port->type) {
            continue;
        }

        /* Sanity check to ensure the pb_graph_pin is the top-level */
        VTR_ASSERT(pb_graph_pin->parent_node == pb->pb_graph_node);
        VTR_ASSERT(pb_graph_pin->parent_node->is_root());

        int pin = pb_graph_pin->pin_count_in_cluster;

        /* Bypass unused pins */
        if ((0 == pb->pb_route.count(pin)) || (AtomNetId::INVALID() == pb->pb_route.at(pin).atom_net_id)) {
            continue;
        }

        auto remapped_result = clustering_ctx.post_routing_clb_pin_nets.at(blk_id).find(pin);

        /* Skip this pin if it is consistent in pre- and post- routing results */
        if (remapped_result == clustering_ctx.post_routing_clb_pin_nets.at(blk_id).end()) {
            continue;
        }

        /* Only care the pin has the same parent port as source_pb_pin */
        if (source_pb_graph_pin->port != pb->pb_route.at(pin).pb_graph_pin->port) {
            continue;
        }

        /* We can use the routing trace if it is mapped to the same net as the remapped result
         */
        if (target_net == pb->pb_route.at(pin).atom_net_id) {
            for (const int& sink_pb_route : pb->pb_route.at(pin).sink_pb_pin_ids) {
                VTR_ASSERT(pb->pb_route.at(sink_pb_route).atom_net_id == target_net);
            }

            pb_route_indices.push_back(pin);
        }
    }

    VTR_LOGV(verbose,
             "Found %lu candidates to remap net '%s' at clustered block '%s' pin '%s'\n",
             pb_route_indices.size(),
             atom_ctx.nlist.net_name(target_net).c_str(),
             clustering_ctx.clb_nlist.block_pb(blk_id)->name,
             source_pb_graph_pin->to_string().c_str());

    /* Should find at least 1 candidate */
    VTR_ASSERT(!pb_route_indices.empty());

    return pb_route_indices[0];
}

/********************************************************************
 * Find an unused (unrouted) pb_graph_pin that is in the same port as the given pin
 * NO optimization is done here!!! First find first fit
 *******************************************************************/
static const t_pb_graph_pin* find_unused_pb_graph_pin_in_the_same_port(const t_pb_graph_pin* pb_graph_pin,
                                                                       const t_pb_routes& pb_routes,
                                                                       const AtomNetId& mapped_net) {
    /* If the current pb has the right pb_route, we can return it directly */
    if ((0 < pb_routes.count(pb_graph_pin->pin_count_in_cluster))
        && (mapped_net == pb_routes.at(pb_graph_pin->pin_count_in_cluster).atom_net_id)) {
        return pb_graph_pin;
    }

    /* Otherwise, we have to find an unused pin from the same port */
    for (int ipin = 0; ipin < pb_graph_pin->port->num_pins; ++ipin) {
        const t_pb_graph_pin* candidate_pb_graph_pin = find_pb_graph_pin(pb_graph_pin->parent_node, std::string(pb_graph_pin->port->name), ipin);
        int cand_pb_route_id = candidate_pb_graph_pin->pin_count_in_cluster;

        /* If unused, we find it */
        if (0 == pb_routes.count(cand_pb_route_id)) {
            return candidate_pb_graph_pin;
        }
        /* If used but in the same net, we can reuse that */
        if (mapped_net == pb_routes.at(cand_pb_route_id).atom_net_id) {
            return candidate_pb_graph_pin;
        }
    }

    /* Not found: Print debugging information */
    for (int ipin = 0; ipin < pb_graph_pin->port->num_pins; ++ipin) {
        const t_pb_graph_pin* candidate_pb_graph_pin = find_pb_graph_pin(pb_graph_pin->parent_node, std::string(pb_graph_pin->port->name), ipin);
        int cand_pb_route_id = candidate_pb_graph_pin->pin_count_in_cluster;

        VTR_LOG("\tCandidate pin: '%s'",
                candidate_pb_graph_pin->to_string().c_str());

        if (0 == pb_routes.count(cand_pb_route_id)) {
            VTR_LOG("\tUnused\n");
        } else {
            VTR_LOG("\tmapped to net '%s'\n",
                    g_vpr_ctx.atom().nlist.net_name(pb_routes.at(cand_pb_route_id).atom_net_id).c_str());
        }
    }

    return nullptr;
}

/********************************************************************
 * Identify if this pb_graph pin only drives 1 edge and the edge size is 1
 *******************************************************************/
static bool is_single_fanout_pb_pin(const t_pb_graph_pin* pb_pin) {
    return (1 == pb_pin->num_output_edges) && (1 == pb_pin->output_edges[0]->num_output_pins);
}

/********************************************************************
 * Find the sink pb_graph_pin from a driver pb_graph_pin
 * which is the only one used in the fan-outs of the driver pb_graph_pin!
 *******************************************************************/
static t_pb_graph_pin* find_used_sink_pb_pin(t_pb_graph_pin* driver_pb_pin,
                                             const int& mode) {
    t_pb_graph_pin* sink_pb_pin = nullptr;
    int num_cand_pins = 0;

    for (int iedge = 0; iedge < driver_pb_pin->num_output_edges; ++iedge) {
        if (mode != driver_pb_pin->output_edges[iedge]->interconnect->parent_mode_index) {
            continue;
        }
        for (int ipin = 0; ipin < driver_pb_pin->output_edges[iedge]->num_output_pins; ++ipin) {
            sink_pb_pin = driver_pb_pin->output_edges[iedge]->output_pins[ipin];
            num_cand_pins++;
        }
    }

    /* If we see multiple pins, we should return null pointer, as we only expect 1 pin to be found! */
    if (1 < num_cand_pins) {
        return nullptr;
    }

    return sink_pb_pin;
}

/********************************************************************
 * Try to find the atom pin that is mapped to the sink pb_graph_pin
 * which can be traced back through the pb routing tree 
 *
 * Note:
 *   - This function assumes only 1 sink node to be found
 *   - All the pb_graph_pins in the routing tree should have only 1 fan-out
 *     in the selected mode!
 *
 *             Top-level pb_type
 *           +----------------------------
 *           |   +------------------------    
 *           |   |      Primitive pb_type
 *           |   |     +------------------
 *           |   |     |
 *   driver  +---+---->| sink pb_pin <------> atom_pin 
 *  pb_route |
 *     
 *******************************************************************/
static AtomPinId find_mapped_atom_pin(const AtomContext& atom_ctx,
                                      const IntraLbPbPinLookup& intra_lb_pb_pin_lookup,
                                      t_logical_block_type_ptr logical_type,
                                      t_pb* pb,
                                      const int& pb_route_id) {
    /* Walk through the routing tree until we reach the end */
    int sink_pb_route_id = pb_route_id;
    while (0 < pb->pb_route.count(sink_pb_route_id)) {
        /* We will only care about single fan-out routing trees,
         * their atom pin their could be remapped
         * If multiple fan-out exist in the paths,
         * return an invalid id showing not found!
         */
        if (1 != pb->pb_route.at(sink_pb_route_id).sink_pb_pin_ids.size()) {
            return AtomPinId::INVALID();
        }
        sink_pb_route_id = pb->pb_route.at(sink_pb_route_id).sink_pb_pin_ids[0];
        if (pb->pb_route.at(sink_pb_route_id).sink_pb_pin_ids.empty()) {
            break;
        }
    }

    const t_pb_graph_pin* sink_pb_pin = intra_lb_pb_pin_lookup.pb_gpin(logical_type->index, sink_pb_route_id);
    const t_pb* leaf_pb = pb->find_pb(sink_pb_pin->parent_node);
    const AtomPortId& atom_port = atom_ctx.nlist.find_atom_port(atom_ctx.lookup.pb_atom(leaf_pb), sink_pb_pin->port->model_port);
    BitIndex atom_pin_bit_index = leaf_pb->atom_pin_bit_index(sink_pb_pin);
    AtomPinId mapped_atom_pin = atom_ctx.nlist.port_pin(atom_port, atom_pin_bit_index);

    return mapped_atom_pin;
}

/********************************************************************
 * Cache the mapping from atom pin to pb_graph pin for remapped nets
 * in current routing traces
 *
 * Note: 
 *   - The pb_route_id is the routing trace id for top-level pins ONLY!
 *   - This is only applicable to the following case:
 *
 *             Top-level pb_type
 *           +----------------------------
 *           |   +------------------------    
 *           |   |      Primitive pb_type
 *           |   |     +------------------
 *           |   |     |
 *   driver  +---+---->| sink
 *           |
 *
 *     
 *     Anything violates the assumption will be NOT be cached!!!
 *******************************************************************/
static std::map<int, std::pair<AtomPinId, const t_pb_graph_pin*>> cache_atom_pin_to_pb_pin_mapping(const AtomContext& atom_ctx,
                                                                                                   const IntraLbPbPinLookup& intra_lb_pb_pin_lookup,
                                                                                                   const ClusteringContext& clustering_ctx,
                                                                                                   const ClusterBlockId& blk_id,
                                                                                                   t_pb* pb,
                                                                                                   t_logical_block_type_ptr logical_block) {
    std::map<int, std::pair<AtomPinId, const t_pb_graph_pin*>> atom_pin_to_pb_pin_mapping;
    for (int pb_type_pin = 0; pb_type_pin < logical_block->pb_type->num_pins; ++pb_type_pin) {
        /* Skip non-equivalent ports, no need to do fix-up */
        const t_pb_graph_pin* pb_graph_pin = get_pb_graph_node_pin_from_block_pin(blk_id, pb_type_pin);
        if (PortEquivalence::FULL != pb_graph_pin->port->equivalent) {
            continue;
        }

        if (OUT_PORT == pb_graph_pin->port->type) {
            continue;
        }

        /* Sanity check to ensure the pb_graph_pin is the top-level */
        VTR_ASSERT(pb_graph_pin->parent_node == pb->pb_graph_node);
        VTR_ASSERT(pb_graph_pin->parent_node->is_root());

        auto remapped_result = clustering_ctx.post_routing_clb_pin_nets.at(blk_id).find(pb_graph_pin->pin_count_in_cluster);

        /* Skip this pin: it is consistent in pre- and post- routing results */
        if (remapped_result == clustering_ctx.post_routing_clb_pin_nets.at(blk_id).end()) {
            continue;
        }

        int pb_route_id = pb_graph_pin->pin_count_in_cluster;
        auto pb_route_result = pb->pb_route.find(pb_route_id);
        if (pb->pb_route.end() == pb_route_result) {
            continue;
        }

        /* Only cache for single fan-out routing trees */
        if (1 != pb_route_result->second.sink_pb_pin_ids.size()) {
            continue;
        }

        const AtomPinId& orig_mapped_atom_pin = find_mapped_atom_pin(atom_ctx, intra_lb_pb_pin_lookup, logical_block, pb, pb_route_id);

        /* Sometimes the routing traces is not what we target, skip caching */
        if (orig_mapped_atom_pin) {
            atom_pin_to_pb_pin_mapping[pb_route_id] = std::make_pair(orig_mapped_atom_pin, atom_ctx.lookup.atom_pin_pb_graph_pin(orig_mapped_atom_pin));
        }
    }

    return atom_pin_to_pb_pin_mapping;
}

/********************************************************************
 * Spot the original pb_graph_pin that is mapped 
 * to a given net from the other equivalent pins in this port
 *******************************************************************/
static const t_pb_graph_pin* find_mapped_equivalent_pb_pin_by_net(t_pb* pb,
                                                                  const t_pb_graph_pin* remapped_pb_pin,
                                                                  const AtomNetId& atom_net_id) {
    t_pb_graph_node* pb_graph_node = remapped_pb_pin->parent_node;
    t_port* eq_port = remapped_pb_pin->port;
    VTR_ASSERT(IN_PORT == eq_port->type);

    int cnt = 0;
    const t_pb_graph_pin* found_pb_pin = nullptr;

    for (int ipin = 0; ipin < eq_port->num_pins; ++ipin) {
        const t_pb_graph_pin* cand_pb_pin = &(pb_graph_node->input_pins[eq_port->port_index_by_type][ipin]);
        VTR_ASSERT(eq_port == cand_pb_pin->port);
        auto pb_route_result = pb->pb_route.find(cand_pb_pin->pin_count_in_cluster);
        if (pb->pb_route.end() == pb_route_result) {
            continue;
        }
        if (atom_net_id == pb_route_result->second.atom_net_id) {
            found_pb_pin = cand_pb_pin;
            cnt++;
        }
    }

    VTR_ASSERT(1 == cnt);

    return found_pb_pin;
}

/********************************************************************
 * Recursively remove all the downstream pb_routes with a given starting point
 * until reach the primitive nodes
 *******************************************************************/
static void rec_remove_downstream_pb_routes(t_pb_routes& curr_pb_routes,
                                            const int& src_pb_route_id) {
    for (const auto& sink_pb_route : curr_pb_routes.at(src_pb_route_id).sink_pb_pin_ids) {
        /* Go recursively if this is not the end */
        rec_remove_downstream_pb_routes(curr_pb_routes, sink_pb_route);
        /* Remove this sink */
        curr_pb_routes.erase(sink_pb_route);
    }
}

/********************************************************************
 * Update routing traces for regular nets of a logical block
 * This function should NOT 
 *  - create a routing traces but use the new routing traces
 *    from the inputs
 *  - modify any routing traces for global nets,
 *    which should be handled in another function!!!
 *******************************************************************/
static void update_cluster_regular_routing_traces_with_post_routing_results(AtomContext& atom_ctx,
                                                                            const std::map<int, std::pair<AtomPinId, const t_pb_graph_pin*>>& previous_atom_pin_to_pb_pin_mapping,
                                                                            ClusteringContext& clustering_ctx,
                                                                            const ClusterBlockId& blk_id,
                                                                            t_pb* pb,
                                                                            t_logical_block_type_ptr logical_block,
                                                                            t_pb_routes& new_pb_routes,
                                                                            size_t& num_fixup,
                                                                            const bool& verbose) {
    /* Go through each pb_graph pin at the top level
     * and build the new routing traces
     */
    for (int pb_type_pin = 0; pb_type_pin < logical_block->pb_type->num_pins; ++pb_type_pin) {
        /* Skip non-equivalent ports, no need to do fix-up */
        const t_pb_graph_pin* pb_graph_pin = get_pb_graph_node_pin_from_block_pin(blk_id, pb_type_pin);
        if (PortEquivalence::FULL != pb_graph_pin->port->equivalent) {
            continue;
        }

        /* Limitation: bypass output pins now
         * TODO: This is due to the 'instance' equivalence port 
         * where outputs may be swapped. This definitely requires re-run of packing
         * It can not be solved by swapping routing traces now
         */
        if (OUT_PORT == pb_graph_pin->port->type) {
            continue;
        }

        /* Sanity check to ensure the pb_graph_pin is the top-level */
        VTR_ASSERT(pb_graph_pin->parent_node == pb->pb_graph_node);
        VTR_ASSERT(pb_graph_pin->parent_node->is_root());

        auto remapped_result = clustering_ctx.post_routing_clb_pin_nets.at(blk_id).find(pb_graph_pin->pin_count_in_cluster);

        /* Skip this pin: it is consistent in pre- and post- routing results */
        if (remapped_result == clustering_ctx.post_routing_clb_pin_nets.at(blk_id).end()) {
            continue;
        }

        /* Update only when there is a remapping! */
        VTR_ASSERT_SAFE(remapped_result != clustering_ctx.post_routing_clb_pin_nets[blk_id].end());

        /* Cache the remapped net id */
        AtomNetId remapped_net = atom_ctx.lookup.atom_net(remapped_result->second);

        /* Skip those pins become unmapped after remapping */
        if (!remapped_net) {
            /* Remove the invalid pb_route */
            ClusterNetId global_net_id = clustering_ctx.clb_nlist.block_net(blk_id, pb_type_pin);
            if ((clustering_ctx.clb_nlist.valid_net_id(global_net_id))
                && (!clustering_ctx.clb_nlist.net_is_ignored(global_net_id))) {
                /* Special: for single-fanout pin, remove all the downstream pb_routes */
                if (is_single_fanout_pb_pin(const_cast<const t_pb_graph_pin*>(pb_graph_pin))) {
                    rec_remove_downstream_pb_routes(new_pb_routes, pb_graph_pin->pin_count_in_cluster);
                } else {
                    VTR_ASSERT_SAFE(!is_single_fanout_pb_pin(const_cast<const t_pb_graph_pin*>(pb_graph_pin)));
                    /* Now remove the source routing tree */
                    new_pb_routes.erase(pb_graph_pin->pin_count_in_cluster);
                }
            }
            continue;
        }

        VTR_LOGV(verbose,
                 "Remapping routing trace for net '%s'\n",
                 atom_ctx.nlist.net_name(remapped_net).c_str());

        /* Spot the routing trace
         * Two conditions could happen: 
         * - There is already a routing trace for this pin:
         *   we just rename the net id
         * - There is no routed path for this pin:
         *   we have to find a routing trace which is used by another pin
         *   in the same port (every pin in this port should be logic equivalent) 
         *   Rename the net id and pb_graph_node pins
         */
        int pb_route_id = find_target_pb_route_from_equivalent_pins(atom_ctx,
                                                                    clustering_ctx,
                                                                    blk_id,
                                                                    pb,
                                                                    pb_graph_pin,
                                                                    remapped_net,
                                                                    verbose);

        /* Record the previous pin mapping for finding the correct pin index during timing analysis */
        clustering_ctx.pre_routing_net_pin_mapping[blk_id][pb_graph_pin->pin_count_in_cluster] = pb_route_id;

        /* Remove the old pb_route and insert the new one */
        new_pb_routes.insert(std::make_pair(pb_graph_pin->pin_count_in_cluster, t_pb_route()));
        t_pb_route& new_pb_route = new_pb_routes[pb_graph_pin->pin_count_in_cluster];

        /* Deposit pb_route data from the reference */
        VTR_ASSERT(remapped_net == pb->pb_route.at(pb_route_id).atom_net_id);
        new_pb_route.atom_net_id = pb->pb_route.at(pb_route_id).atom_net_id;
        new_pb_route.pb_graph_pin = pb->pb_route.at(pb_route_id).pb_graph_pin;
        new_pb_route.driver_pb_pin_id = pb->pb_route.at(pb_route_id).driver_pb_pin_id;

        /* Modify the source pb_graph_pin if we reuse routing trace from another pin */
        if (new_pb_route.pb_graph_pin != pb_graph_pin) {
            new_pb_route.pb_graph_pin = pb_graph_pin;
            new_pb_route.driver_pb_pin_id = OPEN;
        }

        /* Check if the sink pb_pins are accessible from the driver pb_pin
         * - If not accessible, remove the associated sink pin_pins
         * - We will add the only accessible pin if and only if 
         *   the driver pb_pin has only 1 fan-out according to the pb_graph
         *
         * Note:
         *   - This is applicable to the case where the driver pb_pin
         *     has only 1 fan-out!!!
         *   - This fix-up is mainly for the following pb_type organization
         *
         *             Top-level pb_type
         *           +----------------------------
         *           |   +------------------------    
         *           |   |      Primitive pb_type
         *           |   |     +------------------
         *           |   |     |
         *   driver  +---+---->| sink
         *           |
         *
         *     
         *     Anything violates the assumption will be errored out!
         */
        new_pb_route.sink_pb_pin_ids.clear();
        std::map<int, bool> sink_pb_pin_accessibility;
        t_pb_graph_pin* sink_pb_pin_to_add = nullptr;

        for (int iedge = 0; iedge < pb_graph_pin->num_output_edges; ++iedge) {
            for (int ipin = 0; ipin < pb_graph_pin->output_edges[iedge]->num_output_pins; ++ipin) {
                sink_pb_pin_accessibility[pb_graph_pin->output_edges[iedge]->output_pins[ipin]->pin_count_in_cluster] = true;
            }
        }

        if (is_single_fanout_pb_pin(const_cast<const t_pb_graph_pin*>(pb_graph_pin))) {
            sink_pb_pin_to_add = pb_graph_pin->output_edges[0]->output_pins[0];
        }

        for (const int& sink_pb_route : pb->pb_route.at(pb_route_id).sink_pb_pin_ids) {
            if (0 < sink_pb_pin_accessibility.count(sink_pb_route)) {
                new_pb_route.sink_pb_pin_ids.push_back(sink_pb_route);
            }
        }

        /* Add the directly connected pin ONLY when there is no pin to add */
        if ((sink_pb_pin_to_add)
            && (new_pb_route.sink_pb_pin_ids.empty())) {
            new_pb_route.sink_pb_pin_ids.push_back(sink_pb_pin_to_add->pin_count_in_cluster);
        }

        /* Since we modify the pb_route id at the top-level,
         * update the children so that we maintain the correct links
         * when back-tracing
         */
        for (const int& sink_pb_route : new_pb_route.sink_pb_pin_ids) {
            if (0 == new_pb_routes.count(sink_pb_route)) {
                /* A new sink pb_pin requires a new pb_route to be added
                 * This could lead to a chain effect to add new pb_routes   
                 * We keep adding the pb_pin until we reach a primitive pb_type
                 * All the sink pb_pin of the pb_route to be added
                 * must meet the following requirements
                 * so that the addition can be considered as legal:
                 * - It has only 1 fan-out
                 *
                 */
                VTR_ASSERT(sink_pb_route == sink_pb_pin_to_add->pin_count_in_cluster);
                t_pb_graph_pin* new_sink_pb_pin_to_add = sink_pb_pin_to_add;
                VTR_ASSERT(is_single_fanout_pb_pin(const_cast<const t_pb_graph_pin*>(new_sink_pb_pin_to_add)));
                int new_driver_pb_pin = pb_graph_pin->pin_count_in_cluster;
                while (1) {
                    int new_sink_pb_route_id = new_sink_pb_pin_to_add->pin_count_in_cluster;
                    new_pb_routes.insert(std::make_pair(new_sink_pb_route_id, t_pb_route()));
                    new_pb_routes[new_sink_pb_route_id].atom_net_id = remapped_net;
                    new_pb_routes[new_sink_pb_route_id].pb_graph_pin = new_sink_pb_pin_to_add;
                    new_pb_routes[new_sink_pb_route_id].driver_pb_pin_id = new_driver_pb_pin;

                    /* Stop adding more routing tree when reach the primitive */
                    if (new_sink_pb_pin_to_add->parent_node->is_primitive()) {
                        break;
                    }

                    /* Go to the next level */
                    const t_pb* sink_pb = pb->find_pb(new_sink_pb_pin_to_add->parent_node);
                    VTR_ASSERT(sink_pb);
                    t_pb_graph_pin* next_sink_pb_pin_to_add = find_used_sink_pb_pin(new_sink_pb_pin_to_add, sink_pb->mode);
                    VTR_ASSERT(next_sink_pb_pin_to_add);

                    /* Assign sinks */
                    new_pb_routes[new_sink_pb_route_id].sink_pb_pin_ids.push_back(next_sink_pb_pin_to_add->pin_count_in_cluster);

                    /* Print verbose outputs */
                    VTR_LOGV(verbose,
                             "Add a new trace '%d' linked to pin '%s' for net '%s'.\n",
                             new_sink_pb_route_id,
                             new_sink_pb_pin_to_add->to_string().c_str(),
                             atom_ctx.nlist.net_name(remapped_net).c_str());

                    /* Update the pins to add for next round */
                    new_driver_pb_pin = new_sink_pb_pin_to_add->pin_count_in_cluster;
                    new_sink_pb_pin_to_add = next_sink_pb_pin_to_add;
                }
                /* The chain effect will lead to a remapping to atom net pins
                 * Fix the atom net lookup 
                 */
                VTR_ASSERT(1 == pb->pb_route.at(pb_route_id).sink_pb_pin_ids.size());
                const AtomPinId& orig_mapped_atom_pin = previous_atom_pin_to_pb_pin_mapping.at(pb_route_id).first;

                /* Print verbose outputs */
                VTR_LOGV(verbose,
                         "Redirect atom pin '%lu' mapping from '%s' to '%s' for net '%s'.\n",
                         size_t(orig_mapped_atom_pin),
                         atom_ctx.lookup.atom_pin_pb_graph_pin(orig_mapped_atom_pin)->to_string().c_str(),
                         new_sink_pb_pin_to_add->to_string().c_str(),
                         atom_ctx.nlist.net_name(remapped_net).c_str());

                /* Update the pin binding in atom netlist fast look-up */
                atom_ctx.lookup.set_atom_pin_pb_graph_pin(orig_mapped_atom_pin, new_sink_pb_pin_to_add);
                VTR_ASSERT(new_sink_pb_pin_to_add == atom_ctx.lookup.atom_pin_pb_graph_pin(orig_mapped_atom_pin));

                /* Update the pin rotation map */
                t_pb* atom_pb = pb->find_mutable_pb(new_sink_pb_pin_to_add->parent_node);
                atom_pb->set_atom_pin_bit_index(new_sink_pb_pin_to_add, atom_ctx.nlist.pin_port_bit(orig_mapped_atom_pin));
            } else {
                new_pb_routes[sink_pb_route].driver_pb_pin_id = pb_graph_pin->pin_count_in_cluster;

                /* It is possible that the sink pb_route was used by another net
                 * If so, we need to correct the net ids for all the downstream pb_routes
                 */
                if (remapped_net != new_pb_routes[sink_pb_route].atom_net_id) {
                    new_pb_routes[sink_pb_route].atom_net_id = remapped_net;

                    const t_pb_graph_pin* sink_pb_pin = new_pb_routes[sink_pb_route].pb_graph_pin;
                    if (is_single_fanout_pb_pin(sink_pb_pin)) {
                        /* Try to find the leaf pb_graph pin in the routing trace */
                        t_pb_graph_pin* next_pb_pin = const_cast<t_pb_graph_pin*>(sink_pb_pin);
                        while (!next_pb_pin->parent_node->is_primitive()) {
                            /* Go to the next level */
                            const t_pb* next_pb = pb->find_pb(next_pb_pin->parent_node);
                            VTR_ASSERT(next_pb);
                            next_pb_pin = find_used_sink_pb_pin(next_pb_pin, next_pb->mode);
                            VTR_ASSERT(next_pb_pin);
                            new_pb_routes[next_pb_pin->pin_count_in_cluster].atom_net_id = remapped_net;
                        }

                        /* Since this routing trace was mapped to another net,
                         * we have to spot the original pb_graph_pin that is mapped 
                         * to this net (from the other equivalent pins in this port)
                         * Then we can find out the atom pin that is linked to the remapped net
                         */
                        const t_pb_graph_pin* orig_mapped_top_level_pb_pin = find_mapped_equivalent_pb_pin_by_net(pb, pb_graph_pin, remapped_net);
                        VTR_ASSERT(orig_mapped_top_level_pb_pin);
                        const AtomPinId& orig_mapped_atom_pin = previous_atom_pin_to_pb_pin_mapping.at(orig_mapped_top_level_pb_pin->pin_count_in_cluster).first;

                        /* Print verbose outputs */
                        VTR_LOGV(verbose,
                                 "Redirect atom pin '%lu' mapping from '%s' to '%s' for net '%s'.\n",
                                 size_t(orig_mapped_atom_pin),
                                 atom_ctx.lookup.atom_pin_pb_graph_pin(orig_mapped_atom_pin)->to_string().c_str(),
                                 next_pb_pin->to_string().c_str(),
                                 atom_ctx.nlist.net_name(remapped_net).c_str());

                        /* Update the pin binding in atom netlist fast look-up */
                        atom_ctx.lookup.set_atom_pin_pb_graph_pin(orig_mapped_atom_pin, next_pb_pin);
                        VTR_ASSERT(next_pb_pin == atom_ctx.lookup.atom_pin_pb_graph_pin(orig_mapped_atom_pin));

                        /* Update the pin rotation map */
                        t_pb* atom_pb = pb->find_mutable_pb(next_pb_pin->parent_node);
                        atom_pb->set_atom_pin_bit_index(next_pb_pin, atom_ctx.nlist.pin_port_bit(orig_mapped_atom_pin));
                    }
                }
            }
        }

        VTR_LOGV(verbose,
                 "Remap clustered block '%s' routing trace[%d] to net '%s'\n",
                 clustering_ctx.clb_nlist.block_pb(blk_id)->name,
                 pb_graph_pin->pin_count_in_cluster,
                 atom_ctx.nlist.net_name(remapped_net).c_str());

        /* Update fixup counter */
        num_fixup++;
    }
}

/********************************************************************
 * Update routing traces for global nets of a logical block
 * This function should NOT 
 *  - create a routing traces but use the new routing traces
 *    from the inputs
 *  - modify any routing traces for non-global nets,
 *    which should be handled in another function!!!
 *******************************************************************/
static void update_cluster_global_routing_traces_with_post_routing_results(const AtomContext& atom_ctx,
                                                                           ClusteringContext& clustering_ctx,
                                                                           const ClusterBlockId& blk_id,
                                                                           t_pb* pb,
                                                                           t_logical_block_type_ptr logical_block,
                                                                           t_pb_routes& new_pb_routes,
                                                                           size_t& num_fixup,
                                                                           const bool& verbose) {
    /* Reassign global nets to unused pins in the same port where they were mapped
     * NO optimization is done here!!! First find first fit
     */
    for (int pb_type_pin = 0; pb_type_pin < logical_block->pb_type->num_pins; ++pb_type_pin) {
        const t_pb_graph_pin* pb_graph_pin = get_pb_graph_node_pin_from_block_pin(blk_id, pb_type_pin);

        /* Limitation: bypass output pins now
         * TODO: This is due to the 'instance' equivalence port 
         * where outputs may be swapped. This definitely requires re-run of packing
         * It can not be solved by swapping routing traces now
         */
        if (OUT_PORT == pb_graph_pin->port->type) {
            continue;
        }

        /* Sanity check to ensure the pb_graph_pin is the top-level */
        VTR_ASSERT(pb_graph_pin->parent_node == pb->pb_graph_node);
        VTR_ASSERT(pb_graph_pin->parent_node->is_root());

        /* Focus on global net only */
        ClusterNetId global_net_id = clustering_ctx.clb_nlist.block_net(blk_id, pb_type_pin);
        if (!clustering_ctx.clb_nlist.valid_net_id(global_net_id)) {
            continue;
        }
        if ((clustering_ctx.clb_nlist.valid_net_id(global_net_id))
            && (!clustering_ctx.clb_nlist.net_is_ignored(global_net_id))) {
            continue;
        }

        AtomNetId global_atom_net_id = atom_ctx.lookup.atom_net(global_net_id);

        auto remapped_result = clustering_ctx.post_routing_clb_pin_nets.at(blk_id).find(pb_graph_pin->pin_count_in_cluster);

        /* Skip this pin: it is consistent in pre- and post- routing results */
        if (remapped_result == clustering_ctx.post_routing_clb_pin_nets.at(blk_id).end()) {
            continue;
        }

        /* Update only when there is a remapping! */
        VTR_ASSERT_SAFE(remapped_result != clustering_ctx.post_routing_clb_pin_nets[blk_id].end());

        VTR_LOGV(verbose,
                 "Remapping clustered block '%s' global net '%s' to unused pin as %s\r",
                 clustering_ctx.clb_nlist.block_pb(blk_id)->name,
                 atom_ctx.nlist.net_name(global_atom_net_id).c_str(),
                 pb_graph_pin->to_string().c_str());

        const t_pb_graph_pin* unused_pb_graph_pin = find_unused_pb_graph_pin_in_the_same_port(pb_graph_pin, new_pb_routes, global_atom_net_id);
        /* Must find one */
        VTR_ASSERT(nullptr != unused_pb_graph_pin);
        /* Create a new pb_route and update sink pb_route */
        /* Remove the old pb_route and insert the new one */
        new_pb_routes.insert(std::make_pair(unused_pb_graph_pin->pin_count_in_cluster, t_pb_route()));
        t_pb_route& new_pb_route = new_pb_routes[unused_pb_graph_pin->pin_count_in_cluster];

        int pb_route_id = pb_graph_pin->pin_count_in_cluster;
        /* Deposit pb_route data from the reference */
        VTR_ASSERT(global_atom_net_id == pb->pb_route.at(pb_route_id).atom_net_id);
        new_pb_route.atom_net_id = pb->pb_route.at(pb_route_id).atom_net_id;
        new_pb_route.pb_graph_pin = unused_pb_graph_pin;
        new_pb_route.driver_pb_pin_id = pb->pb_route.at(pb_route_id).driver_pb_pin_id;
        new_pb_route.sink_pb_pin_ids = pb->pb_route.at(pb_route_id).sink_pb_pin_ids;

        for (const int& sink_pb_route : new_pb_route.sink_pb_pin_ids) {
            VTR_ASSERT(new_pb_routes.at(sink_pb_route).atom_net_id == global_atom_net_id);
            new_pb_routes[sink_pb_route].driver_pb_pin_id = unused_pb_graph_pin->pin_count_in_cluster;
        }

        /* Update the remapping nets for this global net */
        clustering_ctx.post_routing_clb_pin_nets[blk_id][unused_pb_graph_pin->pin_count_in_cluster] = global_net_id;
        clustering_ctx.pre_routing_net_pin_mapping[blk_id][unused_pb_graph_pin->pin_count_in_cluster] = pb_route_id;

        VTR_LOGV(verbose,
                 "Remap clustered block '%s' global net '%s' to pin '%s'\n",
                 clustering_ctx.clb_nlist.block_pb(blk_id)->name,
                 atom_ctx.nlist.net_name(global_atom_net_id).c_str(),
                 unused_pb_graph_pin->to_string().c_str());

        /* Update fixup counter */
        num_fixup++;
    }
}

/********************************************************************
 * Fix up routing traces for a given clustered block
 * This function will directly update the nets of routing traces
 * stored in the clustered block by considering the post-routing results
 *
 * Note: 
 *   - This function should be called AFTER the function
 *       update_cluster_pin_with_post_routing_results()
 *******************************************************************/
static void update_cluster_routing_traces_with_post_routing_results(AtomContext& atom_ctx,
                                                                    const IntraLbPbPinLookup& intra_lb_pb_pin_lookup,
                                                                    ClusteringContext& clustering_ctx,
                                                                    const ClusterBlockId& blk_id,
                                                                    size_t& num_fixup,
                                                                    const bool& verbose) {
    /* Skip block where no remapping is applied */
    if (clustering_ctx.post_routing_clb_pin_nets.find(blk_id) == clustering_ctx.post_routing_clb_pin_nets.end()) {
        return;
    }

    t_pb* pb = clustering_ctx.clb_nlist.block_pb(blk_id);
    auto logical_block = clustering_ctx.clb_nlist.block_type(blk_id);

    /* Create a new set of pb routing traces */
    t_pb_routes new_pb_routes = pb->pb_route;

    /* Cache the current mapping between atom pin to pb_graph pin in this block */
    std::map<int, std::pair<AtomPinId, const t_pb_graph_pin*>> previous_atom_pin_to_pb_pin_mapping = cache_atom_pin_to_pb_pin_mapping(const_cast<const AtomContext&>(atom_ctx), intra_lb_pb_pin_lookup, const_cast<const ClusteringContext&>(clustering_ctx), blk_id, pb, logical_block);

    update_cluster_regular_routing_traces_with_post_routing_results(atom_ctx,
                                                                    previous_atom_pin_to_pb_pin_mapping,
                                                                    clustering_ctx,
                                                                    blk_id,
                                                                    pb,
                                                                    logical_block,
                                                                    new_pb_routes,
                                                                    num_fixup,
                                                                    verbose);

    update_cluster_global_routing_traces_with_post_routing_results(const_cast<const AtomContext&>(atom_ctx),
                                                                   clustering_ctx,
                                                                   blk_id,
                                                                   pb,
                                                                   logical_block,
                                                                   new_pb_routes,
                                                                   num_fixup,
                                                                   verbose);

    /* Replace old pb_routes with the new one */
    pb->pb_route = new_pb_routes;
}

/********************************************************************
 * Top-level function to synchronize a packed netlist to routing results
 * The problem comes from a mismatch between the packing and routing results
 * When there are equivalent input/output for any grids, router will try
 * to swap the net mapping among these pins so as to achieve best 
 * routing optimization.
 * However, it will cause the packing results out-of-date as the net mapping 
 * of each grid remain untouched once packing is done.
 * This function aims to fix the mess after routing so that the net mapping
 * can be synchronized
 *
 * Note:
 *   - This function SHOULD be run ONLY when routing is finished!!!
 *******************************************************************/
void sync_netlists_to_routing(const DeviceContext& device_ctx,
                              AtomContext& atom_ctx,
                              ClusteringContext& clustering_ctx,
                              const PlacementContext& placement_ctx,
                              const RoutingContext& routing_ctx,
                              const bool& verbose) {
    vtr::ScopedStartFinishTimer timer("Synchronize the packed netlist to routing optimization");

    /* Reset the database for post-routing clb net mapping */
    clustering_ctx.post_routing_clb_pin_nets.clear();
    clustering_ctx.pre_routing_net_pin_mapping.clear();

    /* Create net-to-rr_node mapping */
    vtr::vector<RRNodeId, ClusterNetId> rr_node_nets = annotate_rr_node_nets(device_ctx,
                                                                             clustering_ctx,
                                                                             routing_ctx,
                                                                             verbose);

    IntraLbPbPinLookup intra_lb_pb_pin_lookup(device_ctx.logical_block_types);

    /* Count the number of mismatches and fix-up */
    size_t num_mismatches = 0;
    size_t num_fixup = 0;

    /* Update the core logic (center blocks of the FPGA) */
    for (const ClusterBlockId& cluster_blk_id : clustering_ctx.clb_nlist.blocks()) {
        /* We know the entrance to grid info and mapping results, do the fix-up for this block */
        vtr::Point<size_t> grid_coord(placement_ctx.block_locs[cluster_blk_id].loc.x,
                                      placement_ctx.block_locs[cluster_blk_id].loc.y);

        update_cluster_pin_with_post_routing_results(device_ctx,
                                                     clustering_ctx,
                                                     rr_node_nets,
                                                     grid_coord, cluster_blk_id,
                                                     placement_ctx.block_locs[cluster_blk_id].loc.sub_tile,
                                                     num_mismatches,
                                                     verbose);
        update_cluster_routing_traces_with_post_routing_results(atom_ctx,
                                                                intra_lb_pb_pin_lookup,
                                                                clustering_ctx,
                                                                cluster_blk_id,
                                                                num_fixup,
                                                                verbose);
    }

    /* Print a short summary */
    VTR_LOG("Found %lu mismatches between routing and packing results.\n",
            num_mismatches);
    VTR_LOG("Fixed %lu routing traces due to mismatch between routing and packing results.\n",
            num_fixup);
}
