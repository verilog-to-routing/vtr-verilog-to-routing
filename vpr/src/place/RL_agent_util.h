#ifndef RL_AGENT_UTIL_H
#define RL_AGENT_UTIL_H

#include "move_generator.h"

//enum represents the available agent states
enum class e_agent_state {
    EARLY_IN_THE_ANNEAL,
    LATE_IN_THE_ANNEAL
};

/**
 * @brief Creates the move generators that will be used by the annealer
 *
 * This function creates 2 move generators to be used by the annealer. The type of the move generators created here depends on the 
 * type selected in placer_opts.
 * It returns a unique pointer for each move generator in move_generator and move_generator2
 *
 * @param placer_ctx Move generators store a reference to the placer context to avoid global state access.
 * @param placer_opts Contains information about the placement algorithm and its parameters.
 * @param move_lim represents the num of moves per temp.
 * @param noc_attraction_weight The attraction weight by which the NoC-biased centroid move adjust the computed location
 * towards reachable NoC routers from the moving block.
 *
 * @return Two unique pointers referring to move generators. These move generators are supposed to be used
 * in the first and second states of the agent.
 *
 */
std::pair<std::unique_ptr<MoveGenerator>, std::unique_ptr<MoveGenerator>> create_move_generators(PlacerContext& placer_ctx,
                                                                                                 const t_placer_opts& placer_opts,
                                                                                                 int move_lim,
                                                                                                 double noc_attraction_weight);

/**
 * @brief copy one of the available move_generators to be the current move_generator that would be used in the placement based on the placer_options and the agent state
 */
void assign_current_move_generator(std::unique_ptr<MoveGenerator>& move_generator,
                                   std::unique_ptr<MoveGenerator>& move_generator2,
                                   e_agent_state agent_state,
                                   const t_placer_opts& placer_opts,
                                   bool in_quench,
                                   std::unique_ptr<MoveGenerator>& current_move_generator);

/**
 * @brief move the updated current_move_generator to its original move_Generator structure based on the placer_options and the agent state
 */
void update_move_generator(std::unique_ptr<MoveGenerator>& move_generator,
                           std::unique_ptr<MoveGenerator>& move_generator2,
                           e_agent_state agent_state,
                           const t_placer_opts& placer_opts,
                           bool in_quench,
                           std::unique_ptr<MoveGenerator>& current_move_generator);
#endif
