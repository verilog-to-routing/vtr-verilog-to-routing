#include "weighted_median_move_generator.h"
#include "globals.h"
#include <algorithm>
#include "math.h"


static void get_bb_cost_for_net_excluding_block(ClusterNetId net_id, t_bb_cost* coords, ClusterBlockId block_id, ClusterPinId moving_pin_id, const PlacerCriticalities* criticalities, bool & skip_net);


//static void get_bb_for_net_excluding_block(ClusterNetId net_id, t_bb* coords, ClusterBlockId block_id);

//static void get_bb_cost_sink_for_net_excluding_block(ClusterNetId net_id, t_bb_cost* coords, ClusterBlockId block_id, ClusterPinId moving_pin_id);

e_create_move WeightedMedianMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim ,
    std::vector<int>& X_coord, std::vector<int>& Y_coord, int &,int place_high_fanout_net, const PlacerCriticalities* criticalities) {

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();


    /* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();
/*    for(int i =0; i < 10; i++){
        b_from  = pick_from_block();
        if (!b_from) {
        return e_create_move::ABORT; //No movable block found
        }
        if(cluster_ctx.clb_nlist.block_type(b_from)->index != 1){
            break;
        }
    }
*/

    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }
    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the median region */
    t_pl_loc to;
    //float temp_cost;
    //int ipin;

    t_bb_cost coords;
    t_bb temp_coords,limit_coords;
    X_coord.clear();
    Y_coord.clear();
    bool skip_net;

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;
        if(int(cluster_ctx.clb_nlist.net_pins(net_id).size()) >  place_high_fanout_net)
            continue;
/*
        //if the moving block is a sink, weight all the terminals of the input net by the criticality of the moving block net pin
        if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER)
            get_bb_cost_for_net_excluding_block(net_id, &coords, b_from, pin_id);

        else{
            ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            temp_cost = get_timing_place_crit(net_id, ipin);
            get_bb_for_net_excluding_block(net_id, &temp_coords, b_from, pin_id);
            coords.xmin = std::make_pair(temp_coords.xmin,temp_cost);
            coords.xmax = std::make_pair(temp_coords.xmax,temp_cost);
            coords.ymin = std::make_pair(temp_coords.ymin,temp_cost);
            coords.ymax = std::make_pair(temp_coords.ymax,temp_cost);

        }
*/
/*
        //if the moving block is a sink, weight the driver of the net by the criticality of the moving block net pin, all the other terminals of the nets are weighted by 0.1

        if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER)
            get_bb_cost_for_net_excluding_block(net_id, &coords, b_from, pin_id);
        else
            get_bb_cost_sink_for_net_excluding_block(net_id, &coords, b_from, pin_id);
*/
        //all net pins are weighted with their own crititcalities
        get_bb_cost_for_net_excluding_block(net_id, &coords, b_from, pin_id, criticalities, skip_net);
        if(skip_net)
            continue;

        X_coord.insert(X_coord.end(),ceil(coords.xmin.second*10), coords.xmin.first);
        X_coord.insert(X_coord.end(),ceil(coords.xmax.second*10), coords.xmax.first);
        Y_coord.insert(Y_coord.end(),ceil(coords.ymin.second*10), coords.ymin.first);
        Y_coord.insert(Y_coord.end(),ceil(coords.ymax.second*10), coords.ymax.first);
    }


    if((X_coord.size()==0) || (Y_coord.size()==0))
        return e_create_move::ABORT;

    std::sort(X_coord.begin(),X_coord.end());
    std::sort(Y_coord.begin(),Y_coord.end());

    if(X_coord.size() == 1){
        limit_coords.xmin = X_coord[floor((X_coord.size()-1)/2)];
        limit_coords.xmax = limit_coords.xmin;
    }
    else{
       limit_coords.xmin = X_coord[floor((X_coord.size()-1)/2)];
       limit_coords.xmax = X_coord[floor((X_coord.size()-1)/2)+1];
    }

    if(Y_coord.size() ==1){
        limit_coords.ymin = Y_coord[floor((Y_coord.size()-1)/2)];
        limit_coords.ymax = limit_coords.ymin;
    }
    else{
       limit_coords.ymin = Y_coord[floor((Y_coord.size()-1)/2)];
       limit_coords.ymax = Y_coord[floor((Y_coord.size()-1)/2)+1];
    }

    t_pl_loc median_point;
    median_point.x = (limit_coords.xmin + limit_coords.xmax)/2;
    median_point.y = (limit_coords.ymin + limit_coords.ymax)/2;
    if(!find_to_loc_centroid(cluster_from_type, rlim, from, median_point, to)){
        return e_create_move::ABORT;
    }
    return ::create_move(blocks_affected, b_from, to);
}



/* This routine finds the bounding box of a net from scratch excluding   *
 * a specific block. This is very useful in some directed moves.         *
 * It updates coordinates of the bb                                      */
