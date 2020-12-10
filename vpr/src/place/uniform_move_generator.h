#ifndef VPR_UNIFORM_MOVE_GEN_H
#define VPR_UNIFORM_MOVE_GEN_H
#include "move_generator.h"

class UniformMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/);
};

#endif
