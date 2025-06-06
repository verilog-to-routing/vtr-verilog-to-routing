#pragma once

#include "move_utils.h"

//transforms the vector moved_blocks to a vector of ints and adds it in glob_breakpoint_state
void transform_blocks_affected(const t_pl_blocks_to_be_moved& blocksAffected);

//checks the breakpoint and see whether one of them was reached and pause place,emt accordingly
void stop_placement_and_check_breakpoints(t_pl_blocks_to_be_moved& blocks_affected,
                                          e_move_result move_outcome,
                                          double delta_c,
                                          double bb_delta_c,
                                          double timing_delta_c);
