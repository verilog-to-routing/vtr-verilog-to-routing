#ifndef VPR_FEASIBLE_REGION_MOVE_GEN_H
#define VPR_FEASIBLE_REGION_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"

const float CRIT_LIMIT = 0.7;

class FeasibleRegionMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float,
    std::vector<int>& , std::vector<int>&, int &, int, const PlacerCriticalities* criticalities );
};

#endif
