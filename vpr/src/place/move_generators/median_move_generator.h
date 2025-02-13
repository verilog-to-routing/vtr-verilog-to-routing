#ifndef VPR_MEDIAN_MOVE_GEN_H
#define VPR_MEDIAN_MOVE_GEN_H

#include "move_generator.h"

class PlaceMacros;

/**
 * @brief Median move generator
 *
 * It mainly targets wirelength minimization by a random block (bi)
 * into the median region of this block which is the range of locations (x, y) 
 * of positions that minimize the HPWL. 
 *
 * To calculate the median region, we iterate over all the moving block pins calculating the bounding box of each of this nets.
 * Then, we push the corrdinates of these bb into two vectors and calculate its median.
 *
 * To get the exact location, we calculate the center of median region and find a random location in a range
 * around it
 */
class MedianMoveGenerator : public MoveGenerator {
  public:
    MedianMoveGenerator() = delete;
    MedianMoveGenerator(PlacerState& placer_state,
                        e_reward_function reward_function,
                        vtr::RngContainer& rng);

  private:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const PlaceMacros& place_macros,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* /*criticalities*/) override;

    /**
     * @brief Calculates the bounding box of a net by storing its coordinates
     * in the bb_coord_new data structure.
     *
     * @details It uses information from PlaceMoveContext to calculate the bb incrementally.
     * This routine should only be called for large nets, since it has some overhead
     * relative to just doing a brute force bounding box calculation. The bounding box coordinate
     * and edge information for inet must be valid before this routine is called.
     * Currently assumes channels on both sides of the CLBs forming the edges of the bounding box
     * can be used. Essentially, I am assuming the pins always lie on the outside of the bounding box.
     * The x and y coordinates are the pin's x and y coordinates. IO blocks are considered to be
     * one cell in for simplicity. */
    bool get_bb_incrementally(ClusterNetId net_id, t_bb& bb_coord_new,
                              t_physical_tile_loc old_pin_loc,
                              t_physical_tile_loc new_pin_loc);


    /**
     * @brief Finds the bounding box of a net and stores its coordinates in the bb_coord_new data structure.
     *
     * @details It excludes the moving block sent in function arguments in moving_block_id.
     * It also returns whether this net should be excluded from median calculation or not.
     * This routine should only be called for small nets, since it does not determine
     * enough information for the bounding box to be updated incrementally later.
     * Currently assumes channels on both sides of the CLBs forming the edges of the bounding box can be used.
     * Essentially, I am assuming the pins always lie on the outside of the bounding box.
     */
    void get_bb_from_scratch_excluding_block(ClusterNetId net_id,
                                             t_bb& bb_coord_new,
                                             ClusterBlockId moving_block_id,
                                             bool& skip_net);
};

#endif
