#ifndef VPR_MANUAL_MOVE_GEN_H
#define VPR_MANUAL_MOVE_GEN_H

#ifndef NO_GRAPHICS

#    include "move_generator.h"
#    include "median_move_generator.h"
#    include "weighted_median_move_generator.h"
#    include "weighted_centroid_move_generator.h"
#    include "feasible_region_move_generator.h"
#    include "uniform_move_generator.h"
#    include "critical_uniform_move_generator.h"
#    include "centroid_move_generator.h"
#    include <numeric>

/** Manual Moves Generator, inherits from MoveGenerator class **/
class ManualMoveGenerator : public MoveGenerator {
  public:
    //Evaluates if move is successful and legal or unable to do.
    using MoveGenerator::propose_move;
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected);
};

#endif /*NO_GRAPHICS*/

#endif /*VPR_MANUAL_MOVE_GEN_H */
