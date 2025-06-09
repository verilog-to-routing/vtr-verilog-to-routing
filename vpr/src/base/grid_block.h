#pragma once

#include "clustered_netlist_fwd.h"
#include "physical_types.h"
#include "vpr_types.h"

#include <vector>

///@brief Stores the clustered blocks placed at a particular grid location
struct t_grid_blocks {
    /**
     * @brief The clustered blocks associated with this grid location.
     *
     * Index range: [0..device_ctx.grid[x_loc][y_loc].type->capacity]
     */
    std::vector<ClusterBlockId> blocks;

    /**
     * @brief Test if a subtile at a grid location is occupied by a block.
     *
     * Returns true if the subtile corresponds to the passed-in id is not
     * occupied by a block at this grid location. The subtile id serves
     * as the z-dimensional offset in the grid indexing.
     */
    inline bool subtile_empty(size_t isubtile) const {
        return blocks[isubtile] == ClusterBlockId::INVALID();
    }
};

class GridBlock {
  public:
    GridBlock() = default;

    GridBlock(size_t width, size_t height, size_t layers) {
        grid_blocks_.resize({layers, width, height});
    }

    /**
     * @brief Initialize `grid_blocks`, the inverse structure of `block_locs`.
     *
     * The container at each grid block location should have a length equal to the
     * subtile capacity of that block. Unused subtiles would be marked ClusterBlockId::INVALID().
     */
    void init_grid_blocks(const DeviceGrid& device_grid);

    inline void initialized_grid_block_at_location(const t_physical_tile_loc& loc, int num_sub_tiles) {
        grid_blocks_[loc.layer_num][loc.x][loc.y].blocks.resize(num_sub_tiles, ClusterBlockId::INVALID());
    }

    inline void set_block_at_location(const t_pl_loc& loc, ClusterBlockId blk_id) {
        grid_blocks_[loc.layer][loc.x][loc.y].blocks[loc.sub_tile] = blk_id;
    }

    inline ClusterBlockId block_at_location(const t_pl_loc& loc) const {
        return grid_blocks_[loc.layer][loc.x][loc.y].blocks[loc.sub_tile];
    }

    inline size_t num_blocks_at_location(const t_physical_tile_loc& loc) const {
        return grid_blocks_[loc.layer_num][loc.x][loc.y].blocks.size();
    }

    /**
     * @brief Returns the number of subtiles in use at the specified grid location.
     *
     * Iterates over all subtiles at the given physical tile location and counts
     * how many are currently occupied by a block.
     */
    inline int get_usage(const t_physical_tile_loc loc) const {
        int usage = 0;
        for (size_t sub_tile = 0; sub_tile < num_blocks_at_location(loc); ++sub_tile) {
            if (!is_sub_tile_empty(loc, static_cast<int>(sub_tile)))
                usage++;
        }
        return usage;
    }

    inline bool is_sub_tile_empty(const t_physical_tile_loc loc, int sub_tile) const {
        return grid_blocks_[loc.layer_num][loc.x][loc.y].subtile_empty(sub_tile);
    }

    inline void clear() {
        grid_blocks_.clear();
    }

    /**
     * @brief Initialize usage to 0 and blockID to INVALID for all grid block locations
     */
    void zero_initialize();

    /**
     * @brief Initializes the GridBlock object with the given block_locs.
     * @param block_locs Stores the location of each clustered block.
     */
    void load_from_block_locs(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

  private:
    vtr::NdMatrix<t_grid_blocks, 3> grid_blocks_;
};
