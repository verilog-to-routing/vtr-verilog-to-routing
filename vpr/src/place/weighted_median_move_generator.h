#ifndef VPR_WEIGHTED_MEDIAN_MOVE_GEN_H
#define VPR_WEIGHTED_MEDIAN_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"

/**
 * @brief The weighted median move generator
 *
 * This move calculates the median of the bounding box edges connected to the moving block weighted by the 
 * timing criticality of the pin the caused every edge.
 *
 * For details about the move generator, Check Edge Weighted Median move generator in:
 * "Learn to Place: FPGA Placement using Reinforcement Learning and Directed Moves", ICFPT2020
 */
class WeightedMedianMoveGenerator : public MoveGenerator {
  public:
    WeightedMedianMoveGenerator() = delete;
    explicit WeightedMedianMoveGenerator(PlacerContext& placer_ctx);

  private:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* criticalities) override;

    bool get_bb_cost_for_net_excluding_block(ClusterNetId net_id,
                                             ClusterPinId moving_pin_id,
                                             const PlacerCriticalities* criticalities,
                                             t_bb_cost* coords);
};

#endif
