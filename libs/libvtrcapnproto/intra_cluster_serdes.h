//
// Created by amin on 1/17/23.
//

#ifndef VTR_INTRA_CLUSTER_SERDES_H
#define VTR_INTRA_CLUSTER_SERDES_H

#include <functional>
#include <vector>
#include <unordered_map>

#include "vtr_ndmatrix.h"
#include "vpr_error.h"
#include "matrix.capnp.h"
#include "map_lookahead.capnp.h"
#include "vpr_types.h"
#include "router_lookahead_map_utils.h"


// Generic function to convert from Matrix capnproto message to vtr::NdMatrix.
//
// Template arguments:
//  N = Number of matrix dimensions, must be fixed.
//  CapType = Source capnproto message type that is a single element the
//            Matrix capnproto message.
//  CType = Target C++ type that is a single element of vtr::NdMatrix.
//
// Arguments:
//  m_out = Target vtr::NdMatrix.
//  m_in = Source capnproto message reader.
//  copy_fun = Function to convert from CapType to CType.
void ToIntraClusterLookahead(std::unordered_map<t_physical_tile_type_ptr, util::t_ipin_primitive_sink_delays>& inter_tile_pin_primitive_pin_delay,
                             std::unordered_map<t_physical_tile_type_ptr, std::unordered_map<int, util::Cost_Entry>>& tile_min_cost,
                             const std::vector<t_physical_tile_type>& physical_tile_types,
                             const VprIntraClusterLookahead::Reader& intra_cluster_lookahead_builder) {

    inter_tile_pin_primitive_pin_delay.clear();
    tile_min_cost.clear();

    int num_tile_types = intra_cluster_lookahead_builder.getPhysicalTileNumPins().size();
    VTR_ASSERT(num_tile_types == (int)physical_tile_types.size());

    std::vector<int> tile_num_pins(num_tile_types);
    std::vector<int> tile_num_sinks(num_tile_types);

    for (int tile_idx = 0; tile_idx < num_tile_types; tile_idx++) {
        tile_num_pins[tile_idx] = intra_cluster_lookahead_builder.getPhysicalTileNumPins()[tile_idx];
        tile_num_sinks[tile_idx] = intra_cluster_lookahead_builder.getTileNumSinks()[tile_idx];
    }

    int num_seen_sinks = 0;
    int num_seen_pins = 0;
    for(int tile_type_idx = 0; tile_type_idx < num_tile_types; tile_type_idx++) {
        int cur_tile_num_pins = tile_num_pins[tile_type_idx];
        t_physical_tile_type_ptr physical_type_ptr = &physical_tile_types[tile_type_idx];
        inter_tile_pin_primitive_pin_delay[physical_type_ptr] = util::t_ipin_primitive_sink_delays(cur_tile_num_pins);
        for(int pin_num = 0; pin_num < cur_tile_num_pins; pin_num++) {
            inter_tile_pin_primitive_pin_delay[physical_type_ptr][pin_num].clear();
            int pin_num_sinks = intra_cluster_lookahead_builder.getPinNumSinks()[num_seen_pins];
            num_seen_pins++;
            for(int sink_idx = 0; sink_idx < pin_num_sinks; sink_idx++) {
                int sink_pin_num = intra_cluster_lookahead_builder.getPinSinks()[num_seen_sinks];
                auto cost = intra_cluster_lookahead_builder.getPinSinkCosts()[num_seen_sinks];
                inter_tile_pin_primitive_pin_delay[physical_type_ptr][pin_num].insert(std::make_pair(sink_pin_num,
                                                                                                     util::Cost_Entry(cost.getDelay(), cost.getCongestion())));
                num_seen_sinks++;
            }


        }
    }

    num_seen_sinks = 0;
    for(int tile_type_idx = 0; tile_type_idx < num_tile_types; tile_type_idx++) {
        int cur_tile_num_sinks = tile_num_sinks[tile_type_idx];
        t_physical_tile_type_ptr physical_type_ptr = &physical_tile_types[tile_type_idx];
        tile_min_cost[physical_type_ptr] = std::unordered_map<int, util::Cost_Entry>();
        for(int sink_idx = 0; sink_idx < cur_tile_num_sinks; sink_idx++) {
            int sink_num = intra_cluster_lookahead_builder.getTileSinks()[num_seen_sinks];
            auto cost = intra_cluster_lookahead_builder.getTileMinCosts()[num_seen_sinks];
            tile_min_cost[physical_type_ptr].insert(std::make_pair(sink_num,
                                                                   util::Cost_Entry(cost.getDelay(), cost.getCongestion())));
            num_seen_sinks++;

        }
    }


}

