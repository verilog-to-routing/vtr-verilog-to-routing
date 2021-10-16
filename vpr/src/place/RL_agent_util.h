#ifndef RL_AGENT_UTIL_H
#define RL_AGENT_UTIL_H

#include "static_move_generator.h"
#include "simpleRL_move_generator.h"
#include "manual_move_generator.h"

//enum represents the available agent states
enum e_agent_state {
    EARLY_IN_THE_ANNEAL,
    LATE_IN_THE_ANNEAL
};

/**
 * @brief Creates the move generators that will be used by the annealer
 *
 * This function creates 2 move generators to be used by the annealer. The type of the move generators created here depends on the 
 * type selected in placer_opts.
 * It returns a unique pointer for each move generator in move_generator and move_generator2
 * move_lim: represents the num of moves per temp.
 */
void create_move_generators(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, const t_placer_opts& placer_opts, int move_lim);

/**
 * @brief copy one of the available move_generators to be the current move_generator that would be used in the placement based on the placer_options and the agent state
 */
void assign_current_move_generator(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, e_agent_state agent_state, const t_placer_opts& placer_opts, bool in_quench, std::unique_ptr<MoveGenerator>& current_move_generator);

/**
 * @ brief move the updated current_move_generator to its original move_Generator structure based on he placer_options and the agent state
 */
void update_move_generator(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, e_agent_state agent_state, const t_placer_opts& placer_opts, bool in_quench, std::unique_ptr<MoveGenerator>& current_move_generator);
#endif
