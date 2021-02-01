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

/**
 * @brief a special move generator class that cointrols the different move types by assigning a fixed probability for each move type 
 *
 * It is useful to give VPR user the ability to assign static probabilities for different move types.
 */
class StaticMoveGenerator : public MoveGenerator {
  private:
    std::vector<std::unique_ptr<MoveGenerator>> avail_moves; //list of pointers to the different available move type generators
    std::vector<float> cumm_move_probs;                      // accumulative probabilities for different move types
    float total_prob;                                        // sum of the input probabilities from the use
    void initialize_move_prob(const std::vector<float>& prob);

  public:
    StaticMoveGenerator(const std::vector<float>& prob);
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& move_type, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities);
};
#endif
