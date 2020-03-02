#include "weighted_centroid_move_generator.h"
#include "globals.h"
#include <algorithm>
//#include "math.h"

bool sort_by_weights(const std::pair<int,float> &a, const std::pair<int,float> &b);


e_create_move WeightedCentroidMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim,
    std::vector<int>& X_coord, std::vector<int>& Y_coord,  std::vector<int>&, int & ,int place_high_fanout_net) {
    /* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();
    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the weighted centroid */
    t_pl_loc to;

    t_bb limit_coords;
    X_coord.clear();
    Y_coord.clear();
    ClusterNetId net_id;
    ClusterBlockId bnum;
    int x,y;
    float acc_weight = 0;
    float acc_x = 0;
    float acc_y = 0;
    float weight,center_x,center_y;

    //iterate over the from block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        net_id = cluster_ctx.clb_nlist.pin_net(pin_id);

        //if the pin is driver iterate over all the sinks
        if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
            if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                continue;
            if(int(cluster_ctx.clb_nlist.net_pins(net_id).size()) >  place_high_fanout_net)
                continue;

            for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
                weight = get_timing_place_crit(net_id, ipin); //*comp_td_point_to_point_delay(delay_model, net_id, ipin) 
                acc_weight += weight;

                ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net_id, ipin);
                ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(sink_pin);

                x = place_ctx.block_locs[sink_block].loc.x + physical_tile_type(sink_block)->pin_width_offset[ipin];
                y = place_ctx.block_locs[sink_block].loc.y + physical_tile_type(sink_block)->pin_height_offset[ipin];

                x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
                y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

                acc_x += x*weight;
                acc_y += y*weight;
            }
        }

        //else the pin is sink --> only care about its driver
        else  {
            int net_pin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            weight = get_timing_place_crit(net_id, net_pin);
            acc_weight += weight;

            ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
            ClusterBlockId source_block = cluster_ctx.clb_nlist.pin_block(source_pin);

            x = place_ctx.block_locs[source_block].loc.x ;
            y = place_ctx.block_locs[source_block].loc.y ;

            x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
            y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

            acc_x += x*weight;
            acc_y += y*weight;
        }
    }
    
    //Calculate the centroid location
    center_x = acc_x/acc_weight;
    center_y = acc_y/acc_weight;

    t_pl_loc centroid;
    centroid.x = center_x;
    centroid.y = center_y;

    if (!find_to_loc_centroid(cluster_from_type, rlim, centroid, to)) {
        return e_create_move::ABORT;
    }

    return ::create_move(blocks_affected, b_from, to);
}