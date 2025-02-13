#ifndef VPR_WEIGHTED_MEDIAN_MOVE_GEN_H
#define VPR_WEIGHTED_MEDIAN_MOVE_GEN_H

#include "move_generator.h"

class PlaceMacros;

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
    WeightedMedianMoveGenerator(PlacerState& placer_state,
                                e_reward_function reward_function,
                                vtr::RngContainer& rng);

  private:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const PlaceMacros& place_macros,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* criticalities) override;

    /**
     * @brief Finds the bounding box and the cost of each side of the bounding box,
     * which is defined as the criticality of the connection that led to the bounding box extending
     * that far. If more than one terminal leads to a bounding box edge, w pick the cost using the criticality of the first one.
     *
     * @param net_id The net we are considering
     * @param moving_pin_id pin (which should be on this net) on a block that is being moved.
     * @param criticalities the timing criticalities of all connections
     * @param coords the bounding box and the edge costs to be filled by this method
     * @return bool Whether this net should be skipped in calculation or not
     */
    bool get_bb_cost_for_net_excluding_block(ClusterNetId net_id,
                                             ClusterPinId moving_pin_id,
                                             const PlacerCriticalities* criticalities,
                                             t_bb_cost* coords);
};

#endif
