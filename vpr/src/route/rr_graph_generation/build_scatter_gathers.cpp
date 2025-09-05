
#include "build_scatter_gathers.h"

#include "switchblock_scatter_gather_common_utils.h"
#include "scatter_gather_types.h"
#include "globals.h"
#include "vtr_assert.h"

void alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                               const std::vector<bool>& inter_cluster_rr) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;


    for (const t_scatter_gather_pattern& sg_pattern : scatter_gather_patterns) {
        VTR_ASSERT(sg_pattern.type == e_scatter_gather_type::UNIDIR);
        VTR_ASSERT(sg_pattern.sg_locations.size() == 1);
        VTR_ASSERT(sg_pattern.sg_locations[0].type == e_sb_location::E_EVERYWHERE);

        for (const t_physical_tile_loc loc : grid.all_locations()) {
            if (sb_not_here(grid, inter_cluster_rr, loc, sg_pattern.sg_locations[0].type)) {
                continue;
            }
        }


    }
}
