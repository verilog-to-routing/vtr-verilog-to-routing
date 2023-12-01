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


struct SamplingRegion {
    SamplingRegion() = default;
    SamplingRegion(int x_max_, int y_max_, int x_min_, int y_min_, int step_)
        : x_max(x_max_)
        , y_max(y_max_)
        , x_min(x_min_)
        , y_min(y_min_)
        , step(step_) {}
    int x_max = OPEN;
    int y_max = OPEN;
    int x_min = OPEN;
    int y_min = OPEN;
    int step = OPEN;

    int width() const {
        return x_max - x_min;
    }

    int height() const {
        return y_max - y_min;
    }
};

static void initialize_compressed_loc_structs(std::vector<SamplingRegion>& sampling_regions, int num_sampling_points);

static std::vector<SamplingRegion> get_sampling_regions(const std::vector<t_segment_inf>& segment_inf);

/* sets the lookahead cost map entries based on representative cost entries from routing_cost_map */
static void set_compressed_lookahead_map_costs(int from_layer_num, int segment_index, e_rr_type chan_type, util::t_routing_cost_map& routing_cost_map);

/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_compressed_lookahead_entries(int segment_index, e_rr_type chan_type);

/* returns a cost entry in the f_wire_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static util::Cost_Entry get_nearby_cost_entry_compressed_lookahead(int from_layer_num,
                                                                   int x,
                                                                   int y,
                                                                   int to_layer_num,
                                                                   int segment_index,
                                                                   int chan_index);

static void initialize_compressed_loc_structs(std::vector<SamplingRegion>& sampling_regions, int num_sampling_points) {
    std::sort(sampling_regions.begin(), sampling_regions.end(), [](const SamplingRegion& a, const SamplingRegion& b) {
        VTR_ASSERT_DEBUG(a.width() == b.width() && a.height() == b.height());
        VTR_ASSERT_DEBUG(a.height() != 0 && b.height() != 0);

        if(a.y_max <= b.y_min) {
            return true;
        } else if (b.y_max <= a.y_min) {
            return false;
        } else{
            VTR_ASSERT_DEBUG(a.y_min == b.y_min);
            if (a.x_min < b.x_min) {
                return true;
            } else {
                return false;
            }
        }
    });

    const auto& grid = g_vpr_ctx.device().grid;
    compressed_loc_index_map.resize({grid.width(), grid.height()}, OPEN);

    int sample_point_num = 0;
    for (const auto& sample_region: sampling_regions) {
        int step = sample_region.step;
        int x_max = sample_region.x_max;
        int y_max = sample_region.y_max;
        int x_min = sample_region.x_min;
        int y_min = sample_region.y_min;
        for (int x = x_min; x < x_max; x += step) {
            for (int y = y_min; y < y_max; y += step) {
                if (sample_locations.count(x) == 0) {
                    sample_locations[x] = std::unordered_set<int>();
                }
                sample_locations[x].insert(y);
                compressed_loc_index_map[x][y] = sample_point_num;
                sample_point_num++;
            }
        }
    }
    VTR_ASSERT(sample_point_num == num_sampling_points);
}

static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_inf_vec)  {
    vtr::ScopedStartFinishTimer timer("Computing wire lookahead");

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    auto sampling_regions = get_sampling_regions(segment_inf_vec);

    int compresses_x_size = 0;
    int compressed_y_size = 0;
    for (const auto& sampling_region : sampling_regions) {
        int step = sampling_region.step;
        int num_x = (sampling_region.x_max - sampling_region.x_min) / step;
        int num_y = (sampling_region.y_max - sampling_region.y_min) / step;
        compresses_x_size += num_x;
        compressed_y_size += num_y;
    }

    initialize_compressed_loc_structs(sampling_regions, compresses_x_size * compressed_y_size);
}

static std::vector<SamplingRegion> get_sampling_regions(const std::vector<t_segment_inf>& segment_inf) {

    const auto& grid = g_vpr_ctx.device().grid;
    std::vector<SamplingRegion> sampling_regions;


    int grid_width = grid.width();
    int grid_height = grid.height();

    int max_seg_lenght = std::numeric_limits<int>::min();
    int min_seg_length = std::numeric_limits<int>::max();

    for (const auto& segment : segment_inf) {
        if (!segment.longline) {
            max_seg_lenght = std::max(max_seg_lenght, segment.length);
            min_seg_length = std::min(min_seg_length, segment.length);
        }
    }
    VTR_ASSERT(max_seg_lenght != std::numeric_limits<int>::min());
    VTR_ASSERT(min_seg_length != std::numeric_limits<int>::max());

    for (int x = 0; x < grid_width; x+=max_seg_lenght) {
        for (int y = 0; y < grid_height; y+=max_seg_lenght) {
            int step;
            if (x == 0 && y == 0) {
                step= 1;
            } else if (x < 2*max_seg_lenght && y < 2*max_seg_lenght) {
                    step = 2;
            } else if (x < 4*max_seg_lenght && y < 4*max_seg_lenght) {
                step = 4;
            } else {
                step = 8;
            }
            sampling_regions.emplace_back(x+max_seg_lenght, y+max_seg_lenght, x, y, step);
        }
    }

    return sampling_regions;
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

                f_compressed_wire_cost_map[from_layer_num][chan_index][segment_index][to_layer][compressed_idx] =
                    expansion_cost_entry.get_representative_cost_entry(util::e_representative_entry_method::SMALLEST);
            }
        }
    }
}

static void fill_in_missing_compressed_lookahead_entries(int segment_index, e_rr_type chan_type) {
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
                        util::Cost_Entry copied_entry = get_nearby_cost_entry_compressed_lookahead(from_layer_num,
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



/******** Interface class member function definitions ********/
CompressedMapLookahead::CompressedMapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat) {}
