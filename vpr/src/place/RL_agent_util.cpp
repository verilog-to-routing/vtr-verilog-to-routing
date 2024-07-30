#include "RL_agent_util.h"
#include "static_move_generator.h"
#include "manual_move_generator.h"

void create_move_generators(std::unique_ptr<MoveGenerator>& move_generator,
                            std::unique_ptr<MoveGenerator>& move_generator2,
                            const t_placer_opts& placer_opts,
                            int move_lim,
                            float noc_attraction_weight) {
    if (!placer_opts.RL_agent_placement) { // RL agent is disabled
        auto move_types = placer_opts.place_static_move_prob;
        move_types.resize((int)e_move_type::NUMBER_OF_AUTO_MOVES, 0.0f);

        VTR_LOG("Using static probabilities for choosing each move type\n");
        for (const auto move_type : placer_opts.place_static_move_prob.keys()) {
            const std::string& move_name = move_type_to_string(move_type);
            VTR_LOG("Probability of %s : %f \n",
                    move_name.c_str(),
                    placer_opts.place_static_move_prob[move_type]);
        }
        move_generator = std::make_unique<StaticMoveGenerator>(placer_opts.place_static_move_prob);
        move_generator2 = std::make_unique<StaticMoveGenerator>(placer_opts.place_static_move_prob);
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

        std::vector<e_move_type> first_state_avail_moves{e_move_type::UNIFORM, e_move_type::MEDIAN, e_move_type::CENTROID};
        if (placer_opts.place_algorithm.is_timing_driven()) {
            first_state_avail_moves.push_back(e_move_type::W_CENTROID);
        }

        std::vector<e_move_type> second_state_avail_moves{e_move_type::UNIFORM, e_move_type::MEDIAN, e_move_type::CENTROID};
        if (placer_opts.place_algorithm.is_timing_driven()) {
            second_state_avail_moves.insert(second_state_avail_moves.end(),
                                            {e_move_type::W_CENTROID, e_move_type::W_MEDIAN, e_move_type::CRIT_UNIFORM, e_move_type::FEASIBLE_REGION});
        }

        if (noc_attraction_weight > 0.0f) {
            first_state_avail_moves.push_back(e_move_type::NOC_ATTRACTION_CENTROID);
            second_state_avail_moves.push_back(e_move_type::NOC_ATTRACTION_CENTROID);
        }

        if (placer_opts.place_agent_algorithm == E_GREEDY) {
            std::unique_ptr<EpsilonGreedyAgent> karmed_bandit_agent1, karmed_bandit_agent2;
            //agent's 1st state
            if (placer_opts.place_agent_space == e_agent_space::MOVE_BLOCK_TYPE) {
                VTR_LOG("Using simple RL 'Epsilon Greedy agent' for choosing move and block types\n");
                karmed_bandit_agent1 = std::make_unique<EpsilonGreedyAgent>(first_state_avail_moves,
                                                                            e_agent_space::MOVE_BLOCK_TYPE,
                                                                            placer_opts.place_agent_epsilon);
            } else {
                VTR_LOG("Using simple RL 'Epsilon Greedy agent' for choosing move types\n");
                karmed_bandit_agent1 = std::make_unique<EpsilonGreedyAgent>(first_state_avail_moves,
                                                                            e_agent_space::MOVE_TYPE,
                                                                            placer_opts.place_agent_epsilon);
            }
            karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1,
                                                                     noc_attraction_weight,
                                                                     placer_opts.place_high_fanout_net);
            //agent's 2nd state
            karmed_bandit_agent2 = std::make_unique<EpsilonGreedyAgent>(second_state_avail_moves,
                                                                        e_agent_space::MOVE_TYPE,
                                                                        placer_opts.place_agent_epsilon);
            karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2,
                                                                      noc_attraction_weight,
                                                                      placer_opts.place_high_fanout_net);
        } else {
            std::unique_ptr<SoftmaxAgent> karmed_bandit_agent1, karmed_bandit_agent2;
            //agent's 1st state
            if (placer_opts.place_agent_space == e_agent_space::MOVE_BLOCK_TYPE) {
                VTR_LOG("Using simple RL 'Softmax agent' for choosing move and block types\n");
                karmed_bandit_agent1 = std::make_unique<SoftmaxAgent>(first_state_avail_moves,
                                                                      e_agent_space::MOVE_BLOCK_TYPE);
            } else {
                VTR_LOG("Using simple RL 'Softmax agent' for choosing move types\n");
                karmed_bandit_agent1 = std::make_unique<SoftmaxAgent>(first_state_avail_moves,
                                                                      e_agent_space::MOVE_TYPE);
            }
            karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1,
                                                                     noc_attraction_weight,
                                                                     placer_opts.place_high_fanout_net);
            //agent's 2nd state
            karmed_bandit_agent2 = std::make_unique<SoftmaxAgent>(second_state_avail_moves,
                                                                  e_agent_space::MOVE_TYPE);
            karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
            move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2,
                                                                      noc_attraction_weight,
                                                                      placer_opts.place_high_fanout_net);
        }
    }
}

void assign_current_move_generator(std::unique_ptr<MoveGenerator>& move_generator,
                                   std::unique_ptr<MoveGenerator>& move_generator2,
                                   e_agent_state agent_state,
                                   const t_placer_opts& placer_opts,
                                   bool in_quench,
                                   std::unique_ptr<MoveGenerator>& current_move_generator) {
    if (in_quench) {
        if (placer_opts.place_quench_algorithm.is_timing_driven() && placer_opts.place_agent_multistate)
            current_move_generator = std::move(move_generator2);
        else
            current_move_generator = std::move(move_generator);
    } else {
        if (agent_state == e_agent_state::EARLY_IN_THE_ANNEAL || !placer_opts.place_agent_multistate)
            current_move_generator = std::move(move_generator);
        else
            current_move_generator = std::move(move_generator2);
    }
}

void update_move_generator(std::unique_ptr<MoveGenerator>& move_generator,
                           std::unique_ptr<MoveGenerator>& move_generator2,
                           e_agent_state agent_state,
                           const t_placer_opts& placer_opts,
                           bool in_quench,
                           std::unique_ptr<MoveGenerator>& current_move_generator) {
    if (in_quench) {
        if (placer_opts.place_quench_algorithm.is_timing_driven() && placer_opts.place_agent_multistate)
            move_generator2 = std::move(current_move_generator);
        else
            move_generator = std::move(current_move_generator);
    } else {
        if (agent_state == e_agent_state::EARLY_IN_THE_ANNEAL || !placer_opts.place_agent_multistate)
            move_generator = std::move(current_move_generator);
        else
            move_generator2 = std::move(current_move_generator);
    }
}