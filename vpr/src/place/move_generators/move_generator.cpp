
#include "move_generator.h"

#include "vpr_error.h"

void MoveGenerator::calculate_reward_and_process_outcome(const MoveOutcomeStats& move_outcome_stats,
                                                         double delta_c,
                                                         float timing_bb_factor) {
    /* To learn about different reward functions refer to the following paper:
     * Elgammal MA, Murray KE, Betz V. RLPlace: Using reinforcement learning and
     * smart perturbations to optimize FPGA placement.
     * IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems.
     * 2021 Sep 3;41(8):2532-45.
     *
     * For runtime-aware reward function, the reward value is divided by a normalized
     * runtime in the implementation of process_outcome()
     */
    switch (reward_func_) {
        case e_reward_function::WL_BIASED_RUNTIME_AWARE:
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
            break;

        case e_reward_function::BASIC:
            process_outcome(-1 * delta_c, reward_func_);
            break;


        case e_reward_function::NON_PENALIZING_BASIC:
        case e_reward_function::RUNTIME_AWARE:
            if (delta_c < 0) {
                process_outcome(-1 * delta_c, reward_func_);
            } else {
                process_outcome(0, reward_func_);
            }
            break;

        default:
            VTR_ASSERT_SAFE(reward_func_ == e_reward_function::UNDEFINED_REWARD);
            VPR_ERROR(VPR_ERROR_PLACE, "Undefined reward function!\n");
            break;
    }
}

void MoveTypeStat::print_placement_move_types_stats() const {
    VTR_LOG("\n\nPlacement perturbation distribution by block and move type: \n");

    VTR_LOG(
        "------------------ ----------------- ---------------- ---------------- --------------- ------------ \n");
    VTR_LOG(
        "    Block Type         Move Type       (%%) of Total      Accepted(%%)     Rejected(%%)    Aborted(%%)\n");
    VTR_LOG(
        "------------------ ----------------- ---------------- ---------------- --------------- ------------ \n");

    int total_moves = 0;
    for (size_t i = 0; i < blk_type_moves.size(); ++i) {
        total_moves += blk_type_moves.get(i);
    }

    auto& device_ctx = g_vpr_ctx.device();
    int count = 0;
    int num_of_avail_moves = blk_type_moves.size() / device_ctx.logical_block_types.size();

    //Print placement information for each block type
    for (const auto& itype : device_ctx.logical_block_types) {
        //Skip non-existing block types in the netlist
        if (itype.index == 0 || movable_blocks_per_type(itype).empty()) {
            continue;
        }

        count = 0;
        for (int imove = 0; imove < num_of_avail_moves; imove++) {
            const auto& move_name = move_type_to_string(e_move_type(imove));
            int moves = blk_type_moves[itype.index][imove];
            if (moves != 0) {
                int accepted = accepted_moves[itype.index][imove];
                int rejected = rejected_moves[itype.index][imove];
                int aborted = moves - (accepted + rejected);
                if (count == 0) {
                    VTR_LOG("%-18.20s", itype.name.c_str());
                } else {
                    VTR_LOG("                  ");
                }
                VTR_LOG(
                    " %-22.20s %-16.2f %-15.2f %-14.2f %-13.2f\n",
                    move_name.c_str(), 100.0f * (float)moves / (float)total_moves,
                    100.0f * (float)accepted / (float)moves, 100.0f * (float)rejected / (float)moves,
                    100.0f * (float)aborted / (float)moves);
            }
            count++;
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

static std::map<std::string, e_reward_function> available_reward_function = {
    {"basic", e_reward_function::BASIC},
    {"nonPenalizing_basic", e_reward_function::NON_PENALIZING_BASIC},
    {"runtime_aware", e_reward_function::RUNTIME_AWARE},
    {"WLbiased_runtime_aware", e_reward_function::WL_BIASED_RUNTIME_AWARE}};

e_reward_function string_to_reward(const std::string& st) {
    return available_reward_function[st];
}
