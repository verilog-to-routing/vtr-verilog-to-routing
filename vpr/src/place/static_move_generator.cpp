
#include "static_move_generator.h"

#include "median_move_generator.h"
#include "weighted_median_move_generator.h"
#include "weighted_centroid_move_generator.h"
#include "feasible_region_move_generator.h"
#include "uniform_move_generator.h"
#include "critical_uniform_move_generator.h"
#include "centroid_move_generator.h"

#include "vtr_random.h"
#include "vtr_assert.h"

StaticMoveGenerator::StaticMoveGenerator(const vtr::vector<e_move_type, float>& move_probs) {
    all_moves.resize((int)e_move_type::NUMBER_OF_AUTO_MOVES);

    all_moves[e_move_type::UNIFORM] = std::make_unique<UniformMoveGenerator>();
    all_moves[e_move_type::MEDIAN] = std::make_unique<MedianMoveGenerator>();
    all_moves[e_move_type::CENTROID] = std::make_unique<CentroidMoveGenerator>();
    all_moves[e_move_type::W_CENTROID] = std::make_unique<WeightedCentroidMoveGenerator>();
    all_moves[e_move_type::W_MEDIAN] = std::make_unique<WeightedMedianMoveGenerator>();
    all_moves[e_move_type::CRIT_UNIFORM] = std::make_unique<CriticalUniformMoveGenerator>();
    all_moves[e_move_type::FEASIBLE_REGION] = std::make_unique<FeasibleRegionMoveGenerator>();

    initialize_move_prob(move_probs);
}

void StaticMoveGenerator::initialize_move_prob(const vtr::vector<e_move_type, float>& move_probs) {
    cumm_move_probs.resize((int)e_move_type::NUMBER_OF_AUTO_MOVES);
    std::fill(cumm_move_probs.begin(), cumm_move_probs.end(), 0.0f);

    for(auto move_type : move_probs.keys()) {
        cumm_move_probs[move_type] = move_probs[move_type];
    }

    std::partial_sum(cumm_move_probs.begin(), cumm_move_probs.end(), cumm_move_probs.begin());

    total_prob = cumm_move_probs.back();
}

e_create_move StaticMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                t_propose_action& proposed_action,
                                                float rlim,
                                                const t_placer_opts& placer_opts,
                                                const PlacerCriticalities* criticalities) {
    float rand_num = vtr::frand() * total_prob;

    for (auto move_type : cumm_move_probs.keys()) {
        if (rand_num <= cumm_move_probs[move_type]) {
            proposed_action.move_type = move_type;
            return all_moves[move_type]->propose_move(blocks_affected, proposed_action, rlim, placer_opts, criticalities);
        }
    }

    VTR_ASSERT_MSG(false, vtr::string_fmt("During static probability move selection, random number (%g) exceeded total expected probability (%g)", rand_num, total_prob).c_str());

    //Unreachable
    proposed_action.move_type = e_move_type::INVALID_MOVE;
    return e_create_move::ABORT;
}
