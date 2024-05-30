//
// Created by amin on 11/27/23.
//

#include <cmath>
#include <vector>
#include <queue>
#include <ctime>
#include "router_lookahead_compressed_map.h"
#include "connection_router_interface.h"
#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "globals.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_geometry.h"
#include "router_lookahead_map.h"
#include "router_lookahead_map_utils.h"
#include "rr_graph2.h"
#include "rr_graph.h"
#include "route_common.h"

vtr::Matrix<int> compressed_loc_index_map;
std::unordered_map<int, std::unordered_set<int>> sample_locations;

t_compressed_wire_cost_map f_compressed_wire_cost_map;

static int initialize_compressed_loc_structs(const std::vector<t_segment_inf>& segment_inf_vec);

static void compute_router_wire_compressed_lookahead(const std::vector<t_segment_inf>& segment_inf_vec);

/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_compressed_lookahead_map_costs(int from_layer_num, int segment_index, e_rr_type chan_type, util::t_routing_cost_map& routing_cost_map);

/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_compressed_lookahead_entries(const std::map<int, std::set<int>>& sorted_sample_loc, int segment_index, e_rr_type chan_type);

/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static util::Cost_Entry get_nearby_cost_entry_compressed_lookahead(int from_layer_num,
                                                                   int x,
                                                                   int y,
                                                                   int to_layer_num,
                                                                   int segment_index,
                                                                   int chan_index);

static util::Cost_Entry get_nearby_cost_entry_average_neighbour(const std::map<int, std::set<int>>& sorted_sample_loc,
                                                                int from_layer_num,
                                                                int missing_dx,
                                                                int missing_dy,
                                                                int to_layer_num,
                                                                int segment_index,
                                                                int chan_index);

static util::Cost_Entry get_wire_cost_entry_compressed_lookahead(e_rr_type rr_type, int seg_index, int from_layer_num, int delta_x, int delta_y, int to_layer_num);

static int initialize_compressed_loc_structs(const std::vector<t_segment_inf>& segment_inf_vec) {
    const auto& grid = g_vpr_ctx.device().grid;
    compressed_loc_index_map.resize({grid.width(), grid.height()}, OPEN);

    int max_seg_lenght = std::numeric_limits<int>::min();

    for (const auto& segment : segment_inf_vec) {
        if (!segment.longline) {
            max_seg_lenght = std::max(max_seg_lenght, segment.length);
        }
    }
    VTR_ASSERT(max_seg_lenght != std::numeric_limits<int>::min());

    int grid_width = static_cast<int>(grid.width());
    int grid_height = static_cast<int>(grid.height());

    int sample_point_num = 0;
    for (int x = 0; x < grid_width;) {
        int x_step = -1;
        if (x < 2 * max_seg_lenght) {
            x_step = 1;
        } else if (x < 4 * max_seg_lenght) {
            x_step = 2;
        } else if (x < 8 * max_seg_lenght) {
            x_step = 4;
        } else {
            x_step = 8;
        }
        for (int y = 0; y < grid_height;) {
            int y_step = -1;
            if (y < 2 * max_seg_lenght) {
                y_step = 1;
            } else if (y < 4 * max_seg_lenght) {
                y_step = 2;
            } else if (y < 8 * max_seg_lenght) {
                y_step = 4;
            } else {
                y_step = 8;
            }

            if (sample_locations.count(x) == 0) {
                sample_locations[x] = std::unordered_set<int>();
            }
            sample_locations[x].insert(y);

            int step = std::max(x_step, y_step);
            int sample_region_x_max = std::min(x + step, grid_width);
            int sample_region_y_max = std::min(y + step, grid_height);

            for (int sample_x = x; sample_x < sample_region_x_max; sample_x++) {
                for (int sample_y = y; sample_y < sample_region_y_max; sample_y++) {
                    compressed_loc_index_map[sample_x][sample_y] = sample_point_num;
                }
            }

            sample_point_num++;
            y += step;
        }
        x += x_step;
    }

    return sample_point_num;
}

