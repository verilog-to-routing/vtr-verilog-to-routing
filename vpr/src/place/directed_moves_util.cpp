
#include "directed_moves_util.h"

static std::map<std::string, e_reward_function> available_reward_function = {
    {"basic", e_reward_function::BASIC},
    {"nonPenalizing_basic", e_reward_function::NON_PENALIZING_BASIC},
    {"runtime_aware", e_reward_function::RUNTIME_AWARE},
    {"WLbiased_runtime_aware", e_reward_function::WL_BIASED_RUNTIME_AWARE}};

e_reward_function string_to_reward(const std::string& st) {
    return available_reward_function[st];
}
