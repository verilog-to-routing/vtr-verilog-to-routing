#ifndef VPR_WEIGHTED_CENTROID_MOVE_GEN_H
#define VPR_WEIGHTED_CENTROID_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"


class WeightedCentroidMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim,
    std::vector<int>& X_coord, std::vector<int>& Y_coord, int &,int place_high_fanout_net, const PlacerCriticalities* criticalities);
};

#endif