static void compute_router_wire_compressed_lookahead(const std::vector<t_segment_inf>& segment_inf_vec) {
    vtr::ScopedStartFinishTimer timer("Computing wire lookahead");

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    int num_sampling_points = initialize_compressed_loc_structs(segment_inf_vec);

    f_compressed_wire_cost_map = t_compressed_wire_cost_map({static_cast<unsigned long>(grid.get_num_layers()),
                                                             2,
                                                             segment_inf_vec.size(),
                                                             static_cast<unsigned long>(grid.get_num_layers()),
                                                             static_cast<unsigned long>(num_sampling_points)});

    int longest_seg_length = 0;
    for (const auto& seg_inf : segment_inf_vec) {
        longest_seg_length = std::max(longest_seg_length, seg_inf.length);
    }

    std::map<int, std::set<int>> sorted_sample_loc;
    for (const auto& sample_loc : sample_locations) {
        sorted_sample_loc[sample_loc.first] = std::set<int>(sample_loc.second.begin(), sample_loc.second.end());
    }
    //Profile each wire segment type
    for (int from_layer_num = 0; from_layer_num < grid.get_num_layers(); from_layer_num++) {
        for (const auto& segment_inf : segment_inf_vec) {
            std::map<t_rr_type, std::vector<RRNodeId>> sample_nodes;
            std::vector<e_rr_type> chan_types;
            if (segment_inf.parallel_axis == X_AXIS)
                chan_types.push_back(CHANX);
            else if (segment_inf.parallel_axis == Y_AXIS)
                chan_types.push_back(CHANY);
            else //Both for BOTH_AXIS segments and special segments such as clock_networks we want to search in both directions.
                chan_types.insert(chan_types.end(), {CHANX, CHANY});

            for (e_rr_type chan_type : chan_types) {
                util::t_routing_cost_map routing_cost_map = util::get_routing_cost_map(longest_seg_length,
                                                                                       from_layer_num,
                                                                                       chan_type,
                                                                                       segment_inf,
                                                                                       sample_locations,
                                                                                       false);
                if (routing_cost_map.empty()) {
                    continue;
                }

                /* boil down the cost list in routing_cost_map at each coordinate to a representative cost entry and store it in the lookahead
                 * cost map */
                set_compressed_lookahead_map_costs(from_layer_num, segment_inf.seg_index, chan_type, routing_cost_map);

                /* fill in missing entries in the lookahead cost map by copying the closest cost entries (cost map was computed based on
                 * a reference coordinate > (0,0) so some entries that represent a cross-chip distance have not been computed) */
                fill_in_missing_compressed_lookahead_entries(sorted_sample_loc, segment_inf.seg_index, chan_type);
            }
        }
    }
}

static void set_compressed_lookahead_map_costs(int from_layer_num, int segment_index, e_rr_type chan_type, util::t_routing_cost_map& routing_cost_map) {
    int chan_index = 0;
    if (chan_type == CHANY) {
        chan_index = 1;
    }

    /* set the lookahead cost map entries with a representative cost entry from routing_cost_map */
    int to_layer_dim = static_cast<int>(routing_cost_map.dim_size(0));
    for (int to_layer = 0; to_layer < to_layer_dim; to_layer++) {
        int x_dim = static_cast<int>(routing_cost_map.dim_size(1));
        for (int ix = 0; ix < x_dim; ix++) {
            int y_dim = static_cast<int>(routing_cost_map.dim_size(2));
            for (int iy = 0; iy < y_dim; iy++) {
                if (sample_locations.find(ix) == sample_locations.end() || sample_locations.at(ix).find(iy) == sample_locations[ix].end()) {
                    continue;
                }
                util::Expansion_Cost_Entry& expansion_cost_entry = routing_cost_map[to_layer][ix][iy];
                int compressed_idx = compressed_loc_index_map[ix][iy];
                VTR_ASSERT(compressed_idx != OPEN);

                f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer][compressed_idx] = expansion_cost_entry.get_representative_cost_entry(util::e_representative_entry_method::SMALLEST);
            }
        }
    }
}

