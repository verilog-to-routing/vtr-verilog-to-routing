#include "static_move_generator.h"
#include "globals.h"
#include <algorithm>

#include "vtr_random.h"
#include "vtr_assert.h"

StaticMoveGenerator::StaticMoveGenerator(const std::vector<float>& prob) {
    avail_moves.emplace_back(std::make_unique<UniformMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<MedianMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<CentroidMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<WeightedCentroidMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<WeightedMedianMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<CriticalUniformMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<FeasibleRegionMoveGenerator>());

    initialize_move_prob(prob);
}

void StaticMoveGenerator::initialize_move_prob(const std::vector<float>& prob) {
    cumm_move_probs.push_back(prob[0]);
    for (size_t i = 1; i < prob.size(); i++) {
        cumm_move_probs.push_back(prob[i] + cumm_move_probs[i - 1]);
    }

    total_prob = cumm_move_probs[cumm_move_probs.size() - 1];
}

e_create_move StaticMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& proposed_action, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    float rand_num = vtr::frand() * total_prob;
    for (size_t i = 0; i < cumm_move_probs.size(); i++) {
        if (rand_num <= cumm_move_probs[i]) {
            proposed_action.move_type = (e_move_type)i;
            return avail_moves[i]->propose_move(blocks_affected, proposed_action, rlim, placer_opts, criticalities);
        }
    }
    VTR_ASSERT_MSG(false, vtr::string_fmt("During static probability move selection, random number (%g) exceeded total expected probabaility (%g)", rand_num, total_prob).c_str());

    //Unreachable
    proposed_action.move_type = (e_move_type)(avail_moves.size() - 1);
    return avail_moves[avail_moves.size() - 1]->propose_move(blocks_affected, proposed_action, rlim, placer_opts, criticalities);
}
