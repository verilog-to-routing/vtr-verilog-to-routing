#ifndef VPR_MOVE_GENERATOR_H
#define VPR_MOVE_GENERATOR_H
#include "vpr_types.h"
#include "move_utils.h"
#include "timing_place.h"

#include <limits>

struct MoveOutcomeStats {
    float delta_cost_norm = std::numeric_limits<float>::quiet_NaN();
    float delta_bb_cost_norm = std::numeric_limits<float>::quiet_NaN();
    float delta_timing_cost_norm = std::numeric_limits<float>::quiet_NaN();

    float delta_bb_cost_abs = std::numeric_limits<float>::quiet_NaN();
    float delta_timing_cost_abs = std::numeric_limits<float>::quiet_NaN();

    e_move_result outcome = ABORTED;
    float elapsed_time = std::numeric_limits<float>::quiet_NaN();
};

/**
 * @brief a base class for move generators
 *
 * This class represents the base class for all move generators.
 * All its functions are virtual functions.
 */
class MoveGenerator {
  public:
    virtual ~MoveGenerator() {}

    /**
     * @brief Updates affected_blocks with the proposed move, while respecting the current rlim
     *
     * This function proposes a new move and updates affected_blocks accorrdingly. The function interface is general 
     * to match the parameters needed by all move generators
     *
     *  @param blocks_affected: the blocks affected by the proposed move
     *  @param rlim: the range limit for this move type
     *  @param X_coord: a scratch vector used to save X coordinates by some moves, used to save memory allocation time
     *  @param Y_coord: same as Y_coord for y coordinates
     *  @param move_type: return the move type that was used
     *  @param placer_opts: all the placer options
     *  @param criticalities: the placer criticalities, useful for timing directed moves
     */
    virtual e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim, std::vector<int>& X_coord, std::vector<int>& Y_coord, e_move_type& move_type, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) = 0;

    /**
     * @brief Recieves feedback about the outcome of the previously proposed move
     *
     * This function is very useful for RL agent to get the feedback to the agent
     *
     *  @param reward: the value of the agent's reward
     *  @param reward_fun: the name of the reward function used
     */
    virtual void process_outcome(double reward, std::string reward_fun) {}
};

#endif
