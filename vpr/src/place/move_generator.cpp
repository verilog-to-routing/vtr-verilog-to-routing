
#include "move_generator.h"

#include "vpr_error.h"

void MoveGenerator::calculate_reward_and_process_outcome(const MoveOutcomeStats& move_outcome_stats,
                                                         double delta_c,
                                                         float timing_bb_factor) {
    if (reward_func_ == e_reward_function::BASIC) {
        process_outcome(-1 * delta_c, reward_func_);
    } else if (reward_func_ == e_reward_function::NON_PENALIZING_BASIC || reward_func_ == e_reward_function::RUNTIME_AWARE) {
        if (delta_c < 0) {
            process_outcome(-1 * delta_c, reward_func_);
        } else {
            process_outcome(0, reward_func_);
        }
    } else if (reward_func_ == e_reward_function::WL_BIASED_RUNTIME_AWARE) {
        if (delta_c < 0) {
            float reward = -1
                           * (move_outcome_stats.delta_cost_norm
                              + (0.5 - timing_bb_factor)
                                    * move_outcome_stats.delta_timing_cost_norm
                              + timing_bb_factor
                                    * move_outcome_stats.delta_bb_cost_norm);
            process_outcome(reward, reward_func_);
        } else {
            process_outcome(0, reward_func_);
        }
    } else {
        VTR_ASSERT_SAFE(reward_func_ == e_reward_function::UNDEFINED_REWARD);
        VPR_ERROR(VPR_ERROR_PLACE, "Undefined reward function!\n");
    }
}
