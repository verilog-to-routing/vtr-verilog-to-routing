#include "feasible_region_move_generator.h"
#include "globals.h"
#include <algorithm>
#include "math.h"


e_create_move FeasibleRegionMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float ,
    std::vector<int>& X_coord, std::vector<int>& Y_coord, std::vector<int>&, int &,int ) {

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    /* Pick a random block to be swapped with another random block.   */
    //ClusterBlockId b_from = pick_from_block();
    std::pair<ClusterNetId, int> crit_pin = highly_crit_pins[vtr::irand(highly_crit_pins.size())];
    ClusterBlockId b_from = cluster_ctx.clb_nlist.net_pin_block(crit_pin.first, crit_pin.second);

    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }

    //from block data
    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));


    /* Calculate the feasible region */
    t_pl_loc to;
    int ipin;
    ClusterBlockId bnum;
    int max_x, min_x, max_y, min_y;


    X_coord.clear();
    Y_coord.clear();
    //For critical input nodes, calculate the x&y min-max valus
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_input_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;

        ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
        if(get_timing_place_crit(net_id, ipin) > CRIT_LIMIT){
            bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
            X_coord.push_back(place_ctx.block_locs[bnum].loc.x);
            Y_coord.push_back(place_ctx.block_locs[bnum].loc.y);
        }
    }
    if(X_coord.size() != 0){
        max_x = *(std::max_element(X_coord.begin(),X_coord.end()));
        min_x = *(std::min_element(X_coord.begin(),X_coord.end()));
        max_y = *(std::max_element(Y_coord.begin(),Y_coord.end()));
        min_y = *(std::min_element(Y_coord.begin(),Y_coord.end()));
    }
    else{
        max_x = from.x;
        min_x = from.x;
        max_y = from.y;
        min_y = from.y;
    }


    //Get the most critical output of the node
    int xt,yt,pnum;
    float crit,highest_crit = 0;
    for(ClusterPinId driver_pin_id : cluster_ctx.clb_nlist.block_output_pins(b_from)){
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(driver_pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;

        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            crit = get_timing_place_crit(net_id, ipin);
            if(crit > highest_crit){
                bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
                pnum = tile_pin_index(pin_id);
                VTR_ASSERT(pnum >= 0);

                xt = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
                yt = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

                xt = std::max(std::min(xt, (int)grid.width() - 2), 1);  //-2 for no perim channels
                yt = std::max(std::min(yt, (int)grid.height() - 2), 1); //-2 for no perim channels
                if(crit == 1 )
                    break;
            }
        }
        if(crit == 1 )
            break;
    }

    //determine the feasible region
    t_bb FR_coords;
    if(xt < min_x){
        FR_coords.xmin = std::min(from.x, xt);
        FR_coords.xmax = std::max(from.x, min_x);
    }
    else if(xt < max_x){
        FR_coords.xmin = std::min(from.x, xt);
        FR_coords.xmax = std::max(from.x, xt);
    }
    else{
        FR_coords.xmin = std::min(from.x, max_x);
        FR_coords.xmax = std::max(from.x, xt);
    }


    if(yt < min_y){
        FR_coords.ymin = std::min(from.y, yt);
        FR_coords.ymax = std::max(from.y, min_y);
    }
    else if(yt < max_y){
        FR_coords.ymin = std::min(from.y, yt);
        FR_coords.ymax = std::max(from.y, yt);
    }
    else{
        FR_coords.ymin = std::min(from.y, max_y);
        FR_coords.ymax = std::max(from.y, yt);
    }

    if (!find_to_loc_median(cluster_from_type, &FR_coords, from, to)) {
        return e_create_move::ABORT;
    }
    return ::create_move(blocks_affected, b_from, to);
}
