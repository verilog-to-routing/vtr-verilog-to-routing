#ifndef VPR_MEDIAN_MOVE_GEN_H
#define VPR_MEDIAN_MOVE_GEN_H
#include "move_generator.h"

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
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* /*criticalities*/);
};

#endif
