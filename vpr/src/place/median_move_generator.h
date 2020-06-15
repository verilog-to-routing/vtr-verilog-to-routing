#ifndef VPR_MEDIAN_MOVE_GEN_H
#define VPR_MEDIAN_MOVE_GEN_H
#include "move_generator.h"


class MedianMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float,
     std::vector<int>& X_coord, std::vector<int>& Y_coord, int &, int high_fanout_net, const PlacerCriticalities* /*criticalities*/);
};

#endif
