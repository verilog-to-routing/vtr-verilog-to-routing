#pragma once
/**
 * @file
 * @author  Athavan Balakumar
 * @date    May 2026
 * @brief   Defines a windowed bipartite matching based detailed placer approach
 */

#include "detailed_placer.h"
#include "net_cost_handler.h"
#include "placer_state.h"

/**
 * @brief Detailed placer that optimizes legalized placements using local bipartite matching.
 *
 * This placer aims to partition the placement into local windows, then use bipartite
 * matching in each window to improve placement quality (Currently under development).
 */

class WindowedBiMatchingDetailedPlacer : public DetailedPlacer {
  public:
    WindowedBiMatchingDetailedPlacer(const BlkLocRegistry& curr_clustered_placement,
                                     const t_placer_opts& placer_opts);

    void optimize_placement() final;

  private:
    PlacerState placer_state_;
    NetCostHandler net_cost_handler_;
    int compressed_window_radius_ = 2; ///< Radius of the compressed-grid search window.
    int placement_layer_ = 0;          ///< Device layer processed by placer, forced to 0 temporarily
    int placement_sub_tile_ = 0;       ///< Sub-tile processed by placer, forced to 0 temporarily

    /**
     * @brief Attempts to commit an improving move/swap between two placement locations.
     */
    bool try_swap_locations(BlkLocRegistry& blk_loc_registry,
                            const DeviceGrid& grid,
                            t_pl_loc loc_a,
                            t_pl_loc loc_b);
};
