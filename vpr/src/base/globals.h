/*
 * Global variables
 * Key global variables that are used everywhere in VPR:
 */

#ifndef GLOBALS_H
#define GLOBALS_H
#include "vpr_context.h"

extern VprContext g_vpr_ctx;

typedef enum MoveOutcome {
    MOVE_REJECTED,
    MOVE_ACCEPTED,
    MOVE_ABORTED
} t_move_outcome;

struct ManualMoveInfo {
    int block_id;
    int to_x;
    int to_y;
    int subtile = 0;
    double delta_c = 0;
    double bb_delta_c = 0;
    double timing_delta_c = 0;
    t_move_outcome user_move_outcome;
    t_move_outcome placer_move_outcome;
    bool valid_input;
};

#endif
