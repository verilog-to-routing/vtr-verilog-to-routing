#ifndef VPR_MANUAL_MOVE_GEN_H
#define VPR_MANUAL_MOVE_GEN_H
#include "move_generator.h"
#include "globals.h"

void get_manual_move_info_m(manual_move_info mmi);

class ManualMoveGenerator : public MoveGenerator {
    public:
        e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim);
};

#endif
