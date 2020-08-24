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
        std::cout << "Move aborted because block wasn't found\n";
        return e_create_move::ABORT; //No movable block found
    }
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    //Retrieve the compressed block grid for this block type
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[cluster_from_type->index];

    //Each x/y location possibly contains multiple sub tiles, so we need to pick
    //a z location within a compatible sub tile.
    auto to_type = g_vpr_ctx.device().grid[to.x][to.y].type;
    auto& compatible_sub_tiles = compressed_block_grid.compatible_sub_tiles_for_tile.at(to_type->index);
    if (std::find(compatible_sub_tiles.begin(), compatible_sub_tiles.end(), to.sub_tile) == compatible_sub_tiles.end()) {
        std::cout << "Move aborted due to uncompatible subtile\n";
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
