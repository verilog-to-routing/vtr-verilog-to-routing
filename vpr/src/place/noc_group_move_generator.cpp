
#include "noc_group_move_generator.h"

#include "move_utils.h"
#include "place_constraints.h"

e_create_move NocGroupMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                  t_propose_action& proposed_action,
                                                  float rlim,
                                                  const t_placer_opts& placer_opts,
                                                  const PlacerCriticalities* criticalities) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    auto& place_ctx = g_vpr_ctx.placement();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    e_create_move create_move;

    const std::vector<ClusterBlockId>& router_blk_ids = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();

    const ClusterBlockId selected_noc_router_blk_id = router_blk_ids[vtr::irand(router_blk_ids.size()-1)];
    const NocGroupId noc_group_id = place_ctx.noc_router_to_noc_group.find(selected_noc_router_blk_id)->second;

    const auto& group_routers = place_ctx.noc_group_routers[noc_group_id];
    const auto& group_clusters = place_ctx.noc_group_clusters[noc_group_id];

    std::cout << "Group routers: " << group_routers.size() << ", group clusters: " << group_clusters.size() << std::endl;

    for (auto block_id : group_clusters) {
        ClusterBlockId router_blk_id = group_routers[vtr::irand(group_routers.size()-1)];
        t_block_loc router_loc = place_ctx.block_locs[router_blk_id];
        t_block_loc block_loc = place_ctx.block_locs[block_id];

        t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(block_id);
        const auto& compressed_block_grid = place_ctx.compressed_block_grids[block_type->index];

        std::vector<t_physical_tile_loc> router_compressed_loc = get_compressed_loc_approx(compressed_block_grid,
                                                                                           router_loc.loc,
                                                                                           1);

        //Determine the coordinates in the compressed grid space of the current block
        std::vector<t_physical_tile_loc> block_compressed_loc = get_compressed_loc_approx(compressed_block_grid,
                                                                                          block_loc.loc,
                                                                                          1);

        auto search_range = get_compressed_grid_target_search_range(compressed_block_grid,
                                                                    router_compressed_loc[0],
                                                                    20);

        int delta_cx = search_range.xmax - search_range.xmin;

        bool legal = find_compatible_compressed_loc_in_range(block_type,
                                                             delta_cx,
                                                             block_compressed_loc[0],
                                                             search_range,
                                                             router_compressed_loc[0],
                                                             false,
                                                             0,
                                                             true);

        if (!legal) {
            continue;
        }

        t_pl_loc to_loc;
        compressed_grid_to_loc(block_type, router_compressed_loc[0], to_loc);

        create_move = ::create_move(blocks_affected, block_id, to_loc);
        std::cout << "number of affected blocks: " << blocks_affected.num_moved_blocks << std::endl;
        if (create_move == e_create_move::ABORT) {
            std::cout << "clear" << std::endl;
            clear_move_blocks(blocks_affected);
            return e_create_move::ABORT;
        }
    }

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }


    std::cout << "number of affected blocks: " << blocks_affected.num_moved_blocks << std::endl;
    return create_move;
}