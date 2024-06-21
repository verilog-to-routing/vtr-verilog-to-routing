
#include "directed_moves_util.h"
#include "centroid_move_generator.h"

void get_coordinate_of_pin(ClusterPinId pin, t_physical_tile_loc& tile_loc) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int pnum = tile_pin_index(pin);
    ClusterBlockId block = cluster_ctx.clb_nlist.pin_block(pin);

    tile_loc.x = place_ctx.block_locs[block].loc.x + physical_tile_type(block)->pin_width_offset[pnum];
    tile_loc.y = place_ctx.block_locs[block].loc.y + physical_tile_type(block)->pin_height_offset[pnum];
    tile_loc.layer_num = place_ctx.block_locs[block].loc.layer;

    tile_loc.x = std::max(std::min(tile_loc.x, (int)grid.width() - 2), 1);  //-2 for no perim channels
    tile_loc.y = std::max(std::min(tile_loc.y, (int)grid.height() - 2), 1); //-2 for no perim channels
}

void calculate_centroid_loc(ClusterBlockId b_from,
                            bool timing_weights,
                            t_pl_loc& centroid,
                            const PlacerCriticalities* criticalities,
                            bool noc_attraction_enabled,
                            float noc_attraction_weight) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    t_physical_tile_loc tile_loc;
    int ipin;
    float acc_weight = 0;
    float acc_x = 0;
    float acc_y = 0;
    float acc_layer = 0;
    float weight = 1;

    int from_block_layer_num = g_vpr_ctx.placement().block_locs[b_from].loc.layer;
    VTR_ASSERT(from_block_layer_num != OPEN);

    //iterate over the from block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);

        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            continue;
        }

        /* Ignore the special case nets which only connects a block to itself  *
         * Experimentally, it was found that this case greatly degrade QoR     */
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() == 1) {
            ClusterBlockId source = cluster_ctx.clb_nlist.net_driver_block(net_id);
            ClusterPinId sink_pin = *cluster_ctx.clb_nlist.net_sinks(net_id).begin();
            ClusterBlockId sink = cluster_ctx.clb_nlist.pin_block(sink_pin);
            if (sink == source) {
                continue;
            }
        }

        //if the pin is driver iterate over all the sinks
        if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
            if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                continue;

            for (auto sink_pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                /* Ignore if one of the sinks is the block itself      *
                 * This case rarely happens but causes QoR degradation */
                if (pin_id == sink_pin_id)
                    continue;
                ipin = cluster_ctx.clb_nlist.pin_net_index(sink_pin_id);
                if (timing_weights) {
                    weight = criticalities->criticality(net_id, ipin);
                } else {
                    weight = 1;
                }

                get_coordinate_of_pin(sink_pin_id, tile_loc);

                acc_x += tile_loc.x * weight;
                acc_y += tile_loc.y * weight;
                acc_layer += tile_loc.layer_num * weight;
                acc_weight += weight;
            }
        }

        //else the pin is sink --> only care about its driver
        else {
            ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            if (timing_weights) {
                weight = criticalities->criticality(net_id, ipin);
            } else {
                weight = 1;
            }

            ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);

            get_coordinate_of_pin(source_pin, tile_loc);

            acc_x += tile_loc.x * weight;
            acc_y += tile_loc.y * weight;
            acc_layer += tile_loc.layer_num * weight;
            acc_weight += weight;
        }
    }

    if (noc_attraction_enabled) {
        NocGroupId noc_grp_id = CentroidMoveGenerator::get_cluster_noc_group(b_from);

        // check if the block belongs to a NoC group
        if (noc_grp_id != NocGroupId::INVALID()) {
            // get the routers in the associated NoC group
            const auto& noc_routers = CentroidMoveGenerator::get_noc_group_routers(noc_grp_id);
            float single_noc_weight = (acc_weight * noc_attraction_weight) / (float)noc_routers.size();

            acc_x *= (1.0f - noc_attraction_weight);
            acc_y *= (1.0f - noc_attraction_weight);
            acc_weight *= (1.0f - noc_attraction_weight);

            for (ClusterBlockId router_blk_id : noc_routers) {
                t_block_loc router_loc = place_ctx.block_locs[router_blk_id];
                acc_x += router_loc.loc.x * single_noc_weight;
                acc_y += router_loc.loc.y * single_noc_weight;
                acc_weight += single_noc_weight;
            }
        }
    }

    //Calculate the centroid location
    centroid.x = (int)std::round(acc_x / acc_weight);
    centroid.y = (int)std::round(acc_y / acc_weight);
    centroid.layer = (int)std::round(acc_layer / acc_weight);
}

static std::map<std::string, e_reward_function> available_reward_function = {
    {"basic", BASIC},
    {"nonPenalizing_basic", NON_PENALIZING_BASIC},
    {"runtime_aware", RUNTIME_AWARE},
    {"WLbiased_runtime_aware", WL_BIASED_RUNTIME_AWARE}};

e_reward_function string_to_reward(const std::string& st) {
    return available_reward_function[st];
}
