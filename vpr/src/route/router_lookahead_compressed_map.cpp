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


static std::unordered_map<int, std::unordered_set<int>> get_sampling_points(const std::vector<t_segment_inf>& segment_inf);



static std::unordered_map<int, std::unordered_set<int>> get_sampling_points(const std::vector<t_segment_inf>& segment_inf) {

    const auto& grid = g_vpr_ctx.device().grid;
    std::unordered_map<int, std::unordered_set<int>> sampling_points;


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

    for (int x = 0; x < grid_width; ++x) {
        for (int y = 0; y < grid_height; ++y) {
            if (x > max_seg_lenght || y > max_seg_lenght) {
                if (x <= 2*max_seg_lenght || y <= 2*max_seg_lenght) {
                    if (x%2 != 0 && y%2 != 0) {
                        continue;
                    }
                } else if (x <= 4*max_seg_lenght || y <= 4*max_seg_lenght) {
                    if (x%4 != 0 && y%4 != 0) {
                        continue;
                    }
                } else {
                    if (x%8 != 0 && y%8 != 0) {
                        continue;
                    }
                }

                if (sampling_points.count(x) == 0) {
                    sampling_points[x] = std::unordered_set<int>();
                }
                sampling_points[x].insert(y);
            }
        }
    }

    return sampling_points;
}













    /******** Interface class member function definitions ********/
CompressedMapLookahead::CompressedMapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat) {}
