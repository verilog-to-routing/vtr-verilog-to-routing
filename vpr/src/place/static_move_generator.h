#ifndef VPR_STATIC_MOVE_GEN_H
#define VPR_STATIC_MOVE_GEN_H
#include "move_generator.h"
#include "median_move_generator.h"
#include "weighted_median_move_generator.h"
#include "weighted_centroid_move_generator.h"
#include "feasible_region_move_generator.h"
#include "uniform_move_generator.h"
#include "critical_uniform_move_generator.h"
#include "centroid_move_generator.h"
#include <numeric>

class StaticMoveGenerator : public MoveGenerator {
private:
	std::vector<std::unique_ptr<MoveGenerator>> avail_moves;
	std::vector<float> cumm_move_probs;
    float total_prob;

public:
	StaticMoveGenerator(const std::vector<float> & prob);
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim, std::vector<int>& X_coord, std::vector<int>& Y_coord, int& type, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/);
};
#endif
