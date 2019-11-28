#ifndef VPR_MOVE_GENERATOR_H
#define VPR_MOVE_GENERATOR_H
#include "vpr_types.h"
#include "move_utils.h"

#include <limits>

struct MoveOutcomeStats {
    float delta_cost_norm = std::numeric_limits<float>::quiet_NaN();
    float delta_bb_cost_norm = std::numeric_limits<float>::quiet_NaN();
    float delta_timing_cost_norm = std::numeric_limits<float>::quiet_NaN();

    float delta_bb_cost_abs = std::numeric_limits<float>::quiet_NaN();
    float delta_timing_cost_abs = std::numeric_limits<float>::quiet_NaN();

    e_move_result outcome = ABORTED;
    float elapsed_time = std::numeric_limits<float>::quiet_NaN();
};

class MoveGenerator {
  public:
    virtual ~MoveGenerator() {}

    //Updates affected_blocks with the proposed move, while respecting the current rlim
    virtual e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim) = 0;

    //Recieves feedback about the outcome of the previously proposed move
    virtual void process_outcome(const MoveOutcomeStats& /*move_outcome*/) {}
};

#endif
