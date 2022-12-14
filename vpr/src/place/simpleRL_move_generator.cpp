#include "simpleRL_move_generator.h"
#include "globals.h"
#include <algorithm>
#include <numeric>

#include "vtr_random.h"

/* File-scope routines */
//a scaled and clipped exponential function
static float scaled_clipped_exp(float x) { return std::exp(std::min(1000000 * x, float(3.0))); }

/*                                     *
 *                                     *
 *  RL move generator implementation   *
 *                                     *
 *                                     */
SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<SoftmaxAgent>& agent) {
    avail_moves.emplace_back(std::make_unique<UniformMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<MedianMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<CentroidMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<WeightedCentroidMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<WeightedMedianMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<CriticalUniformMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<FeasibleRegionMoveGenerator>());

    karmed_bandit_agent = std::move(agent);
}

SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<EpsilonGreedyAgent>& agent) {
    avail_moves.emplace_back(std::make_unique<UniformMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<MedianMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<CentroidMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<WeightedCentroidMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<WeightedMedianMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<CriticalUniformMoveGenerator>());
    avail_moves.emplace_back(std::make_unique<FeasibleRegionMoveGenerator>());

    karmed_bandit_agent = std::move(agent);
}

e_create_move SimpleRLMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& move_type, t_logical_block_type& blk_type, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    auto propose_action_out = karmed_bandit_agent->propose_action();
    move_type = propose_action_out.move_type;
    blk_type = propose_action_out.blk_type;
    return avail_moves[(int)move_type]->propose_move(blocks_affected, move_type, blk_type, rlim, placer_opts, criticalities);
}

void SimpleRLMoveGenerator::process_outcome(double reward, e_reward_function reward_fun) {
    karmed_bandit_agent->process_outcome(reward, reward_fun);
}

/*                                        *
 *                                        *
 *  K-Armed bandit agent implementation   *
 *                                        *
 *                                        */
void KArmedBanditAgent::process_outcome(double reward, e_reward_function reward_fun) {
    ++num_action_chosen_[last_action_];
    if (reward_fun == RUNTIME_AWARE || reward_fun == WL_BIASED_RUNTIME_AWARE)
        reward /= time_elapsed_[last_action_ % num_available_moves_];

    //Determine step size
    float step = 0.;
    if (exp_alpha_ < 0.) {
        step = 1. / num_action_chosen_[last_action_]; //Incremental average
    } else if (exp_alpha_ <= 1) {
        step = exp_alpha_; //Exponentially wieghted average
    } else {
        VTR_ASSERT_MSG(false, "Invalid step size");
    }

    //Based on the outcome how much should our estimate of q change?
    float delta_q = step * (reward - q_[last_action_]);

    //Update the estimated value of the last action
    q_[last_action_] += delta_q;

    if (agent_info_file_) {
        fprintf(agent_info_file_, "%zu,", last_action_);
        fprintf(agent_info_file_, "%g,", reward);

        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            fprintf(agent_info_file_, "%g,", q_[i]);
        }

        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            fprintf(agent_info_file_, "%zu", num_action_chosen_[i]);
            if (i != num_available_moves_ * num_available_types_ - 1) {
                fprintf(agent_info_file_, ",");
            }
        }
        fprintf(agent_info_file_, "\n");
    }
}

/*                                  *
 *                                  *
 *  E-greedy agent implementation   *
 *                                  *
 *                                  */
EpsilonGreedyAgent::EpsilonGreedyAgent(size_t num_moves, float epsilon) {
    set_epsilon(epsilon);
    num_available_moves_ = num_moves;
    num_available_types_ = 1;
    init_q_scores();
}

EpsilonGreedyAgent::EpsilonGreedyAgent(size_t num_moves, size_t num_types, float epsilon) {
    set_epsilon(epsilon);
    num_available_moves_ = num_moves;
    num_available_types_ = num_types;
    init_q_scores();
}

EpsilonGreedyAgent::~EpsilonGreedyAgent() {
    if (agent_info_file_) vtr::fclose(agent_info_file_);
}

