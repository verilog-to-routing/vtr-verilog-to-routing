/*
 * @file 	manual_move_generator.h
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the ManualMoveGenerator class.
 */

#ifndef VPR_MANUAL_MOVE_GEN_H
#define VPR_MANUAL_MOVE_GEN_H

#include "move_generator.h"
#include "median_move_generator.h"
#include "weighted_median_move_generator.h"
#include "weighted_centroid_move_generator.h"
#include "feasible_region_move_generator.h"
#include "uniform_move_generator.h"
#include "critical_uniform_move_generator.h"
#include "centroid_move_generator.h"
#include "simpleRL_move_generator.h"
#include <numeric>

/**
 * @brief Manual Moves Generator, inherits from MoveGenerator class.
 *
 * Manual Move Generator, needed for swapping blocks requested by the user.
 */
class ManualMoveGenerator : public MoveGenerator {
  public:
    //Evaluates if move is successful and legal or unable to do.
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float /*rlim*/, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/);
};

#endif /*VPR_MANUAL_MOVE_GEN_H */
