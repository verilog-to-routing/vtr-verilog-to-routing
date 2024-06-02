#ifndef VPR_STATIC_MOVE_GEN_H
#define VPR_STATIC_MOVE_GEN_H

#include "move_generator.h"

/**
 * @brief a special move generator class that controls the different move types by assigning a fixed probability for each move type
 *
 * It is useful to give VPR user the ability to assign static probabilities for different move types.
 */
class StaticMoveGenerator : public MoveGenerator {
  private:
    vtr::vector<e_move_type, std::unique_ptr<MoveGenerator>> all_moves;     // list of pointers to the different available move type generators
    vtr::vector<e_move_type, float> cumm_move_probs;                        // accumulative probabilities for different move types
    float total_prob;                                                       // sum of the input probabilities from the use

    void initialize_move_prob(const vtr::vector<e_move_type, float>& move_probs);

  public:
    StaticMoveGenerator(const vtr::vector<e_move_type, float>& move_probs);

    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* criticalities) override;
};
#endif