void EpsilonGreedyAgent::init_q_scores() {
    q_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    num_action_chosen_ = std::vector<size_t>(num_available_moves_ * num_available_types_, 0);

    cumm_epsilon_action_prob_ = std::vector<float>(num_available_moves_ * num_available_types_, 1.0 / (num_available_moves_ * num_available_types_));
    if (agent_info_file_) {
        fprintf(agent_info_file_, "action,reward,");
        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            fprintf(agent_info_file_, "q%zu,", i);
        }
        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            fprintf(agent_info_file_, "n%zu,", i);
        }
        fprintf(agent_info_file_, "\n");
    }
    set_epsilon_action_prob();
}

void EpsilonGreedyAgent::set_step(float gamma, int move_lim) {
    VTR_LOG("Setting egreedy step: %g\n", exp_alpha_);
    if (gamma < 0) {
        exp_alpha_ = -1; //Use sample average
    } else {
        //
        // For an exponentially wieghted average the fraction of total weight applied to
        // to moves which occured > K moves ago is:
        //
        //      gamma = (1 - alpha)^K
        //
        // If we treat K as the number of moves per temperature (move_lim) then gamma
        // is the fraction of weight applied to moves which occured > move_lim moves ago,
        // and given a target gamma we can explicitly calcualte the alpha step-size
        // required by the agent:
        //
        //     alpha = 1 - e^(log(gamma) / K)
        //
        float alpha = 1 - std::exp(std::log(gamma) / move_lim);
        exp_alpha_ = alpha;
    }
}

t_propose_action EpsilonGreedyAgent::propose_action() {
    size_t action = 0;
    t_logical_block_type blk_type;

    if (vtr::frand() < epsilon_) {
        /* Explore
         * With probability epsilon, choose randomly amongst all move types */
        float p = vtr::frand();
        auto itr = std::lower_bound(cumm_epsilon_action_prob_.begin(), cumm_epsilon_action_prob_.end(), p);
        auto action_type_q_pos = itr - cumm_epsilon_action_prob_.begin();
        action = (action_type_q_pos) % num_available_moves_;
        if (num_available_types_ != 1) {
            blk_type.index = action_type_q_pos / num_available_moves_;
        }

    } else {
        /* Greedy (Exploit)
         * For probability 1-epsilon, choose the greedy action */
        auto itr = std::max_element(q_.begin(), q_.end());
        VTR_ASSERT(itr != q_.end());
        auto action_type_q_pos = itr - q_.begin();
        action = action_type_q_pos % num_available_moves_;
        if (num_available_types_ != 1) {
            blk_type.index = action_type_q_pos / num_available_moves_;
        }
    }
    VTR_ASSERT(action < num_available_moves_);

    last_action_ = (num_available_types_ == 1) ? action : action + (blk_type.index * num_available_moves_);

    t_propose_action propose_action;
    propose_action.move_type = (e_move_type)action;
    propose_action.blk_type = blk_type;

    return propose_action;
}

void EpsilonGreedyAgent::set_epsilon(float epsilon) {
    VTR_LOG("Setting egreedy epsilon: %g\n", epsilon);
    epsilon_ = epsilon;
}

void EpsilonGreedyAgent::set_epsilon_action_prob() {
    //initialize to equal probabilities
    std::vector<float> epsilon_prob(num_available_moves_ * num_available_types_, 1.0 / (num_available_moves_ * num_available_types_));

    float accum = 0;
    for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
        accum += epsilon_prob[i];
        cumm_epsilon_action_prob_[i] = accum;
    }
}

/*                                  *
 *                                  *
 *  Softmax agent implementation    *
 *                                  *
 *                                  */
SoftmaxAgent::SoftmaxAgent(size_t num_moves) {
    num_available_moves_ = num_moves;
    num_available_types_ = 1;
    init_q_scores();
}

SoftmaxAgent::SoftmaxAgent(size_t num_moves, size_t num_types) {
    num_available_moves_ = num_moves;
    num_available_types_ = num_types;
    init_q_scores();
}

SoftmaxAgent::~SoftmaxAgent() {
    if (agent_info_file_) vtr::fclose(agent_info_file_);
}

void SoftmaxAgent::init_q_scores() {
    q_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    exp_q_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    num_action_chosen_ = std::vector<size_t>(num_available_moves_ * num_available_types_, 0);
    action_prob_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);

    cumm_action_prob_ = std::vector<float>(num_available_moves_ * num_available_types_);
    if (agent_info_file_) {
        fprintf(agent_info_file_, "action,reward,");
        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            fprintf(agent_info_file_, "q%zu,", i);
        }
        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            fprintf(agent_info_file_, "n%zu,", i);
        }
        fprintf(agent_info_file_, "\n");
    }
    set_action_prob();
    //agent_info_file_ = vtr::fopen("agent_info.txt", "w");
}

