#pragma once
/**
 * @file
 * @author  Athavan Balakumar
 * @date    May 2026
 * @brief   Defines a windowed bipartite matching based detailed placer approach
 */

#include "detailed_placer.h"

/**
 * @brief Detailed placer that optimizes legalized placements using local bipartite matching.
 *
 * This placer aims to partition the placement into local windows, then use bipartite
 * matching in each window to improve placement quality (Currently under development).
 */

class WindowedBiMatchingDetailedPlacer : public DetailedPlacer {
  public:
    using DetailedPlacer::DetailedPlacer;

    void optimize_placement() final;

  private:
    int window_size_ = 2;        ///< @brief Size of local placement window, forced to 2 temporarily
    int placement_layer_ = 0;    ///< @brief Device layer processed by placer, forced to 0 temporarily
    int placement_sub_tile_ = 0; ///< @brief Sub-tile processed by placer.

    /**
     * @brief Creates a placement location from grid coordinates.
     */
    t_pl_loc make_pl_loc(int x, int y, int sub_tile, int layer);

    /**
     * @brief Returns true if  physical tile at given grid location is not empty.
     */
    bool is_non_empty_physical_tile(const DeviceGrid& grid,
                                    int x,
                                    int y,
                                    int layer);

    /**
     * @brief Returns true if every physical tile in the 2x2 window is not empty.
     */
    bool window_has_no_empty_physical_tiles(const DeviceGrid& grid,
                                            int x,
                                            int y,
                                            int layer);
    /**
     * @brief Returns true if every checked sub-tile location in the window contains a placed block.
     */
    bool window_has_all_placed_blocks(const BlkLocRegistry& blk_loc_registry,
                                      int x,
                                      int y,
                                      int layer);
    /**
     * @brief Returns true if two blocks can be swapped by this pass.
     */
    bool blocks_are_swappable(const BlkLocRegistry& blk_loc_registry,
                              const ClusteredNetlist& clb_nlist,
                              ClusterBlockId block_a,
                              ClusterBlockId block_b);
    /**
     * @brief Attempts to commit a swap between two placed blocks.
     */
    bool try_swap_blocks(BlkLocRegistry& blk_loc_registry,
                         ClusterBlockId block_a,
                         t_pl_loc loc_a,
                         ClusterBlockId block_b,
                         t_pl_loc loc_b);
};
