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
static std::vector<e_side> find_logic_tile_pin_side(t_physical_tile_type_ptr physical_tile,
                                                    const int& physical_pin) {
    std::vector<e_side> pin_sides;
    for (const e_side& side_cand : {TOP, RIGHT, BOTTOM, LEFT}) {
        int pin_width_offset = physical_tile->pin_width_offset[physical_pin];
        int pin_height_offset = physical_tile->pin_height_offset[physical_pin];
        if (true == physical_tile->pinloc[pin_width_offset][pin_height_offset][side_cand][physical_pin]) {
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
                                                         const RoutingContext& routing_ctx,
                                                         const vtr::Point<size_t>& grid_coord,
                                                         const ClusterBlockId& blk_id,
                                                         const e_side& border_side,
                                                         const int& sub_tile_z,
                                                         const bool& verbose) {
    /* Handle each pin */
    auto logical_block = clustering_ctx.clb_nlist.block_type(blk_id);
    auto physical_tile = device_ctx.grid[grid_coord.x()][grid_coord.y()].type;

    for (int j = 0; j < logical_block->pb_type->num_pins; j++) {
        /* Get the ptc num for the pin in rr_graph, we need to consider the sub tile offset here
         * sub tile offset is the location in a sub tile whose capacity is larger than zero
         * TODO:
         * It seems that the get_physical_pin() function does not consider the offset z in the
         * sub tile look-up.
         * It is necessary to understand how we can consider 
         * the offset in terms of capacity of each sub_tile
         */
        int physical_pin = get_physical_pin(physical_tile, logical_block, sub_tile_z, j);
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

        std::vector<e_side> pin_sides = find_logic_tile_pin_side(physical_tile, physical_pin);
        /* As some grid has height/width offset, we may not have the pin on any side */
        if (0 == pin_sides.size()) {
            continue;
        }

        /* Special for I/O grids: VPR creates the grid with duplicated pins on every side 
         * but the expected side (only used side) will be opposite side of the border side!
         */
        if (NUM_SIDES != border_side) {
            pin_sides.clear();
            switch (border_side) {
                case TOP:
                    pin_sides.push_back(BOTTOM);
                    break;
                case RIGHT:
                    pin_sides.push_back(LEFT);
                    break;
                case BOTTOM:
                    pin_sides.push_back(TOP);
                    break;
                case LEFT:
                    pin_sides.push_back(RIGHT);
                    break;
                default:
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Invalid side for logic blocks at FPGA border!\n");
            }
        }

        for (const e_side& pin_side : pin_sides) {
            /* Find the net mapped to this pin in routing results */
            const int& rr_node = get_rr_node_index(device_ctx.rr_node_indices,
                                                   grid_coord.x(), grid_coord.y(),
                                                   rr_node_type, physical_pin, pin_side);

            /* Bypass invalid nodes, after that we must have a valid rr_node id */
            if (-1 == rr_node) {
                continue;
            }
            VTR_ASSERT((size_t)rr_node < device_ctx.rr_nodes.size());

            /* Get the cluster net id which has been mapped to this net */
            ClusterNetId routing_net_id = routing_ctx.rr_node_nets[RRNodeId(rr_node)];

            /* Find the net mapped to this pin in clustering results*/
            ClusterNetId cluster_net_id = clustering_ctx.clb_nlist.block_net(blk_id, j);

            /* Ignore those net have never been routed */
            if ((ClusterNetId::INVALID() != cluster_net_id)
                && (true == clustering_ctx.clb_nlist.net_is_ignored(cluster_net_id))) {
                continue;
            }

            /* Ignore used in local cluster only, reserved one CLB pin */
            if ((ClusterNetId::INVALID() != cluster_net_id)
                && (0 == clustering_ctx.clb_nlist.net_sinks(cluster_net_id).size())) {
                continue;
            }

            /* If matched, we finish here */
            if (routing_net_id == cluster_net_id) {
                continue;
            }

            /* Update the clustering context with net modification */
            clustering_ctx.post_routing_clb_pin_nets[blk_id][j] = routing_net_id;

            std::string routing_net_name("unmapped");
            if (ClusterNetId::INVALID() != routing_net_id) {
                routing_net_name = clustering_ctx.clb_nlist.net_name(routing_net_id);
            }

            std::string cluster_net_name("unmapped");
            if (ClusterNetId::INVALID() != cluster_net_id) {
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
        }
    }
}

/********************************************************************
 * Top-level function to fix up the pb pin mapping results 
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
void update_pb_pin_with_post_routing_results(const DeviceContext& device_ctx,
                                             ClusteringContext& clustering_ctx,
                                             const PlacementContext& placement_ctx,
                                             const RoutingContext& routing_ctx,
                                             const bool& verbose) {
    vtr::ScopedStartFinishTimer timer("Fix up pb pin mapping results after routing optimization");

    /* Reset the database for post-routing clb net mapping */
    clustering_ctx.post_routing_clb_pin_nets.clear();

    /* Update the core logic (center blocks of the FPGA) */
    for (size_t x = 1; x < device_ctx.grid.width() - 1; ++x) {
        for (size_t y = 1; y < device_ctx.grid.height() - 1; ++y) {
            /* Bypass the EMPTY tiles */
            if (true == is_empty_type(device_ctx.grid[x][y].type)) {
                continue;
            }
            /* Get the mapped blocks to this grid */
            for (const ClusterBlockId& cluster_blk_id : placement_ctx.grid_blocks[x][y].blocks) {
                /* Skip invalid ids */
                if (ClusterBlockId::INVALID() == cluster_blk_id) {
                    continue;
                }
                /* We know the entrance to grid info and mapping results, do the fix-up for this block */
                vtr::Point<size_t> grid_coord(x, y);
                update_cluster_pin_with_post_routing_results(device_ctx,
                                                             clustering_ctx,
                                                             routing_ctx,
                                                             grid_coord, cluster_blk_id, NUM_SIDES,
                                                             placement_ctx.block_locs[cluster_blk_id].loc.sub_tile,
                                                             verbose);
            }
        }
    }

    /* Update the periperal I/O blocks at fours sides of FPGA */
    std::vector<e_side> io_sides{TOP, RIGHT, BOTTOM, LEFT};
    std::map<e_side, std::vector<vtr::Point<size_t>>> io_coords;

    /* TOP side */
    for (size_t x = 1; x < device_ctx.grid.width() - 1; ++x) {
        io_coords[TOP].push_back(vtr::Point<size_t>(x, device_ctx.grid.height() - 1));
    }

    /* RIGHT side */
    for (size_t y = 1; y < device_ctx.grid.height() - 1; ++y) {
        io_coords[RIGHT].push_back(vtr::Point<size_t>(device_ctx.grid.width() - 1, y));
    }

    /* BOTTOM side */
    for (size_t x = 1; x < device_ctx.grid.width() - 1; ++x) {
        io_coords[BOTTOM].push_back(vtr::Point<size_t>(x, 0));
    }

    /* LEFT side */
    for (size_t y = 1; y < device_ctx.grid.height() - 1; ++y) {
        io_coords[LEFT].push_back(vtr::Point<size_t>(0, y));
    }

    /* Walk through io grid on by one */
    for (const e_side& io_side : io_sides) {
        for (const vtr::Point<size_t>& io_coord : io_coords[io_side]) {
            /* Bypass EMPTY grid */
            if (true == is_empty_type(device_ctx.grid[io_coord.x()][io_coord.y()].type)) {
                continue;
            }
            /* Get the mapped blocks to this grid */
            for (const ClusterBlockId& cluster_blk_id : placement_ctx.grid_blocks[io_coord.x()][io_coord.y()].blocks) {
                /* Skip invalid ids */
                if (ClusterBlockId::INVALID() == cluster_blk_id) {
                    continue;
                }
                /* Update on I/O grid */
                update_cluster_pin_with_post_routing_results(device_ctx,
                                                             clustering_ctx,
                                                             routing_ctx,
                                                             io_coord, cluster_blk_id, io_side,
                                                             placement_ctx.block_locs[cluster_blk_id].loc.sub_tile,
                                                             verbose);
            }
        }
    }
}
