
#include "build_scatter_gathers.h"

#include "switchblock_scatter_gather_common_utils.h"
#include "scatter_gather_types.h"
#include "globals.h"
#include "vtr_assert.h"

#include <vector>

static void index_to_correct_channels(const t_wireconn_inf& pattern,
                                      const t_physical_tile_loc& loc,
                                      const t_chan_details& chan_details_x,
                                      const t_chan_details& chan_details_y,
                                      std::vector<std::pair<t_physical_tile_loc, e_rr_type>>& correct_channels) {
    correct_channels.clear();

    for (e_side side : pattern.sides) {
        t_physical_tile_loc chan_loc;
        e_rr_type chan_type;

        index_into_correct_chan(loc, side, chan_details_x, chan_details_y,chan_loc, chan_type);

        if (!chan_coords_out_of_bounds(chan_loc, chan_type)) {
            correct_channels.push_back({chan_loc, chan_type});
        }
    }
}

void alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                               const std::vector<bool>& inter_cluster_rr,
                                               const t_chan_details& chan_details_x,
                                               const t_chan_details& chan_details_y) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    std::vector<std::pair<t_physical_tile_loc, e_rr_type>> gather_channels;
    std::vector<std::pair<t_physical_tile_loc, e_rr_type>> scatter_channels;

    for (const t_scatter_gather_pattern& sg_pattern : scatter_gather_patterns) {
        VTR_ASSERT(sg_pattern.type == e_scatter_gather_type::UNIDIR);

        for (const t_sg_location& sg_loc_info : sg_pattern.sg_locations) {

            for (const t_physical_tile_loc gather_loc : grid.all_locations()) {
                if (sb_not_here(grid, inter_cluster_rr, gather_loc, sg_loc_info.type)) {
                    continue;
                }

                auto it = std::find_if(sg_pattern.sg_links.begin(), sg_pattern.sg_links.end(),
                       [&](const t_sg_link& link) {
                           return link.name == sg_loc_info.sg_link_name;
                       });

                VTR_ASSERT_SAFE(it != sg_pattern.sg_links.end());
                const t_sg_link& sg_link = *it;

                t_physical_tile_loc scatter_loc;
                scatter_loc.x = gather_loc.x + sg_link.x_offset;
                scatter_loc.y = gather_loc.y + sg_link.y_offset;
                scatter_loc.layer_num = gather_loc.layer_num + sg_link.z_offset;


                index_to_correct_channels(sg_pattern.gather_pattern, gather_loc, chan_details_x, chan_details_y, gather_channels);
                index_to_correct_channels(sg_pattern.scatter_pattern, scatter_loc, chan_details_x, chan_details_y, scatter_channels);

                if (gather_channels.empty() || scatter_channels.empty()) {
                    continue;
                }

            }
        }


    }
}