static void get_bb_cost_for_net_excluding_block(ClusterNetId net_id, t_bb_cost* coords, ClusterBlockId , ClusterPinId moving_pin_id, const PlacerCriticalities* criticalities, bool& skip_net) {
    int pnum, x, y, xmin, xmax, ymin, ymax;
    float xmin_cost,xmax_cost,ymin_cost,ymax_cost, cost;

    skip_net = true;

    xmin=0;
    xmax=0;
    ymin=0;
    ymax=0;
    cost = 0.0;
    xmin_cost=0.0;
    xmax_cost=0.0;
    ymin_cost=0.0;
    ymax_cost=0.0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum;
    bool is_first_block = true;
    int ipin ;
    for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);

        //if(bnum != block_id)
        if(pin_id != moving_pin_id)
        {
            skip_net = false;
            pnum = tile_pin_index(pin_id);
            if(cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER){
                ipin = cluster_ctx.clb_nlist.pin_net_index(moving_pin_id);
            }
            else{
                ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            }

            VTR_ASSERT(pnum >= 0);
            x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
            y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

            x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
            y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

            cost = criticalities->criticality(net_id, ipin);
            if(is_first_block){
                xmin = x;
                xmin_cost = cost;
                ymin = y;
                ymin_cost = cost;
                xmax = x;
                xmax_cost = cost;
                ymax = y;
                ymax_cost = cost;
                is_first_block = false;
            }
            else {
                if (x < xmin) {
                    xmin = x;
                    xmin_cost = cost;
                } else if (x > xmax) {
                    xmax = x;
                    xmax_cost = cost;
                }

                if (y < ymin) {
                    ymin = y;
                    ymin_cost = cost;
                } else if (y > ymax) {
                    ymax = y;
                    ymax_cost = cost;
                }
            }
        }
    }

    /* Copy the coordinates and number on edges information into the proper   *
     * structures.                                                            */
    coords->xmin = std::make_pair(xmin,xmin_cost);
    coords->xmax = std::make_pair(xmax,xmax_cost);
    coords->ymin = std::make_pair(ymin,ymin_cost);
    coords->ymax = std::make_pair(ymax,ymax_cost);
}

/*
static void get_bb_for_net_excluding_block(ClusterNetId net_id, t_bb* coords, ClusterBlockId block_id) {
    int pnum, x, y, xmin, xmax, ymin, ymax;
    xmin=0;
    xmax=0;
    ymin=0;
    ymax=0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum;
    bool is_first_block = true;
    for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        if(bnum != block_id)
        {
            pnum = tile_pin_index(pin_id);
            VTR_ASSERT(pnum >= 0);
            x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
            y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

            x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
            y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

            if(is_first_block){
                xmin = x;
                ymin = y;
                xmax = x;
                ymax = y;
                is_first_block = false;
            }
            else {
                if (x < xmin) {
                    xmin = x;
                } else if (x > xmax) {
                    xmax = x;
                }

                if (y < ymin) {
                    ymin = y;
                } else if (y > ymax) {
                    ymax = y;
                }
            }
        }
    }

    coords->xmin = xmin;
    coords->xmax = xmax;
    coords->ymin = ymin;
    coords->ymax = ymax;
}




static void get_bb_cost_sink_for_net_excluding_block(ClusterNetId net_id, t_bb_cost* coords, ClusterBlockId , ClusterPinId moving_pin_id) {
    int pnum, x, y, xmin, xmax, ymin, ymax;
    float xmin_cost,xmax_cost,ymin_cost,ymax_cost, cost;
    xmin=0;
    xmax=0;
    ymin=0;
    ymax=0;
    cost = 0.0;
    xmin_cost=0.0;
    xmax_cost=0.0;
    ymin_cost=0.0;
    ymax_cost=0.0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;




    ClusterBlockId bnum;
    bool is_first_block = true;
    int ipin ;

    for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);

        //if(bnum != block_id)
        if(pin_id != moving_pin_id)
        {
            pnum = tile_pin_index(pin_id);
            if(cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER){
                ipin = cluster_ctx.clb_nlist.pin_net_index(moving_pin_id);
                cost = get_timing_place_crit(net_id, ipin);
            }
            else{
                ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
                cost = 0.1;
            }

            VTR_ASSERT(pnum >= 0);
            x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
            y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

            x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
            y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

            if(is_first_block){
                xmin = x;
                xmin_cost = cost;
                ymin = y;
                ymin_cost = cost;
                xmax = x;
                xmax_cost = cost;
                ymax = y;
                ymax_cost = cost;
                is_first_block = false;
            }
            else {
                if (x < xmin) {
                    xmin = x;
                    xmin_cost = cost;
                } else if (x > xmax) {
                    xmax = x;
                    xmax_cost = cost;
                }

                if (y < ymin) {
                    ymin = y;
                    ymin_cost = cost;
                } else if (y > ymax) {
                    ymax = y;
                    ymax_cost = cost;
                }
            }
        }
    }

    coords->xmin = std::make_pair(xmin,xmin_cost);
    coords->xmax = std::make_pair(xmax,xmax_cost);
    coords->ymin = std::make_pair(ymin,ymin_cost);
    coords->ymax = std::make_pair(ymax,ymax_cost);
}
*/
