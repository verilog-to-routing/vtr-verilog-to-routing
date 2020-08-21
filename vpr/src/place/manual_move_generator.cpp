#include "manual_move_generator.h"
#include <iostream>

t_pl_loc to;
int block_id;

void mmg_get_manual_move_info(ManualMoveInfo mmi) {
    block_id = mmi.block_id;
    to = t_pl_loc(mmi.to_x, mmi.to_y, mmi.subtile);
}

e_create_move ManualMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim) {
    /* Pick the user indicated block to be swapped with user indicated block.   */
    ClusterBlockId b_from = ClusterBlockId(block_id);
    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    if (!find_to_loc_uniform(cluster_from_type, rlim, from, to)) {
        return e_create_move::ABORT;
    }

#if 0
    auto& grid = g_vpr_ctx.device().grid;
	VTR_LOG( "swap [%d][%d][%d] %s block %zu \"%s\" <=> [%d][%d][%d] %s block ",
		from.x, from.y, from.sub_tile, grid[from.x][from.y].type->name, size_t(b_from), (b_from ? cluster_ctx.clb_nlist.block_name(b_from).c_str() : ""),
		to.x, to.y, to.sub_tile, grid[to.x][to.y].type->name);
    if (b_to) {
        VTR_LOG("%zu \"%s\"", size_t(b_to), cluster_ctx.clb_nlist.block_name(b_to).c_str());
    } else {
        VTR_LOG("(EMPTY)");
    }
    VTR_LOG("\n");
#endif

    return ::create_move(blocks_affected, b_from, to);
}