t_propose_action SoftmaxAgent::propose_action() {
    set_action_prob();
    size_t action = 0;
    t_logical_block_type blk_type;

    float p = vtr::frand();
    auto itr = std::lower_bound(cumm_action_prob_.begin(), cumm_action_prob_.end(), p);
    auto action_type_q_pos = itr - cumm_action_prob_.begin();
    action = (action_type_q_pos) % num_available_moves_;
    if (num_available_types_ != 1) {
        blk_type.index = action_type_q_pos / num_available_moves_;
    }

    //To take care that the last element in cumm_action_prob_ might be less than 1 by a small value
    if (action_type_q_pos == num_available_moves_ * num_available_types_) {
        action = num_available_moves_ - 1;
        if (num_available_types_ > 1) {
            blk_type.index = num_available_types_ - 1;
        }
    }

    VTR_ASSERT(action < num_available_moves_);

    last_action_ = (num_available_types_ == 1) ? action : action + (blk_type.index * num_available_moves_);

    t_propose_action propose_action;
    propose_action.move_type = (e_move_type)action;
    propose_action.blk_type = blk_type;

    return propose_action;
}

void SoftmaxAgent::set_action_prob() {
    //calculate the scaled and clipped explonential function for the estimated q value for each action
    std::transform(q_.begin(), q_.end(), exp_q_.begin(), scaled_clipped_exp);

    // calculate the sum of all scaled clipped expnential q values
    float sum_q = accumulate(exp_q_.begin(), exp_q_.end(), 0.0);

    auto& cluster_ctx = g_vpr_ctx.clustering();
    int num_total_blocks = cluster_ctx.clb_nlist.blocks().size();

    if(sum_q == num_available_types_ * num_available_moves_) {
        //action probabilities need to be initialized based on availability of the block type in the netlist
        for (size_t i = 0; i < num_available_moves_ * num_available_types_; i++) {
            t_logical_block_type blk_type;
            blk_type.index = i / num_available_moves_ + 1; //excluding the EMPTY type by adding one to the blk type index
            auto num_blocks = cluster_ctx.clb_nlist.blocks_per_type(blk_type).size();
            //q_[i] = (float) num_blocks / num_total_blocks;
//            q_[i] /= (num_available_moves_ * num_available_types_);

            action_prob_[i] = (float) num_blocks / num_total_blocks;
            action_prob_[i] /= (num_available_moves_);
        }
    } else {
        // calculate the probability of each action as the ratio of scaled_clipped_exp(action(i))/sum(scaled_clipped_exponentials)
        for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
            action_prob_[i] = exp_q_[i] / sum_q; //SARA_TODO: check its logic with Mohamed.
        }
    }
    // normalize all the action probabilities to guarantee the sum(all action probs) = 1
    float sum_prob = std::accumulate(action_prob_.begin(), action_prob_.end(), 0.0);
    std::transform(action_prob_.begin(), action_prob_.end(), action_prob_.begin(),
                   bind2nd(std::plus<float>(), (1.0 - sum_prob) / (num_available_moves_ * num_available_types_)));

    //calulcate the accumulative action probability of each action
    // e.g. if we have 5 actions with equal probability of 0.2, the cumm_action_prob will be {0.2,0.4,0.6,0.8,1.0}
    float accum = 0;
    for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
        accum += action_prob_[i];
        cumm_action_prob_[i] = accum;
    }
}

void SoftmaxAgent::set_step(float gamma, int move_lim) {
    if (gamma < 0) {
        exp_alpha_ = -1; //Use sample average
    } else {
        //
        // For an exponentially weighted average the fraction of total weight applied
        // to moves which occured > K moves ago is:
        //
        //      gamma = (1 - alpha)^K
        //
        // If we treat K as the number of moves per temperature (move_lim) then gamma
        // is the fraction of weight applied to moves which occured > move_lim moves ago,
        // and given a target gamma we can explicitly calcualte the alpha step-size
        // required by the agent:
        //
        //     alpha = 1 - e^(log(gamma) / K)
        //
        float alpha = 1 - std::exp(std::log(gamma) / move_lim);
        exp_alpha_ = alpha;
    }
}
