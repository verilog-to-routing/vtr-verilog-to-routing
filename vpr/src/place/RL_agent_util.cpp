#include "RL_agent_util.h"
#include "manual_move_generator.h"

void create_move_generators(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, const t_placer_opts& placer_opts, int move_lim) {
    if (placer_opts.RL_agent_placement == false) {
        if (placer_opts.place_algorithm.is_timing_driven()) {
            VTR_LOG("Using static probabilities for choosing each move type\n");
            VTR_LOG("Probability of Uniform_move : %f \n", placer_opts.place_static_move_prob[0]);
            VTR_LOG("Probability of Median_move : %f \n", placer_opts.place_static_move_prob[1]);
            VTR_LOG("Probability of Centroid_move : %f \n", placer_opts.place_static_move_prob[2]);
            VTR_LOG("Probability of Weighted_centroid_move : %f \n", placer_opts.place_static_move_prob[3]);
            VTR_LOG("Probability of Weighted_median_move : %f \n", placer_opts.place_static_move_prob[4]);
            VTR_LOG("Probability of Timing_feasible_region_move : %f \n", placer_opts.place_static_move_prob[5]);
            VTR_LOG("Probability of Critical_uniform_move : %f \n", placer_opts.place_static_move_prob[6]);
            move_generator = std::make_unique<StaticMoveGenerator>(placer_opts.place_static_move_prob);
            move_generator2 = std::make_unique<StaticMoveGenerator>(placer_opts.place_static_move_prob);
        } else { //Non-timing driven placement
            VTR_LOG("Using static probabilities for choosing each move type\n");
            VTR_LOG("Probability of Uniform_move : %f \n", placer_opts.place_static_notiming_move_prob[0]);
            VTR_LOG("Probability of Median_move : %f \n", placer_opts.place_static_notiming_move_prob[1]);
            VTR_LOG("Probability of Centroid_move : %f \n", placer_opts.place_static_notiming_move_prob[2]);
            move_generator = std::make_unique<StaticMoveGenerator>(placer_opts.place_static_notiming_move_prob);
            move_generator2 = std::make_unique<StaticMoveGenerator>(placer_opts.place_static_notiming_move_prob);
        }
    } else { //RL based placement
        /* For the non timing driven placement: the agent has a single state   *
         *     - Available moves are (Uniform / Median / Centroid)             *
         *                                                                     *
         * For the timing driven placement: the agent has two states           *
         *     - 1st state: includes 4 moves (Uniform / Median / Centroid /    *
         *                  WeightedCentroid)                                  *
         *      If agent should propose block type as well as the mentioned    *
         *      move types, 1st state Q-table size is:                          *
         *                 4 move types * number of block types in the netlist *
         *      if not, the Q-table size is : 4                                *
         *                                                                     *
         *                                                                     *
         *     - 2nd state: includes 7 moves (Uniform / Median / Centroid /    *
         *                  WeightedCentroid / WeightedMedian / Feasible       *
         *                  Region / CriticalUniform)                          *
         *      2nd state agent Q-table size is always 7 and always proposes   *
         *      only move type.                                                *
         *      This state is activated late in the anneal and in the Quench   */

        int num_1st_state_avail_moves = placer_opts.place_algorithm.is_timing_driven() ? NUM_PL_1ST_STATE_MOVE_TYPES : NUM_PL_NONTIMING_MOVE_TYPES;
        int num_2nd_state_avail_moves = placer_opts.place_algorithm.is_timing_driven() ? NUM_PL_MOVE_TYPES : NUM_PL_NONTIMING_MOVE_TYPES;

        if (placer_opts.place_agent_algorithm == E_GREEDY) {
            std::unique_ptr<EpsilonGreedyAgent> karmed_bandit_agent1, karmed_bandit_agent2;
            //agent's 1st state
            if (placer_opts.place_agent_space == e_agent_space::MOVE_BLOCK_TYPE) {
                VTR_LOG("Using simple RL 'Epsilon Greedy agent' for choosing move and block types\n");
                karmed_bandit_agent1 = std::make_unique<EpsilonGreedyAgent>(num_1st_state_avail_moves,
                                                                            e_agent_space::MOVE_BLOCK_TYPE,
                                                                            placer_opts.place_agent_epsilon);
            } else {
                VTR_LOG("Using simple RL 'Epsilon Greedy agent' for choosing move types\n");
                karmed_bandit_agent1 = std::make_unique<EpsilonGreedyAgent>(num_1st_state_avail_moves,
                                                                            e_agent_space::MOVE_TYPE,
                                                                            placer_opts.place_agent_epsilon);
            }
            karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1);
            //agent's 2nd state
            karmed_bandit_agent2 = std::make_unique<EpsilonGreedyAgent>(num_2nd_state_avail_moves,
                                                                        e_agent_space::MOVE_TYPE,
                                                                        placer_opts.place_agent_epsilon);
            karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2);
        } else {
            std::unique_ptr<SoftmaxAgent> karmed_bandit_agent1, karmed_bandit_agent2;
            //agent's 1st state
            if (placer_opts.place_agent_space == e_agent_space::MOVE_BLOCK_TYPE) {
                VTR_LOG("Using simple RL 'Softmax agent' for choosing move and block types\n");
                karmed_bandit_agent1 = std::make_unique<SoftmaxAgent>(num_1st_state_avail_moves,
                                                                      e_agent_space::MOVE_BLOCK_TYPE);
            } else {
                VTR_LOG("Using simple RL 'Softmax agent' for choosing move types\n");
                karmed_bandit_agent1 = std::make_unique<SoftmaxAgent>(num_1st_state_avail_moves,
                                                                      e_agent_space::MOVE_TYPE);
            }
            karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1);
            //agent's 2nd state
            karmed_bandit_agent2 = std::make_unique<SoftmaxAgent>(num_2nd_state_avail_moves,
                                                                  e_agent_space::MOVE_TYPE);
            karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2);
        }
    }
}

void assign_current_move_generator(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, e_agent_state agent_state, const t_placer_opts& placer_opts, bool in_quench, std::unique_ptr<MoveGenerator>& current_move_generator) {
    if (in_quench) {
        if (placer_opts.place_quench_algorithm.is_timing_driven() && placer_opts.place_agent_multistate == true)
            current_move_generator = std::move(move_generator2);
        else
            current_move_generator = std::move(move_generator);
    } else {
        if (agent_state == EARLY_IN_THE_ANNEAL || placer_opts.place_agent_multistate == false)
            current_move_generator = std::move(move_generator);
        else
            current_move_generator = std::move(move_generator2);
    }
}

void update_move_generator(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, e_agent_state agent_state, const t_placer_opts& placer_opts, bool in_quench, std::unique_ptr<MoveGenerator>& current_move_generator) {
    if (in_quench) {
        if (placer_opts.place_quench_algorithm.is_timing_driven() && placer_opts.place_agent_multistate == true)
            move_generator2 = std::move(current_move_generator);
        else
            move_generator = std::move(current_move_generator);
    } else {
        if (agent_state == EARLY_IN_THE_ANNEAL || placer_opts.place_agent_multistate == false)
            move_generator = std::move(current_move_generator);
        else
            move_generator2 = std::move(current_move_generator);
    }
}