static void fill_in_missing_compressed_lookahead_entries(const std::map<int, std::set<int>>& sorted_sample_loc,
                                                         int segment_index,
                                                         e_rr_type chan_type) {
    int chan_index = 0;
    if (chan_type == CHANY) {
        chan_index = 1;
    }

    auto& device_ctx = g_vpr_ctx.device();
    int grid_width = static_cast<int>(device_ctx.grid.width());
    int grid_height = static_cast<int>(device_ctx.grid.height());
    /* find missing cost entries and fill them in by copying a nearby cost entry */
    for (int from_layer_num = 0; from_layer_num < device_ctx.grid.get_num_layers(); from_layer_num++) {
        for (int to_layer_num = 0; to_layer_num < device_ctx.grid.get_num_layers(); ++to_layer_num) {
            for (int ix = 0; ix < grid_width; ix++) {
                for (int iy = 0; iy < grid_height; iy++) {
                    if (sample_locations.find(ix) == sample_locations.end() || sample_locations.at(ix).find(iy) == sample_locations[ix].end()) {
                        continue;
                    }
                    int compressed_idx = compressed_loc_index_map[ix][iy];
                    util::Cost_Entry cost_entry = f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][compressed_idx];

                    if (std::isnan(cost_entry.delay) && std::isnan(cost_entry.congestion)) {
                        util::Cost_Entry copied_entry = get_nearby_cost_entry_average_neighbour(sorted_sample_loc,
                                                                                                from_layer_num,
                                                                                                ix,
                                                                                                iy,
                                                                                                to_layer_num,
                                                                                                segment_index,
                                                                                                chan_index);
                        f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][compressed_idx] = copied_entry;
                    }
                }
            }
        }
    }
}

/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static util::Cost_Entry get_nearby_cost_entry_compressed_lookahead(int from_layer_num,
                                                                   int x,
                                                                   int y,
                                                                   int to_layer_num,
                                                                   int segment_index,
                                                                   int chan_index) {
    /* compute the slope from x,y to 0,0 and then move towards 0,0 by one unit to get the coordinates
     * of the cost entry to be copied */

    //VTR_ASSERT(x > 0 || y > 0); //Asertion fails in practise. TODO: debug

    float slope;
    if (x == 0) {
        slope = 1e12; //just a really large number
    } else if (y == 0) {
        slope = 1e-12; //just a really small number
    } else {
        slope = (float)y / (float)x;
    }

    int copy_x, copy_y;
    if (slope >= 1.0) {
        copy_y = y - 1;
        copy_x = vtr::nint((float)y / slope);
    } else {
        copy_x = x - 1;
        copy_y = vtr::nint((float)x * slope);
    }

    copy_y = std::max(copy_y, 0); //Clip to zero
    copy_x = std::max(copy_x, 0); //Clip to zero

    int compressed_idx = compressed_loc_index_map[copy_x][copy_y];

    util::Cost_Entry copy_entry = f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][compressed_idx];

    /* if the entry to be copied is also empty, recurse */
    if (std::isnan(copy_entry.delay) && std::isnan(copy_entry.congestion)) {
        if (copy_x == 0 && copy_y == 0) {
            copy_entry = util::Cost_Entry(0., 0.); //(0, 0) entry is invalid so set zero to terminate recursion
            // set zero if the source and sink nodes are on the same layer. If they are not, it means that there is no connection from the source node to
            // the other layer. This means that the connection should be set to a very large number
            if (from_layer_num == to_layer_num) {
                copy_entry = util::Cost_Entry(0., 0.);
            } else {
                copy_entry = util::Cost_Entry(std::numeric_limits<float>::max() / 1e12, std::numeric_limits<float>::max() / 1e12);
            }
        } else {
            copy_entry = get_nearby_cost_entry_compressed_lookahead(from_layer_num, copy_x, copy_y, to_layer_num, segment_index, chan_index);
        }
    }

    return copy_entry;
}

