#include "simpleRL_move_generator.h"
#include "globals.h"
#include <algorithm>
#include <numeric>

#include "vtr_random.h"

float my_exp(float);

/*                                     *
 *                                     *
 *  RL move generator implementation   *
 *                                     *
 *                                     */
SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<SoftmaxAgent>& agent) {
    avail_moves.push_back(std::move(std::make_unique<UniformMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<MedianMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<CentroidMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<WeightedCentroidMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<WeightedMedianMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<CriticalUniformMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<FeasibleRegionMoveGenerator>()));

    karmed_bandit_agent = std::move(agent);
}

SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<EpsilonGreedyAgent>& agent) {
    avail_moves.push_back(std::move(std::make_unique<UniformMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<MedianMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<CentroidMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<WeightedCentroidMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<WeightedMedianMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<CriticalUniformMoveGenerator>()));
    avail_moves.push_back(std::move(std::make_unique<FeasibleRegionMoveGenerator>()));

    karmed_bandit_agent = std::move(agent);
}

e_create_move SimpleRLMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim, std::vector<int>& X_coord, std::vector<int>& Y_coord, e_move_type& move_type, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    move_type = (e_move_type)karmed_bandit_agent->propose_action();
    return avail_moves[(int)move_type]->propose_move(blocks_affected, rlim, X_coord, Y_coord, move_type, placer_opts, criticalities);
}

void SimpleRLMoveGenerator::process_outcome(double reward, std::string reward_fun) {
    karmed_bandit_agent->process_outcome(reward, reward_fun);
}

/*                                        *
 *                                        *
 *  K-Armed bandit agent implementation   *
 *                                        *
 *                                        */
void KArmedBanditAgent::process_outcome(double reward, std::string reward_fun) {
    ++n_[last_action_];
    if (reward_fun == "runtime_aware" || reward_fun == "WLbiased_runtime_aware")
        reward /= time_elapsed_[last_action_];

    //Determine step size
    float step = 0.;
    if (exp_alpha_ < 0.) {
        step = 1. / n_[last_action_]; //Incremental average
    } else if (exp_alpha_ <= 1) {
        step = exp_alpha_; //Exponentially wieghted average
    } else {
        VTR_ASSERT_MSG(false, "Invalid step size");
    }

    //Based on the outcome how much should our estimate of q change?
    float delta_q = step * (reward - q_[last_action_]);

    //Update the estimated value of the last action
    q_[last_action_] += delta_q;

    if (f_) {
        fprintf(f_, "%zu,", last_action_);
        fprintf(f_, "%g,", reward);

        for (size_t i = 0; i < k_; ++i) {
            fprintf(f_, "%g,", q_[i]);
        }

        for (size_t i = 0; i < k_; ++i) {
            fprintf(f_, "%zu", n_[i]);
            if (i != k_ - 1) {
                fprintf(f_, ",");
            }
        }
        fprintf(f_, "\n");
    }
}

/*                                  *
 *                                  *
 *  E-greedy agent implementation   *
 *                                  *
 *                                  */
EpsilonGreedyAgent::EpsilonGreedyAgent(size_t k, float epsilon) {
    set_epsilon(epsilon);
    set_k(k);
    set_epsilon_action_prob();
}

