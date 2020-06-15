#ifndef VPR_CRITICAL_UNIFORM_MOVE_GEN_H
#define VPR_CRITICAL_UNIFORM_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"


class CriticalUniformMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim,
    	std::vector<int>& , std::vector<int>&, int & , int, const PlacerCriticalities* /*criticalities*/);
};

#endif
