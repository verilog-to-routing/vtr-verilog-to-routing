#ifndef VPR_MOVE_GENERATOR_H
#define VPR_MOVE_GENERATOR_H
#include "vpr_types.h"
#include "move_utils.h"

struct MoveOutcomeStats {

};

class MoveGenerator {
  public:
    virtual ~MoveGenerator() {}

    //Updates affected_blocks with the proposed move, while respecting the current rlim
    virtual e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim) = 0;

    virtual void process_outcome(const MoveOutcomeStats& /*move_outcome*/) {};
};

#endif
