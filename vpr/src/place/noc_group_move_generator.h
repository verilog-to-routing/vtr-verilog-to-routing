#ifndef VTR_NOC_GROUP_MOVE_GENERATOR_H
#define VTR_NOC_GROUP_MOVE_GENERATOR_H

#include "move_generator.h"


class NocGroupMoveGenerator : public MoveGenerator {
  public:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* criticalities) override;
};

#endif //VTR_NOC_GROUP_MOVE_GENERATOR_H