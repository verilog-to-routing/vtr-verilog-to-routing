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




/******** Interface class member function definitions ********/
CompressedMapLookahead::CompressedMapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat) {}