void FromIntraClusterLookahead(VprIntraClusterLookahead::Builder& intra_cluster_lookahead_builder,
                               const std::unordered_map<t_physical_tile_type_ptr, util::t_ipin_primitive_sink_delays>& inter_tile_pin_primitive_pin_delay,
                               const std::unordered_map<t_physical_tile_type_ptr, std::unordered_map<int, util::Cost_Entry>>& tile_min_cost,
                               const std::vector<t_physical_tile_type>& physical_tile_types) {

    ::capnp::List<int64_t>::Builder physical_tile_num_pin_arr_builder;
    ::capnp::List<int64_t>::Builder pin_num_sink_arr_builder;
    ::capnp::List<int64_t>::Builder pin_sink_arr_builder;
    ::capnp::List<VprMapCostEntry>::Builder pin_sink_cost_builder;
    ::capnp::List<int64_t>::Builder tile_num_sinks_builder;
    ::capnp::List<int64_t>::Builder tile_sinks_builder;
    ::capnp::List<VprMapCostEntry>::Builder tile_sink_min_cost_builder;

    int num_tile_types = physical_tile_types.size();

    physical_tile_num_pin_arr_builder = intra_cluster_lookahead_builder.initPhysicalTileNumPins(num_tile_types);

    // Count the number of pins for each tile
    {
        int total_num_pin = 0;
        for (const auto& tile_type : physical_tile_types) {
            const auto tile_pin_primitive_delay = inter_tile_pin_primitive_pin_delay.find(&tile_type);
            if (tile_pin_primitive_delay == inter_tile_pin_primitive_pin_delay.end()) {
                physical_tile_num_pin_arr_builder.set(tile_type.index, 0);
                continue;
            }
            int tile_num_pins = tile_pin_primitive_delay->second.size();
            physical_tile_num_pin_arr_builder.set(tile_type.index, tile_num_pins);
            total_num_pin += tile_num_pins;
        }

        pin_num_sink_arr_builder = intra_cluster_lookahead_builder.initPinNumSinks(total_num_pin);
    }

    // Count the number of sinks for each pin
    {
        int pin_num = 0;
        int total_pin_num_sinks = 0;
        for (const auto& tile_type : physical_tile_types) {
            const auto tile_pin_primitive_delay = inter_tile_pin_primitive_pin_delay.find(&tile_type);
            if (tile_pin_primitive_delay == inter_tile_pin_primitive_pin_delay.end()) {
                continue;
            }
            for (const auto& pin : tile_pin_primitive_delay->second) {
                int pin_num_sinks = pin.size();
                pin_num_sink_arr_builder.set(pin_num, pin_num_sinks);
                pin_num++;
                total_pin_num_sinks += pin_num_sinks;
            }
        }

        pin_sink_arr_builder = intra_cluster_lookahead_builder.initPinSinks(total_pin_num_sinks);
        pin_sink_cost_builder = intra_cluster_lookahead_builder.initPinSinkCosts(total_pin_num_sinks);
    }

    // Iterate over sinks of each pin and store the cost of getting to the sink from the respective pin and the sink ptc number
    {
        int pin_flat_sink_idx = 0;
        for (const auto& tile_type : physical_tile_types) {
            const auto tile_pin_primitive_delay = inter_tile_pin_primitive_pin_delay.find(&tile_type);
            if (tile_pin_primitive_delay == inter_tile_pin_primitive_pin_delay.end()) {
                continue;
            }
            for (const auto& pin : tile_pin_primitive_delay->second) {
                for (const auto& sink : pin) {
                    pin_sink_arr_builder.set(pin_flat_sink_idx, sink.first);
                    pin_sink_cost_builder[pin_flat_sink_idx].setDelay(sink.second.delay);
                    pin_sink_cost_builder[pin_flat_sink_idx].setCongestion(sink.second.congestion);
                    pin_flat_sink_idx++;
                }
            }
        }
    }


    // Store the information related to tile_min cost

    tile_num_sinks_builder = intra_cluster_lookahead_builder.initTileNumSinks(num_tile_types);

    // Count the number of sinks for each tile
    {
        int tile_total_num_sinks = 0;
        for (const auto& tile_type : physical_tile_types) {
            const auto tile_min_cost_entry = tile_min_cost.find(&tile_type);
            if (tile_min_cost_entry == tile_min_cost.end()) {
                tile_num_sinks_builder.set(tile_type.index, 0);
                continue;
            }
            int tile_num_sinks = (int)tile_min_cost_entry->second.size();
            tile_num_sinks_builder.set(tile_type.index, tile_num_sinks);
            tile_total_num_sinks += tile_num_sinks;
        }

        tile_sinks_builder = intra_cluster_lookahead_builder.initTileSinks(tile_total_num_sinks);
        tile_sink_min_cost_builder = intra_cluster_lookahead_builder.initTileMinCosts(tile_total_num_sinks);
    }

    // Iterate over sinks of each tile and store the minimum cost to get to that sink and the sink ptc number
    {
        int pin_flat_sink_idx = 0;
        for (const auto& tile_type : physical_tile_types) {
            const auto tile_min_cost_entry = tile_min_cost.find(&tile_type);
            if (tile_min_cost_entry == tile_min_cost.end()) {
                continue;
            }
            for (const auto& sink : tile_min_cost_entry->second) {
                tile_sinks_builder.set(pin_flat_sink_idx, sink.first);
                tile_sink_min_cost_builder[pin_flat_sink_idx].setDelay(sink.second.delay);
                tile_sink_min_cost_builder[pin_flat_sink_idx].setCongestion(sink.second.congestion);
                pin_flat_sink_idx++;
            }
        }
    }

}




#endif //VTR_INTRA_CLUSTER_SERDES_H