EpsilonGreedyAgent::~EpsilonGreedyAgent() {
    if (f_) vtr::fclose(f_);
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

size_t EpsilonGreedyAgent::propose_action() {
    size_t action = 0;
    if (vtr::frand() < epsilon_) {
        //Explore
        float p = vtr::frand();
        auto itr = std::lower_bound(cumm_epsilon_action_prob_.begin(), cumm_epsilon_action_prob_.end(), p);
        action = itr - cumm_epsilon_action_prob_.begin();
    } else {
        //Greedy (Exploit)
        auto itr = std::max_element(q_.begin(), q_.end());
        VTR_ASSERT(itr != q_.end());
        action = itr - q_.begin();
    }
    VTR_ASSERT(action < k_);

    last_action_ = action;
    return action;
}

void EpsilonGreedyAgent::set_epsilon(float epsilon) {
    VTR_LOG("Setting egreedy epsilon: %g\n", epsilon);
    epsilon_ = epsilon;
}

void EpsilonGreedyAgent::set_k(size_t k) {
    k_ = k;
    q_ = std::vector<float>(k, 0.);
    n_ = std::vector<size_t>(k, 0);

    cumm_epsilon_action_prob_ = std::vector<float>(k, 1.0 / k);
    if (f_) {
        fprintf(f_, "action,reward,");
        for (size_t i = 0; i < k_; ++i) {
            fprintf(f_, "q%zu,", i);
        }
        for (size_t i = 0; i < k_; ++i) {
            fprintf(f_, "n%zu,", i);
        }
        fprintf(f_, "\n");
    }
}

void EpsilonGreedyAgent::set_epsilon_action_prob() {
    //initialize to equal probabilities
    std::vector<float> epsilon_prob(k_, 1.0 / k_);

    float accum = 0;
    for (size_t i = 0; i < k_; ++i) {
        accum += epsilon_prob[i];
        cumm_epsilon_action_prob_[i] = accum;
    }
}

/*                                  *
 *                                  *
 *  Softmax agent implementation    *
 *                                  *
 *                                  */
SoftmaxAgent::SoftmaxAgent(size_t k) {
    set_k(k);
    set_action_prob();
    //f_ = vtr::fopen("agent_info.txt", "w");
}

SoftmaxAgent::~SoftmaxAgent() {
    if (f_) vtr::fclose(f_);
}

size_t SoftmaxAgent::propose_action() {
    set_action_prob();
    size_t action = 0;
    float p = vtr::frand();
    auto itr = std::lower_bound(cumm_action_prob_.begin(), cumm_action_prob_.end(), p);
    action = itr - cumm_action_prob_.begin();
    //To take care that the last element in cumm_action_prob_ might be less than 1 by a small value
    if (action == k_)
        action = k_ - 1;
    VTR_ASSERT(action < k_);

    last_action_ = action;
    return action;
}

void SoftmaxAgent::set_k(size_t k) {
    k_ = k;
    q_ = std::vector<float>(k, 0.);
    exp_q_ = std::vector<float>(k, 0.);
    n_ = std::vector<size_t>(k, 0);
    action_prob_ = std::vector<float>(k, 0.);

    cumm_action_prob_ = std::vector<float>(k);
    if (f_) {
        fprintf(f_, "action,reward,");
        for (size_t i = 0; i < k_; ++i) {
            fprintf(f_, "q%zu,", i);
        }
        for (size_t i = 0; i < k_; ++i) {
            fprintf(f_, "n%zu,", i);
        }
        fprintf(f_, "\n");
    }
}

float my_exp(float x) { return std::exp(std::min(1000000 * x, float(3.0))); }

void SoftmaxAgent::set_action_prob() {
    std::transform(q_.begin(), q_.end(), exp_q_.begin(), my_exp);
    float sum_q = accumulate(exp_q_.begin(), exp_q_.end(), 0.0);

    if (sum_q == 0.0) {
        std::fill(action_prob_.begin(), action_prob_.end(), 1.0 / k_);
    } else {
        for (size_t i = 0; i < k_; ++i) {
            action_prob_[i] = exp_q_[i] / sum_q;
        }
    }

    float sum_prob = std::accumulate(action_prob_.begin(), action_prob_.end(), 0.0);
    std::transform(action_prob_.begin(), action_prob_.end(), action_prob_.begin(),
                   bind2nd(std::plus<float>(), (1.0 - sum_prob) / k_));
    float accum = 0;
    for (size_t i = 0; i < k_; ++i) {
        accum += action_prob_[i];
        cumm_action_prob_[i] = accum;
    }
}

void SoftmaxAgent::set_step(float gamma, int move_lim) {
    VTR_LOG("Setting softmax step: %g\n", exp_alpha_);
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
