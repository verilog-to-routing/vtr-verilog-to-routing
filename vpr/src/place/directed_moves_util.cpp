#include "directed_moves_util.h"

void get_coordinate_of_pin(ClusterPinId pin, int& x, int& y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int pnum = tile_pin_index(pin);
    ClusterBlockId block = cluster_ctx.clb_nlist.pin_block(pin);

    x = place_ctx.block_locs[block].loc.x + physical_tile_type(block)->pin_width_offset[pnum];
    y = place_ctx.block_locs[block].loc.y + physical_tile_type(block)->pin_height_offset[pnum];

    x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
    y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels
}

void calculate_centroid_loc(ClusterBlockId b_from, bool timing_weights, t_pl_loc& centroid, const PlacerCriticalities* criticalities) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int x, y, ipin;
    float acc_weight = 0;
    float acc_x = 0;
    float acc_y = 0;
    float weight = 1;

    //iterate over the from block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        /* Ignore the special case nets which only connects a block to itself  *
         * Experimentally, it was found that this case greatly degrade QoR     */
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() == 1) {
            ClusterBlockId source = cluster_ctx.clb_nlist.net_driver_block(net_id);
            ClusterPinId sink_pin = *cluster_ctx.clb_nlist.net_sinks(net_id).begin();
            ClusterBlockId sink = cluster_ctx.clb_nlist.pin_block(sink_pin);
            if (sink == source) {
                continue;
            }
        }

        //if the pin is driver iterate over all the sinks
        if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
            if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                continue;

            for (auto sink_pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                /* Ignore if one of the sinks is the block itself      *
                 * This case rarely happens but causes QoR degradation */
                if (pin_id == sink_pin_id)
                    continue;
                ipin = cluster_ctx.clb_nlist.pin_net_index(sink_pin_id);
                if (timing_weights) {
                    weight = criticalities->criticality(net_id, ipin);
                } else {
                    weight = 1;
                }

                get_coordinate_of_pin(sink_pin_id, x, y);

                acc_x += x * weight;
                acc_y += y * weight;
                acc_weight += weight;
            }
        }

        //else the pin is sink --> only care about its driver
        else {
            ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            if (timing_weights) {
                weight = criticalities->criticality(net_id, ipin);
            } else {
                weight = 1;
            }

            ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);

            get_coordinate_of_pin(source_pin, x, y);

            acc_x += x * weight;
            acc_y += y * weight;
            acc_weight += weight;
        }
    }

    //Calculate the centroid location
    centroid.x = acc_x / acc_weight;
    centroid.y = acc_y / acc_weight;
}

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
        /* For the non timing driven placecment: the agent has a single state  *
         *     - Available actions are (Uniform / Median / Centroid)           *
         *                                                                     *
         * For the timing driven placement: the agent has two states           *
         *     - 1st state: includes 4 actions (Uniform / Median / Centroid /  *
         *                  WeightedCentroid)                                  *
         *     - 2nd state: includes 7 actions (Uniform / Median / Centroid /  *
         *                  WeightedCentroid / WeightedMedian / Feasible       *
         *                  Region / CriticalUniform)                          *
         *                                                                     *
         *      This state is activated late in the anneale and in the Quench  */

        if (placer_opts.place_agent_algorithm == E_GREEDY) {
            VTR_LOG("Using simple RL 'Epsilon Greedy agent' for choosing move types\n");
            std::unique_ptr<EpsilonGreedyAgent> karmed_bandit_agent1, karmed_bandit_agent2;
            if (placer_opts.place_algorithm.is_timing_driven()) {
                //agent's 1st state
                karmed_bandit_agent1 = std::make_unique<EpsilonGreedyAgent>(NUM_PL_1ST_STATE_MOVE_TYPES, placer_opts.place_agent_epsilon);
                karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1);

                //agent's 2nd state
                karmed_bandit_agent2 = std::make_unique<EpsilonGreedyAgent>(NUM_PL_MOVE_TYPES, placer_opts.place_agent_epsilon);
                karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2);
            } else {
                //agent's 1st state
                karmed_bandit_agent1 = std::make_unique<EpsilonGreedyAgent>(NUM_PL_NONTIMING_MOVE_TYPES, placer_opts.place_agent_epsilon);
                karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1);

                //agent's 2nd state
                karmed_bandit_agent2 = std::make_unique<EpsilonGreedyAgent>(NUM_PL_NONTIMING_MOVE_TYPES, placer_opts.place_agent_epsilon);
                karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2);
            }
        } else {
            VTR_LOG("Using simple RL 'Softmax agent' for choosing move types\n");
            std::unique_ptr<SoftmaxAgent> karmed_bandit_agent1, karmed_bandit_agent2;

            if (placer_opts.place_algorithm.is_timing_driven()) {
                //agent's 1st state
                karmed_bandit_agent1 = std::make_unique<SoftmaxAgent>(NUM_PL_1ST_STATE_MOVE_TYPES);
                karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1);
                //agent's 2nd state
                karmed_bandit_agent2 = std::make_unique<SoftmaxAgent>(NUM_PL_MOVE_TYPES);
                karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2);
            } else {
                //agent's 1st state
                karmed_bandit_agent1 = std::make_unique<SoftmaxAgent>(NUM_PL_NONTIMING_MOVE_TYPES);
                karmed_bandit_agent1->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent1);
                //agent's 2nd state
                karmed_bandit_agent2 = std::make_unique<SoftmaxAgent>(NUM_PL_NONTIMING_MOVE_TYPES);
                karmed_bandit_agent2->set_step(placer_opts.place_agent_gamma, move_lim);
                move_generator2 = std::make_unique<SimpleRLMoveGenerator>(karmed_bandit_agent2);
            }
        }
    }
}
