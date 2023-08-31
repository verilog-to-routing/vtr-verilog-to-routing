#include "simpleRL_move_generator.h"
#include "globals.h"
#include <algorithm>
#include <numeric>

#include "vtr_random.h"
#include "vtr_time.h"
/* File-scope routines */
//a scaled and clipped exponential function
static float scaled_clipped_exp(float x) { return std::exp(std::min(1000 * x, float(3.0))); }

/*                                     *
 *                                     *
 *  RL move generator implementation   *
 *                                     *
 *                                     */
e_create_move SimpleRLMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& proposed_action, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    proposed_action = karmed_bandit_agent->propose_action();
    return avail_moves[(int)proposed_action.move_type]->propose_move(blocks_affected, proposed_action, rlim, placer_opts, criticalities);
}

void SimpleRLMoveGenerator::process_outcome(double reward, e_reward_function reward_fun) {
    karmed_bandit_agent->process_outcome(reward, reward_fun);
}

/*                                        *
 *                                        *
 *  K-Armed bandit agent implementation   *
 *                                        *
 *                                        */
KArmedBanditAgent::KArmedBanditAgent(size_t num_moves, e_agent_space agent_space)
    : num_available_moves_(num_moves)
    , propose_blk_type_(agent_space == e_agent_space::MOVE_BLOCK_TYPE) {
    std::vector<int> available_logical_block_types = get_available_logical_blk_types_();
    num_available_types_ = available_logical_block_types.size();

    num_available_actions_ = propose_blk_type_ ? (num_available_moves_ * num_available_types_) : num_available_moves_;

    action_logical_blk_type_.clear();

    for (auto logical_blk_type_idx : available_logical_block_types) {
        action_logical_blk_type_.push_back(logical_blk_type_idx);
    }
}

/*
 * If the agent selects both move type and block type, the would lool like this:
 *
 *    +---------------+---------------+---------------+---------------+
 *    | (blk0, move0) | (blk0, move1) | ............. | (blk0, moveN) |
 *    +---------------+---------------+---------------+---------------+
 *    | (blk1, move0) | (blk1, move1) | ............. | (blk1, moveN) |
 *    +---------------+---------------+---------------+---------------+
 *    |      ..       |       ..      | ............. |       ..      |
 *    +---------------+---------------+---------------+---------------+
 *    | (blkK, move0) | (blkK, move1) | ............. | (blkK, moveN) |
 *    +---------------+---------------+---------------+---------------+
 *
 *    This meant that (action_idx % num_available_moves_) specifies the move type,
 *    while (action_idx / num_available_moves_) determines the block type.
 *
 */
e_move_type KArmedBanditAgent::action_to_move_type_(const size_t action_idx) {
    e_move_type move_type = e_move_type::INVALID_MOVE;

    if (action_idx < num_available_actions_) {
        move_type = (e_move_type)(action_idx % num_available_moves_);
    }

    return move_type;
}

int KArmedBanditAgent::action_to_blk_type_(const size_t action_idx) {
    if (propose_blk_type_) {
        return action_logical_blk_type_.at(action_idx / num_available_moves_);
    } else { // the agent doesn't select the move type
        return -1;
    }
}

std::vector<int> KArmedBanditAgent::get_available_logical_blk_types_() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::vector<int> available_blk_types;

    for (const auto& logical_blk_type : device_ctx.logical_block_types) {
        if (logical_blk_type.index == 0) { //ignore empty type
            continue;
        }

        const auto& blk_per_type = cluster_ctx.clb_nlist.blocks_per_type(logical_blk_type);

        if (!blk_per_type.empty()) {
            available_blk_types.push_back(logical_blk_type.index);
        }
    }

    return available_blk_types;
}

void KArmedBanditAgent::process_outcome(double reward, e_reward_function reward_fun) {
    ++num_action_chosen_[last_action_];
    if (reward_fun == RUNTIME_AWARE || reward_fun == WL_BIASED_RUNTIME_AWARE)
        reward /= time_elapsed_[last_action_ % num_available_moves_];

    //Determine step size
    float step = 0.;
    if (exp_alpha_ < 0.) {
        step = 1.0f / (float)num_action_chosen_[last_action_]; //Incremental average
    } else if (exp_alpha_ <= 1) {
        step = exp_alpha_; //Exponentially weighted average
    } else {
        VTR_ASSERT_MSG(false, "Invalid step size");
    }

    //Based on the outcome how much should our estimate of q change?
    float delta_q = step * (reward - q_[last_action_]);

    //Update the estimated value of the last action
    q_[last_action_] += delta_q;

    //write agent internal q-table and actions into a file for debugging purposes
    //agent_info_file_ variable is a NULL pointer by default
    //info file is not generated unless the agent_info_file_ set to a filename in "init_q_scores" function
    if (agent_info_file_) {
        write_agent_info(last_action_, reward);
    }
}

