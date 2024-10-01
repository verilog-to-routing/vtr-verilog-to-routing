#ifndef VPR_DIRECTED_MOVES_UTIL_H
#define VPR_DIRECTED_MOVES_UTIL_H

#include "globals.h"
#include "timing_place.h"

/**
 * @brief enum represents the different reward functions
 */
enum class e_reward_function {
    BASIC,                      ///@ directly uses the change of the annealing cost function
    NON_PENALIZING_BASIC,       ///@ same as basic reward function but with 0 reward if it's a hill-climbing one
    RUNTIME_AWARE,              ///@ same as NON_PENALIZING_BASIC but with normalizing with the runtime factor of each move type
    WL_BIASED_RUNTIME_AWARE,    ///@ same as RUNTIME_AWARE but more biased to WL cost (the factor of the bias is REWARD_BB_TIMING_RELATIVE_WEIGHT)
    UNDEFINED_REWARD            ///@ Used for manual moves
};

e_reward_function string_to_reward(const std::string& st);

#endif
