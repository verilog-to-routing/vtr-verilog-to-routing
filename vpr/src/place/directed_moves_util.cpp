#include "directed_moves_util.h"

void calculate_centroid_loc(ClusterBlockId b_from, bool timing_weights, t_pl_loc& centroid, const PlacerCriticalities* criticalities){

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int x, y, pnum, ipin;
    float acc_weight = 0;
    float acc_x = 0;
    float acc_y = 0;
    float weight = 1;

    //iterate over the from block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
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
                if (pin_id == sink_pin_id)
                    continue;
                ipin = cluster_ctx.clb_nlist.pin_net_index(sink_pin_id);
                pnum = tile_pin_index(sink_pin_id);
                if(timing_weights){
                    weight = criticalities->criticality(net_id, ipin); 
                }
                else {
                    weight = 1;
                }

                ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(sink_pin_id);

                x = place_ctx.block_locs[sink_block].loc.x + physical_tile_type(sink_block)->pin_width_offset[pnum];
                y = place_ctx.block_locs[sink_block].loc.y + physical_tile_type(sink_block)->pin_height_offset[pnum];

                x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
                y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

                acc_x += x * weight;
                acc_y += y * weight; 
                acc_weight += weight;
            }
        }

        //else the pin is sink --> only care about its driver
        else {
            ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            if(timing_weights){
                weight = criticalities->criticality(net_id, ipin);
            }
            else {
                weight = 1;
            }

            ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
            ClusterBlockId source_block = cluster_ctx.clb_nlist.pin_block(source_pin);
            pnum = tile_pin_index(source_pin);

            x = place_ctx.block_locs[source_block].loc.x + physical_tile_type(source_block)->pin_width_offset[pnum];
            y = place_ctx.block_locs[source_block].loc.y + physical_tile_type(source_block)->pin_width_offset[pnum];

            x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
            y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

            acc_x += x * weight;
            acc_y += y * weight;
            acc_weight += weight;
        }
    }

    //Calculate the centroid location
    centroid.x = acc_x / acc_weight;
    centroid.y = acc_y / acc_weight;
}
