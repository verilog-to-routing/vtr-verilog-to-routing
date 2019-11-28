#ifndef VPR_UNIFORM_MOVE_GEN_H
#define VPR_UNIFORM_MOVE_GEN_H
#include "move_generator.h"

class UniformMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim);
};

#endif
