
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

    e_create_move create_move = e_create_move::ABORT;

    const std::vector<ClusterBlockId>& router_blk_ids = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();

    const ClusterBlockId selected_noc_router_blk_id = router_blk_ids[vtr::irand(router_blk_ids.size() - 1)];
    const NocGroupId noc_group_id = place_ctx.noc_router_to_noc_group.find(selected_noc_router_blk_id)->second;

    const auto& group_routers = place_ctx.noc_group_routers[noc_group_id];
    const auto& group_clusters = place_ctx.noc_group_clusters[noc_group_id];

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    const int num_layers = grid.get_num_layers();

    //Collect the set of x/y locations for each instance of a block type
    //[group_noc_router_idx][logical_block_type][layer_num][0...num_instance_on_layer] -> (x, y), sub-tile
    std::vector<std::vector<std::vector<std::vector<std::pair<vtr::Point<int>, int>>>>> block_locations;
    block_locations.resize(group_routers.size());

    for (size_t group_noc_router_idx = 0; group_noc_router_idx < group_routers.size(); group_noc_router_idx++) {
        block_locations[group_noc_router_idx].resize(device_ctx.logical_block_types.size());
        for (int block_type_num = 0; block_type_num < (int)device_ctx.logical_block_types.size(); block_type_num++) {
            block_locations[group_noc_router_idx][block_type_num].resize(num_layers);
        }
    }


    for (size_t group_noc_router_idx = 0; group_noc_router_idx < group_routers.size(); group_noc_router_idx++) {
        ClusterBlockId router_blk_id = group_routers[group_noc_router_idx];
        t_block_loc router_loc = place_ctx.block_locs[router_blk_id];
        const int half_length = 10;
        int min_x = std::max<int>(router_loc.loc.x - half_length, 0);
        int max_x = std::min<int>(router_loc.loc.x + half_length, device_ctx.grid.width() - 1);
        int min_y = std::max<int>(router_loc.loc.y - half_length, 0);
        int max_y = std::min<int>(router_loc.loc.y + half_length, device_ctx.grid.height() - 1);
        int layer_num = router_loc.loc.layer;

        for (int x = min_x; x < max_x; ++x) {
            for (int y = min_y; y < max_y; ++y) {
                int width_offset = grid.get_width_offset({x, y, layer_num});
                int height_offset = grid.get_height_offset(t_physical_tile_loc(x, y, layer_num));
                if (width_offset == 0 && height_offset == 0) {
                    const auto& type = grid.get_physical_type({x, y, layer_num});
                    auto equivalent_sites = get_equivalent_sites_set(type);

                    for (auto& block : equivalent_sites) {

                        const auto& compressed_block_grid = place_ctx.compressed_block_grids[block->index];
                        std::vector<t_physical_tile_loc> router_compressed_loc = get_compressed_loc_approx(compressed_block_grid,
                                                                                                           {x, y, 0, layer_num},
                                                                                                           num_layers);

                        int sub_tile = has_empty_compatible_subtile(block, router_compressed_loc[layer_num]);
                        if (sub_tile >= 0) {
                            block_locations[group_noc_router_idx][block->index][layer_num].push_back({{x, y}, sub_tile});
                        }

                    }
                }
            }
        }
    }

    std::set<t_pl_loc> tried_locs;

    for (auto block_id : group_clusters) {
        int i_macro;
        get_imacro_from_iblk(&i_macro, block_id, place_ctx.pl_macros);
        if (i_macro != -1) {
            continue;
        }

        int group_noc_router_idx = vtr::irand(group_routers.size()-1);
        ClusterBlockId router_blk_id = group_routers[group_noc_router_idx];
        t_block_loc router_loc = place_ctx.block_locs[router_blk_id];

        int layer_num = router_loc.loc.layer;

        t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(block_id);

        size_t n_points = block_locations[group_noc_router_idx][block_type->index][layer_num].size();
        if (n_points == 0) {
//            std::cout << "No empty blocks around the selected NoC router" << std::endl;
            continue;
        }
        int point_idx = vtr::irand(n_points - 1);
        auto selected_empty_point = block_locations[group_noc_router_idx][block_type->index][layer_num][point_idx];

        t_pl_loc to_loc{selected_empty_point.first.x(), selected_empty_point.first.y(), selected_empty_point.second, layer_num};
        block_locations[group_noc_router_idx][block_type->index][layer_num].erase(block_locations[group_noc_router_idx][block_type->index][layer_num].begin() + point_idx);
        create_move = ::create_move(blocks_affected, block_id, to_loc);

        bool added = tried_locs.insert(to_loc).second;
        if (!added) {
            std::cout << "repetitive" << std::endl;
        }
        if (create_move == e_create_move::ABORT) {
            std::cout << "clear" << std::endl;
            clear_move_blocks(blocks_affected);
            return e_create_move::ABORT;
        }
    }



    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        std::cout << "illegal floorplan" << std::endl;
        return e_create_move::ABORT;
    }

    return create_move;
}