void KArmedBanditAgent::write_agent_info(int last_action, double reward) {
    fseek(agent_info_file_, 0, SEEK_END);
    fprintf(agent_info_file_, "%d,", last_action);
    fprintf(agent_info_file_, "%g,", reward);

    for (size_t i = 0; i < num_available_actions_; ++i) {
        fprintf(agent_info_file_, "%g,", q_[i]);
    }

    for (size_t i = 0; i < num_available_actions_; ++i) {
        fprintf(agent_info_file_, "%zu,", num_action_chosen_[i]);
    }
    fprintf(agent_info_file_, "\n");
    fflush(agent_info_file_);
}

void KArmedBanditAgent::set_step(float gamma, int move_lim) {
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

int KArmedBanditAgent::agent_to_phy_blk_type(const int idx) {
    return action_logical_blk_type_.at(idx);
}
/*                                  *
 *                                  *
 *  E-greedy agent implementation   *
 *                                  *
 *                                  */
EpsilonGreedyAgent::EpsilonGreedyAgent(size_t num_moves, e_agent_space agent_space, float epsilon)
    : KArmedBanditAgent(num_moves, agent_space) {
    set_epsilon(epsilon);
    init_q_scores_();
}

EpsilonGreedyAgent::~EpsilonGreedyAgent() {
    if (agent_info_file_) vtr::fclose(agent_info_file_);
}

void EpsilonGreedyAgent::init_q_scores_() {
    q_ = std::vector<float>(num_available_actions_, 0.);
    num_action_chosen_ = std::vector<size_t>(num_available_actions_, 0);
    cumm_epsilon_action_prob_ = std::vector<float>(num_available_actions_, 1.0 / (num_available_actions_));

    //agent_info_file_ = vtr::fopen("agent_info.txt", "w");
    //write agent internal q-table and actions into file for debugging purposes
    if (agent_info_file_) {
        //we haven't performed any moves yet, hence last_aciton and reward are 0
        write_agent_info(0, 0);
    }

    set_epsilon_action_prob();
}

t_propose_action EpsilonGreedyAgent::propose_action() {
    if (vtr::frand() < epsilon_) {
        /* Explore
         * With probability epsilon, choose randomly amongst all move types */
        float p = vtr::frand();
        auto itr = std::lower_bound(cumm_epsilon_action_prob_.begin(), cumm_epsilon_action_prob_.end(), p);
        auto action_type_q_pos = itr - cumm_epsilon_action_prob_.begin();
        //Mark the q_table location that agent used to update its value after processing the move outcome
        last_action_ = action_type_q_pos;

    } else {
        /* Greedy (Exploit)
         * For probability 1-epsilon, choose the greedy move_type */
        auto itr = std::max_element(q_.begin(), q_.end());
        VTR_ASSERT(itr != q_.end());
        auto action_type_q_pos = itr - q_.begin();
        //Mark the q_table location that agent used to update its value after processing the move outcome
        last_action_ = action_type_q_pos;
    }

    t_propose_action proposed_action{action_to_move_type_(last_action_),
                                     action_to_blk_type_(last_action_)};

    //Check the move type to be a valid move
    VTR_ASSERT((size_t)proposed_action.move_type < num_available_moves_);

    return proposed_action;
}

void EpsilonGreedyAgent::set_epsilon(float epsilon) {
    VTR_LOG("Setting egreedy epsilon: %g\n", epsilon);
    epsilon_ = epsilon;
}

void EpsilonGreedyAgent::set_epsilon_action_prob() {
    //initialize to equal probabilities
    std::vector<float> epsilon_prob(num_available_actions_, 1.0 / (num_available_actions_));

    float accum = 0;
    for (size_t i = 0; i < num_available_actions_; ++i) {
        accum += epsilon_prob[i];
        cumm_epsilon_action_prob_[i] = accum;
    }
}

/*                                  *
 *                                  *
 *  Softmax agent implementation    *
 *                                  *
 *                                  */
SoftmaxAgent::SoftmaxAgent(size_t num_moves, e_agent_space agent_space)
    : KArmedBanditAgent(num_moves, agent_space) {
    init_q_scores_();
}

SoftmaxAgent::~SoftmaxAgent() {
    if (agent_info_file_) vtr::fclose(agent_info_file_);
}

void SoftmaxAgent::init_q_scores_() {
    q_ = std::vector<float>(num_available_actions_, 0.);
    exp_q_ = std::vector<float>(num_available_actions_, 0.);
    num_action_chosen_ = std::vector<size_t>(num_available_actions_, 0);
    action_prob_ = std::vector<float>(num_available_actions_, 0.);
    block_type_ratio_ = std::vector<float>(num_available_types_, 0.);
    cumm_action_prob_ = std::vector<float>(num_available_actions_);

    //    agent_info_file_ = vtr::fopen("agent_info.txt", "w");
    //write agent internal q-table and actions into file for debugging purposes
    if (agent_info_file_) {
        //we haven't performed any moves yet, hence last_aciton and reward are 0
        write_agent_info(0, 0);
    }

    /*
     * The agent calculates each block type ratio as: (# blocks of each type / total blocks).
     * If the agent is supposed to propose both block type and move type,
     * it will use the block ratio to calculate action probability for each q_table entry.
     */
    if (propose_blk_type_) {
        set_block_ratio_();
    }
    set_action_prob_();
}

t_propose_action SoftmaxAgent::propose_action() {
    set_action_prob_();

    float p = vtr::frand();
    auto itr = std::lower_bound(cumm_action_prob_.begin(), cumm_action_prob_.end(), p);
    auto action_type_q_pos = itr - cumm_action_prob_.begin();
    //To take care that the last element in cumm_action_prob_ might be less than 1 by a small value
    last_action_ = std::min((size_t)action_type_q_pos, num_available_actions_ - 1);

    t_propose_action proposed_action{action_to_move_type_(last_action_),
                                     action_to_blk_type_(last_action_)};

    //Check the move type to be a valid move
    VTR_ASSERT((size_t)proposed_action.move_type < num_available_moves_);

    return proposed_action;
}

void SoftmaxAgent::set_block_ratio_() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    size_t num_total_blocks = cluster_ctx.clb_nlist.blocks().size();

    // allocate enough space for available block types in the netlist
    block_type_ratio_.resize(num_available_types_);

    /* Calculate ratio of each block as : (# blocks of each type / total blocks).
     * Each block type can have "num_available_moves_" different moves. Hence,
     * the ratio will be divided by num_available_moves_ at the end.
     */
    for (size_t itype = 0; itype < num_available_types_; itype++) {
        t_logical_block_type blk_type;
        blk_type.index = agent_to_phy_blk_type(itype);
        auto num_blocks = cluster_ctx.clb_nlist.blocks_per_type(blk_type).size();
        block_type_ratio_[itype] = (float)num_blocks / num_total_blocks;
        block_type_ratio_[itype] /= num_available_moves_;
    }
}

