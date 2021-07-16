#ifndef VPR_MANUAL_MOVE_GEN_H
#define VPR_MANUAL_MOVE_GEN_H

#include "move_generator.h"
#include "median_move_generator.h"
#include "weighted_median_move_generator.h"
#include "weighted_centroid_move_generator.h"
#include "feasible_region_move_generator.h"
#include "uniform_move_generator.h"
#include "critical_uniform_move_generator.h"
#include "centroid_move_generator.h"
#include "simpleRL_move_generator.h"
#include <numeric>

/** Manual Moves Generator, inherits from MoveGenerator class **/
class ManualMoveGenerator : public MoveGenerator {
  private:
    std::vector<std::unique_ptr<MoveGenerator>> avail_moves; // list of pointers to the available move generators (the different move types)
    std::unique_ptr<KArmedBanditAgent> karmed_bandit_agent;  // a pointer to the specific agent used (e.g. Softmax)

  public:
    // constructors using a pointer to the agent used
    ManualMoveGenerator(std::unique_ptr<EpsilonGreedyAgent>& agent);
    ManualMoveGenerator(std::unique_ptr<SoftmaxAgent>& agent);

    //Evaluates if move is successful and legal or unable to do.
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float /*rlim*/, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/);
};

#endif /*VPR_MANUAL_MOVE_GEN_H */