static util::Cost_Entry get_nearby_cost_entry_average_neighbour(const std::map<int, std::set<int>>& sorted_sample_loc,
                                                                int from_layer_num,
                                                                int missing_dx,
                                                                int missing_dy,
                                                                int to_layer_num,
                                                                int segment_index,
                                                                int chan_index) {
    int missing_point_idx = compressed_loc_index_map[missing_dx][missing_dy];
    VTR_ASSERT(missing_point_idx != OPEN);
    VTR_ASSERT(std::isnan(f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][missing_point_idx].delay));
    VTR_ASSERT(std::isnan(f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][missing_point_idx].congestion));

    auto missing_point_compressed_iter_x = sorted_sample_loc.lower_bound(missing_dx);
    if (missing_point_compressed_iter_x->first != missing_dx) {
        missing_point_compressed_iter_x--;
    }

    int neighbour_num = 0;
    float neighbour_delay_sum = 0;
    float neighbour_cong_sum = 0;

    int neighbour_x = OPEN;
    int neighbour_y = OPEN;

    if (missing_dx == 0 && missing_dy == 0) {
        return util::Cost_Entry(0., 0.);
    }

    std::array<int, 3> window = {-1, 0, 1};
    for (int dx : window) {
        auto dist_to_begin = std::distance(sorted_sample_loc.begin(), missing_point_compressed_iter_x);
        auto dist_to_end = std::distance(missing_point_compressed_iter_x, sorted_sample_loc.end());
        if (dx >= 0) {
            if (dx >= dist_to_end) {
                continue;
            }
        } else {
            if (std::abs(dx) > dist_to_begin) {
                continue;
            }
        }
        auto neighbour_sample_loc_x_pair = missing_point_compressed_iter_x;
        std::advance(neighbour_sample_loc_x_pair, dx);
        neighbour_x = neighbour_sample_loc_x_pair->first;
        for (int dy : window) {
            const auto& sampling_column = sorted_sample_loc.at(neighbour_x);
            auto missing_point_compressed_iter_y = sampling_column.lower_bound(missing_dy);
            if ((*missing_point_compressed_iter_y) != missing_dy) {
                missing_point_compressed_iter_y--;
            }
            dist_to_begin = std::distance(sampling_column.begin(), missing_point_compressed_iter_y);
            dist_to_end = std::distance(missing_point_compressed_iter_y, sampling_column.end());
            if (dy >= 0) {
                if (dy >= dist_to_end) {
                    continue;
                }
            } else {
                if (std::abs(dy) > dist_to_begin) {
                    continue;
                }
            }
            std::advance(missing_point_compressed_iter_y, dy);
            neighbour_y = *missing_point_compressed_iter_y;
            int neighbour_compressed_idx = compressed_loc_index_map[neighbour_x][neighbour_y];
            util::Cost_Entry copy_entry = f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer_num][neighbour_compressed_idx];
            if (std::isnan(copy_entry.delay) || std::isnan(copy_entry.congestion)) {
                continue;
            }
            neighbour_delay_sum += copy_entry.delay;
            neighbour_cong_sum += copy_entry.congestion;
            neighbour_num += 1;
        }
    }

    if (neighbour_num >= 3) {
        return {neighbour_delay_sum / static_cast<float>(neighbour_num),
                neighbour_cong_sum / static_cast<float>(neighbour_num)};
    } else {
        return get_nearby_cost_entry_compressed_lookahead(from_layer_num, missing_dx, missing_dy, to_layer_num, segment_index, chan_index);
    }
}

static util::Cost_Entry get_wire_cost_entry_compressed_lookahead(e_rr_type rr_type, int seg_index, int from_layer_num, int delta_x, int delta_y, int to_layer_num) {
    VTR_ASSERT_SAFE(rr_type == CHANX || rr_type == CHANY);

    int chan_index = 0;
    if (rr_type == CHANY) {
        chan_index = 1;
    }

    int compressed_idx = compressed_loc_index_map[delta_x][delta_y];
    VTR_ASSERT_SAFE(from_layer_num < (int)f_compressed_wire_cost_map.dim_size(0));
    VTR_ASSERT_SAFE(to_layer_num < (int)f_compressed_wire_cost_map.dim_size(3));
    VTR_ASSERT_SAFE(compressed_idx < (int)f_compressed_wire_cost_map.dim_size(4));

    return f_compressed_wire_cost_map[from_layer_num][chan_index][seg_index][to_layer_num][compressed_idx];
}

/******** Interface class member function definitions ********/
CompressedMapLookahead::CompressedMapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat) {}

float CompressedMapLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    t_rr_type from_rr_type = rr_graph.node_type(current_node);

    float delay_cost = 0.;
    float cong_cost = 0.;

    if (from_rr_type == CHANX || from_rr_type == CHANY || from_rr_type == SOURCE || from_rr_type == OPIN) {
        // Get the total cost using the combined delay and congestion costs
        std::tie(delay_cost, cong_cost) = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
        return delay_cost + cong_cost;
    } else if (from_rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

std::pair<float, float> CompressedMapLookahead::get_expected_delay_and_cong(RRNodeId from_node,
                                                                            RRNodeId to_node,
                                                                            const t_conn_cost_params& params,
                                                                            float /* R_upstream */) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    int from_layer_num = rr_graph.node_layer(from_node);
    int to_layer_num = rr_graph.node_layer(to_node);
    auto [delta_x, delta_y] = util::get_xy_deltas(from_node, to_node);
    delta_x = abs(delta_x);
    delta_y = abs(delta_y);

    float expected_delay_cost = std::numeric_limits<float>::infinity();
    float expected_cong_cost = std::numeric_limits<float>::infinity();

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == SOURCE || from_type == OPIN) {
        //When estimating costs from a SOURCE/OPIN we look-up to find which wire types (and the
        //cost to reach them) in src_opin_delays. Once we know what wire types are
        //reachable, we query the f_wire_cost_map (i.e. the wire lookahead) to get the final
        //delay to reach the sink.

        t_physical_tile_type_ptr from_tile_type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(from_node),
                                                                                     rr_graph.node_ylow(from_node),
                                                                                     from_layer_num});

        auto from_tile_index = std::distance(&device_ctx.physical_tile_types[0], from_tile_type);

        auto from_ptc = rr_graph.node_ptc_num(from_node);

        std::tie(expected_delay_cost, expected_cong_cost) = util::get_cost_from_src_opin(src_opin_delays[from_layer_num][from_tile_index][from_ptc][to_layer_num],
                                                                                         delta_x,
                                                                                         delta_y,
                                                                                         to_layer_num,
                                                                                         get_wire_cost_entry_compressed_lookahead);

        expected_delay_cost *= params.criticality;
        expected_cong_cost *= (1 - params.criticality);

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_delay_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(from_node, is_flat_).c_str(),
                                            describe_rr_node(rr_graph,
                                                             device_ctx.grid,
                                                             device_ctx.rr_indexed_data,
                                                             from_node,
                                                             is_flat_)
                                                .c_str())
                                .c_str());

    } else if (from_type == CHANX || from_type == CHANY) {
        //When estimating costs from a wire, we directly look-up the result in the wire lookahead (f_wire_cost_map)

        auto from_cost_index = rr_graph.node_cost_index(from_node);
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        VTR_ASSERT(from_seg_index >= 0);

        /* now get the expected cost from our lookahead map */
        util::Cost_Entry cost_entry = get_wire_cost_entry_compressed_lookahead(from_type,
                                                                               from_seg_index,
                                                                               from_layer_num,
                                                                               delta_x,
                                                                               delta_y,
                                                                               to_layer_num);
        expected_delay_cost = cost_entry.delay;
        expected_cong_cost = cost_entry.congestion;

        VTR_ASSERT_SAFE_MSG(std::isfinite(expected_delay_cost),
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(from_node, is_flat_).c_str(),
                                            describe_rr_node(rr_graph,
                                                             device_ctx.grid,
                                                             device_ctx.rr_indexed_data,
                                                             from_node,
                                                             is_flat_)
                                                .c_str())
                                .c_str());
        expected_delay_cost = cost_entry.delay * params.criticality;
        expected_cong_cost = cost_entry.congestion * (1 - params.criticality);
    } else if (from_type == IPIN) { /* Change if you're allowing route-throughs */
        return std::make_pair(0., device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return std::make_pair(0., 0.);
    }

    return std::make_pair(expected_delay_cost, expected_cong_cost);
}

void CompressedMapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing router lookahead map");
    //First compute the delay map when starting from the various wire types
    //(CHANX/CHANY)in the routing architecture
    compute_router_wire_compressed_lookahead(segment_inf);

    //Next, compute which wire types are accessible (and the cost to reach them)
    //from the different physical tile type's SOURCEs & OPINs
    this->src_opin_delays = util::compute_router_src_opin_lookahead(is_flat_);
}

void CompressedMapLookahead::write(const std::string& file_name) const {
    if (vtr::check_file_name_extension(file_name, ".csv")) {
        std::vector<int> wire_cost_map_size(f_compressed_wire_cost_map.ndims());
        for (size_t i = 0; i < f_compressed_wire_cost_map.ndims(); ++i) {
            wire_cost_map_size[i] = static_cast<int>(f_compressed_wire_cost_map.dim_size(i));
        }
        dump_readable_router_lookahead_map(file_name, wire_cost_map_size, get_wire_cost_entry_compressed_lookahead);

    } else {
        VTR_ASSERT(vtr::check_file_name_extension(file_name, ".capnp"));
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::write for .bin format unimplemented");
    }
}
