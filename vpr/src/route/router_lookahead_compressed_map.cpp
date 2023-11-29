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


struct SamplingRegion {
    SamplingRegion() = default;
    SamplingRegion(int x_max_, int y_max_, int x_min_, int y_min_, int x_step_, int y_step_)
        : x_max(x_max_)
        , y_max(y_max_)
        , x_min(x_min_)
        , y_min(y_min_)
        , x_step(x_step_)
        , y_step(y_step_) {}
    int x_max = OPEN;
    int y_max = OPEN;
    int x_min = OPEN;
    int y_min = OPEN;

    int x_step = OPEN;
    int y_step = OPEN;
};

static std::vector<SamplingRegion> get_sampling_points(const std::vector<t_segment_inf>& segment_inf);

static void compute_router_wire_lookahead(const std::vector<t_segment_inf>& segment_inf)  {
    vtr::ScopedStartFinishTimer timer("Computing wire lookahead");

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    auto sampling_points = get_sampling_points(segment_inf);

    compressed_wire_cost_map = t_wire_cost_map({static_cast<unsigned long>(grid.get_num_layers()),
                                                2,
                                                segment_inf.size(),
                                                static_cast<unsigned long>(grid.get_num_layers()),
                                                sampling_points.size(),
                                                sampling_points[0].size()});

    for (int compressed_x = 0; compressed_x < static_cast<int>(sampling_points.size())-1; ++compressed_x) {
        int curr_comp_x = sampling_points[compressed_x].first;
        for (int compressed_y = 0; compressed_y < static_cast<int>(sampling_points[compressed_x].size())-1; ++compressed_y) {

            int curr_comp_y = sampling_points[compressed_x].at(compressed_y);
            int nxt_comp_x = sampling_points[compressed_x+1];




        }
    }



}

static std::vector<SamplingRegion> get_sampling_points(const std::vector<t_segment_inf>& segment_inf) {

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
            SamplingRegion sampling_region(x + max_seg_lenght, y + max_seg_lenght, x, y, -1, -1);
            if (x == 0 && y == 0) {
                sampling_region.x_step = 1;
                sampling_region.y_step = 1;
            } else if (x < 2*max_seg_lenght && y < 2*max_seg_lenght) {
                    sampling_region.x_step = 2;
                    sampling_region.y_step = 2;

            } else if (x < 4*max_seg_lenght || y < 4*max_seg_lenght) {
                sampling_region.x_step = 4;
                sampling_region.y_step = 4;
            } else {
                sampling_region.x_step = 4;
                sampling_region.y_step = 4;
            }
        }
    }

    return sampling_regions;
}













    /******** Interface class member function definitions ********/
CompressedMapLookahead::CompressedMapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat) {}
