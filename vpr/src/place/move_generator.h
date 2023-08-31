#ifndef VPR_MOVE_GENERATOR_H
#define VPR_MOVE_GENERATOR_H
#include "vpr_types.h"
#include "move_utils.h"
#include "timing_place.h"
#include "directed_moves_util.h"
#include "placer_globals.h"

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
 * @brief A Struct to hold statistics about the different move types
 *
 * blk_type_moves: the block type index of each proposed move (e.g. [0..NUM_PL_MOVE_TYPES * (agent_available_types.size()-1)])
 * accepted_moves: the number of accepted moves of each move and block type (e.g. [0..NUM_PL_MOVE_TYPES * (agent_available_types.size()-1)] )
 * rejected_moves: the number of rejected moves of each move and block type (e.g. [0..NUM_PL_MOVE_TYPES * (agent_available_types.size()-1)] )
 *
 */
struct MoveTypeStat {
    std::vector<int> blk_type_moves;
    std::vector<int> accepted_moves;
    std::vector<int> rejected_moves;
};

/**
 * @brief a base class for move generators
 *
 * This class represents the base class for all move generators.
 * All its functions are virtual functions.
 */
class MoveGenerator {
  public:
    virtual ~MoveGenerator() = default;

    /**
     * @brief Updates affected_blocks with the proposed move, while respecting the current rlim
     *
     * This function proposes a new move and updates blocks affected and move_type accorrdingly. The function interface is general 
     * to match the parameters needed by all move generators
     *
     *  @param blocks_affectedt: the output of the move
     *  @param proposed_action: Contains the move type and block type. If the block type is specified,
     *  the proposed move swaps instances of the given block type. Otherwise, the selected block type
     *  by the move generator is written to proposed_action.logical_blk_type_index.
     *  If proposed_action.logical_blk_type_index is -1, this function will choose the block from the netlist (regardless of type).
     *  @param rlim: maximum distance a block can move in x or y direction, in the compressed grid space
     *  @param placer_opts: all the placer options
     *  @param criticalities: the placer criticalities, useful for timing directed moves
     */
    virtual e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& proposed_action, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) = 0;

    /**
     * @brief Recieves feedback about the outcome of the previously proposed move
     *
     * This function is very useful for RL agent to get the feedback to the agent
     *
     *  @param reward: the value of the agent's reward
     *  @param reward_fun: the name of the reward function used
     */
    virtual void process_outcome(double /*reward*/, e_reward_function /*reward_fun*/) {}
};

#endif