void SoftmaxAgent::set_action_prob_() {
    //calculate the scaled and clipped exponential function for the estimated q value for each action
    std::transform(q_.begin(), q_.end(), exp_q_.begin(), scaled_clipped_exp);

    //calculate the sum of all scaled clipped exponential q values
    float sum_q = std::accumulate(exp_q_.begin(), exp_q_.end(), 0.0);

    //calculate the probability of each action as the ratio of scaled_clipped_exp(action(i))/sum(scaled_clipped_exponential)
    for (size_t i = 0; i < num_available_actions_; ++i) {
        if (propose_blk_type_) {
            //calculate block type index based on its location on q_table
            int blk_ratio_index = (int)i / num_available_moves_;
            action_prob_[i] = (exp_q_[i] / sum_q) * block_type_ratio_[blk_ratio_index];
        } else {
            action_prob_[i] = (exp_q_[i] / sum_q);
        }
    }

    // normalize all the action probabilities to guarantee the sum(all action probs) = 1
    float sum_prob = std::accumulate(action_prob_.begin(), action_prob_.end(), 0.0);
    if (propose_blk_type_) {
        std::transform(action_prob_.begin(), action_prob_.end(), action_prob_.begin(),
                       [sum_prob](float x) { return x * (1 / sum_prob); });
    } else {
        std::transform(action_prob_.begin(), action_prob_.end(), action_prob_.begin(),
                       [sum_prob, this](float x) { return x + ((1.0 - sum_prob) / this->num_available_moves_); });
    }

    // calculate the accumulative action probability of each action
    // e.g. if we have 5 actions with equal probability of 0.2, the cumm_action_prob will be {0.2,0.4,0.6,0.8,1.0}
    float accum = 0;
    for (size_t i = 0; i < num_available_actions_; ++i) {
        accum += action_prob_[i];
        cumm_action_prob_[i] = accum;
    }
}