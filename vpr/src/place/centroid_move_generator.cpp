#include "centroid_move_generator.h"
#include "vpr_types.h"
#include "globals.h"
#include "directed_moves_util.h"
#include "place_constraints.h"
#include "move_utils.h"

#include <queue>


// Static member variable definitions
vtr::vector<NocGroupId, std::vector<ClusterBlockId>> CentroidMoveGenerator::noc_group_clusters_;
vtr::vector<NocGroupId, std::vector<ClusterBlockId>> CentroidMoveGenerator::noc_group_routers_;
vtr::vector<ClusterBlockId, NocGroupId> CentroidMoveGenerator::cluster_to_noc_grp_;
std::map<ClusterBlockId, NocGroupId> CentroidMoveGenerator::noc_router_to_noc_group_;


CentroidMoveGenerator::CentroidMoveGenerator()
    : noc_attraction_w_(0.0f)
    , noc_attraction_enabled_(false) {}

CentroidMoveGenerator::CentroidMoveGenerator(float noc_attraction_weight, size_t high_fanout_net)
    : noc_attraction_w_(noc_attraction_weight)
    , noc_attraction_enabled_(true) {
    VTR_ASSERT(noc_attraction_weight > 0.0 && noc_attraction_weight <= 1.0);


    // check if static member variables are already initialized
    if (!noc_group_clusters_.empty() && !noc_group_routers_.empty() &&
        !cluster_to_noc_grp_.empty() && !noc_router_to_noc_group_.empty()) {
        return;
    } else {
        initialize_noc_groups(high_fanout_net);
    }
}

e_create_move CentroidMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                  t_propose_action& proposed_action,
                                                  float rlim,
                                                  const t_placer_opts& placer_opts,
                                                  const PlacerCriticalities* /*criticalities*/) {
    // Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(placer_opts,
                                                  proposed_action.logical_blk_type_index,
                                                  false,
                                                  nullptr,
                                                  nullptr);

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                   "Centroid Move Choose Block %d - rlim %f\n",
                   size_t(b_from),
                   rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                       "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& place_ctx = g_vpr_ctx.placement();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = device_ctx.grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_range_limiters range_limiters{rlim,
                                    place_move_ctx.first_rlim,
                                    placer_opts.place_dm_rlim};

    t_pl_loc to, centroid;

    /* Calculate the centroid location*/
    calculate_centroid_loc(b_from, false, centroid, nullptr, noc_attraction_enabled_, noc_attraction_w_);

    // Centroid location is not necessarily a valid location, and the downstream location expects a valid
    // layer for the centroid location. So if the layer is not valid, we set it to the same layer as from loc.
    centroid.layer = (centroid.layer < 0) ? from.layer : centroid.layer;
    /* Find a location near the weighted centroid_loc */
    if (!find_to_loc_centroid(cluster_from_type, from, centroid, range_limiters, to, b_from)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

const std::vector<ClusterBlockId>& CentroidMoveGenerator::get_noc_group_routers(NocGroupId noc_grp_id) {
    return CentroidMoveGenerator::noc_group_routers_[noc_grp_id];
}

NocGroupId CentroidMoveGenerator::get_cluster_noc_group(ClusterBlockId blk_id) {
    return CentroidMoveGenerator::cluster_to_noc_grp_[blk_id];
}

void CentroidMoveGenerator::initialize_noc_groups(size_t high_fanout_net) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& noc_ctx = g_vpr_ctx.noc();

    noc_group_clusters_.clear();
    noc_group_routers_.clear();
    cluster_to_noc_grp_.clear();
    noc_router_to_noc_group_.clear();

    /*
     * Assume the clustered netlist is an undirected graph where nodes
     * represent clustered blocks, and edges are low fanout connections.
     * To determine NoC groups, we need to find components that include
     * at least one NoC router. To do this, we start a BFS traversal from
     * unvisited NoC routers and assign the starting NoC router and all the
     * blocks that are visited during the traversal to a NoC group.
     */

    // determines whether a block is visited
    vtr::vector<ClusterBlockId, bool> block_visited(cluster_ctx.clb_nlist.blocks().size(), false);

    // NoCGroupIDs are sequential and start from zero. This counter specifies the value to be assigned to a new NoCGroupID
    int noc_group_cnt = 0;

    // Initialize the associated NoC group for all blocks to INVALID. If a block is not visited during traversal,
    // it does not belong to any NoC groups. For other blocks, the associated NoC group is updated once they are visited.
    cluster_to_noc_grp_.resize(cluster_ctx.clb_nlist.blocks().size(), NocGroupId ::INVALID());

    // Get all the router clusters and the NoC router logical block type
    const auto& router_blk_ids = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();
    const auto router_block_type = cluster_ctx.clb_nlist.block_type(router_blk_ids[0]);

    // iterate over logical NoC routers and start a BFS
    for (auto router_blk_id : router_blk_ids) {

        if (block_visited[router_blk_id]) {
            continue;
        }

        NocGroupId noc_group_id(noc_group_cnt);
        noc_group_cnt++;
        noc_group_routers_.emplace_back();
        noc_group_clusters_.emplace_back();

        // BFS frontier
        std::queue<ClusterBlockId> q;

        // initialize the frontier with the NoC router
        q.push(router_blk_id);
        block_visited[router_blk_id] = true;

        while (!q.empty()) {
            ClusterBlockId current_block_id = q.front();
            q.pop();

            // get the logical block type for the block extracted from the frontier queue
            auto block_type = cluster_ctx.clb_nlist.block_type(current_block_id);

            if (block_type->index == router_block_type->index) {
                noc_group_routers_[noc_group_id].push_back(current_block_id);
                noc_router_to_noc_group_[current_block_id] = noc_group_id;
            } else {
                noc_group_clusters_[noc_group_id].push_back(current_block_id);
                cluster_to_noc_grp_[current_block_id] = noc_group_id;
            }

            // iterate over all low fanout nets of the current block to find its unvisited neighbors
            for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(current_block_id)) {
                ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);

                if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
                    continue;
                }

                if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= high_fanout_net) {
                    continue;
                }

                if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
                    for (auto sink_pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                        ClusterBlockId sink_block_id = cluster_ctx.clb_nlist.pin_block(sink_pin_id);
                        if (!block_visited[sink_block_id]) {
                            block_visited[sink_block_id] = true;
                            q.push(sink_block_id);
                        }
                    }
                } else { //else the pin is sink --> only care about its driver
                    ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
                    ClusterBlockId source_blk_id = cluster_ctx.clb_nlist.pin_block(source_pin);
                    if (!block_visited[source_blk_id]) {
                        block_visited[source_blk_id] = true;
                        q.push(source_blk_id);
                    }
                }
            }
        }
    }
}
