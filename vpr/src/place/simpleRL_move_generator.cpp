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

template<class T, class>
SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<T>& agent) {
    avail_moves.resize((int)e_move_type::NUMBER_OF_AUTO_MOVES);

    avail_moves[(int)e_move_type::UNIFORM] = std::make_unique<UniformMoveGenerator>();
    avail_moves[(int)e_move_type::MEDIAN] = std::make_unique<MedianMoveGenerator>();
    avail_moves[(int)e_move_type::CENTROID] = std::make_unique<CentroidMoveGenerator>();
    avail_moves[(int)e_move_type::W_CENTROID] = std::make_unique<WeightedCentroidMoveGenerator>();
    avail_moves[(int)e_move_type::W_MEDIAN] = std::make_unique<WeightedMedianMoveGenerator>();
    avail_moves[(int)e_move_type::CRIT_UNIFORM] = std::make_unique<CriticalUniformMoveGenerator>();
    avail_moves[(int)e_move_type::FEASIBLE_REGION] = std::make_unique<FeasibleRegionMoveGenerator>();

    karmed_bandit_agent = std::move(agent);
}

e_create_move SimpleRLMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& move_type, t_logical_block_type& blk_type, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    auto propose_action_out = karmed_bandit_agent->propose_action();
    move_type = propose_action_out.move_type;
    blk_type = propose_action_out.blk_type; // can be empty to allow agent to only choose move type (pick a block randomly)
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
KArmedBanditAgent::KArmedBanditAgent(size_t num_moves, size_t num_types, bool propose_blk_type)
    : num_available_moves_(num_moves)
    , num_available_types_(num_types)
    , propose_blk_type_(propose_blk_type)
{
}

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

    for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
        fprintf(agent_info_file_, "%g,", q_[i]);
    }

    for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
        fprintf(agent_info_file_, "%zu,", num_action_chosen_[i]);
    }
    fprintf(agent_info_file_, "\n");
    fflush(agent_info_file_);
}

/*                                  *
 *                                  *
 *  E-greedy agent implementation   *
 *                                  *
 *                                  */
EpsilonGreedyAgent::EpsilonGreedyAgent(size_t num_moves, float epsilon)
    : EpsilonGreedyAgent(num_moves, 1, epsilon)
{
}

EpsilonGreedyAgent::EpsilonGreedyAgent(size_t num_moves, size_t num_types, float epsilon)
    : KArmedBanditAgent(num_moves, num_types, num_types > 1)
{
    set_epsilon(epsilon);
    init_q_scores();
}

EpsilonGreedyAgent::~EpsilonGreedyAgent() {
    if (agent_info_file_) vtr::fclose(agent_info_file_);
}

void EpsilonGreedyAgent::init_q_scores() {
    q_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    num_action_chosen_ = std::vector<size_t>(num_available_moves_ * num_available_types_, 0);
    cumm_epsilon_action_prob_ = std::vector<float>(num_available_moves_ * num_available_types_, 1.0 / (num_available_moves_ * num_available_types_));

    //agent_info_file_ = vtr::fopen("agent_info.txt", "w");
    //write agent internal q-table and actions into file for debugging purposes
    if (agent_info_file_) {
        //we haven't performed any moves yet, hence last_aciton and reward are 0
        write_agent_info(0, 0);
    }

    set_epsilon_action_prob();
}

void EpsilonGreedyAgent::set_step(float gamma, int move_lim) {
    VTR_LOG("Setting egreedy step: %g\n", exp_alpha_);
    if (gamma < 0) {
        exp_alpha_ = -1; //Use sample average
    } else {
        //
        // For an exponentially weighted average the fraction of total weight applied
        // to moves which occurred > K moves ago is:
        //
        //      gamma = (1 - alpha)^K
        //
        // If we treat K as the number of moves per temperature (move_lim) then gamma
        // is the fraction of weight applied to moves which occurred > move_lim moves ago,
        // and given a target gamma we can explicitly calculate the alpha step-size
        // required by the agent:
        //
        //     alpha = 1 - e^(log(gamma) / K)
        //
        float alpha = 1 - std::exp(std::log(gamma) / move_lim);
        exp_alpha_ = alpha;
    }
}

t_propose_action EpsilonGreedyAgent::propose_action() {
    size_t move_type;
    t_logical_block_type blk_type;

    if (vtr::frand() < epsilon_) {
        /* Explore
         * With probability epsilon, choose randomly amongst all move types */
        float p = vtr::frand();
        auto itr = std::lower_bound(cumm_epsilon_action_prob_.begin(), cumm_epsilon_action_prob_.end(), p);
        auto action_type_q_pos = itr - cumm_epsilon_action_prob_.begin();
        move_type = (action_type_q_pos) % num_available_moves_;
        if (propose_blk_type_) { //calculate block type index only if agent is supposed to propose both move and block type
            blk_type.index = action_type_q_pos / num_available_moves_;
        }

    } else {
        /* Greedy (Exploit)
         * For probability 1-epsilon, choose the greedy move_type */
        auto itr = std::max_element(q_.begin(), q_.end());
        VTR_ASSERT(itr != q_.end());
        auto action_type_q_pos = itr - q_.begin();
        move_type = action_type_q_pos % num_available_moves_;
        if (propose_blk_type_) { //calculate block type index only if agent is supposed to propose both move and block type
            blk_type.index = action_type_q_pos / num_available_moves_;
        }
    }

    //Check the move type to be a valid move
    VTR_ASSERT(move_type < num_available_moves_);
    //Check the block type index to be valid type if the agent is supposed to propose block type
    VTR_ASSERT((size_t)blk_type.index < num_available_types_ || !propose_blk_type_);

    //Mark the q_table location that agent used to update its value after processing the move outcome
    last_action_ = (!propose_blk_type_) ? move_type : move_type + (blk_type.index * num_available_moves_);

    t_propose_action propose_action;
    propose_action.move_type = (e_move_type)move_type;
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
SoftmaxAgent::SoftmaxAgent(size_t num_moves)
    : SoftmaxAgent(num_moves, 1)
{
}

SoftmaxAgent::SoftmaxAgent(size_t num_moves, size_t num_types)
    : KArmedBanditAgent(num_moves, num_types, num_types > 1) {
    init_q_scores();
}

SoftmaxAgent::~SoftmaxAgent() {
    if (agent_info_file_) vtr::fclose(agent_info_file_);

    std::cout << "Softmax time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time_).count() << std::endl;

    vtr::printf_info("Softmax took %.2f miliseconds\n",
                     std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time_).count());
}

void SoftmaxAgent::init_q_scores() {
    q_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    exp_q_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    num_action_chosen_ = std::vector<size_t>(num_available_moves_ * num_available_types_, 0);
    action_prob_ = std::vector<float>(num_available_moves_ * num_available_types_, 0.);
    block_type_ratio_ = std::vector<float>(num_available_types_, 0.);
    cumm_action_prob_ = std::vector<float>(num_available_moves_ * num_available_types_);

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
        set_block_ratio();
    }
    set_action_prob();
    elapsed_time_ = std::chrono::duration<double, std::nano>::zero();
}

t_propose_action SoftmaxAgent::propose_action() {
    auto start_time = std::chrono::high_resolution_clock::now();
    set_action_prob();
    // Get the current time after executing the function
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the duration in milliseconds
    std::chrono::duration<double, std::nano> duration = end_time - start_time;

    elapsed_time_ += duration;

    size_t move_type;
    t_logical_block_type blk_type;

    float p = vtr::frand();
    auto itr = std::lower_bound(cumm_action_prob_.begin(), cumm_action_prob_.end(), p);
    auto action_type_q_pos = itr - cumm_action_prob_.begin();
    move_type = (action_type_q_pos) % num_available_moves_;
    if (propose_blk_type_) { //calculate block type index only if agent is supposed to propose both move and block type
        blk_type.index = action_type_q_pos / num_available_moves_;
    }

    //To take care that the last element in cumm_action_prob_ might be less than 1 by a small value
    if ((size_t)action_type_q_pos == num_available_moves_ * num_available_types_) {
        move_type = num_available_moves_ - 1;
        if (propose_blk_type_) { //calculate block type index only if agent is supposed to propose both move and block type
            blk_type.index = num_available_types_ - 1;
        }
    }

    //Check the move type to be a valid move
    VTR_ASSERT(move_type < num_available_moves_);
    //Check the block type index to be valid type if the agent is supposed to propose block type
    VTR_ASSERT((size_t)blk_type.index < num_available_types_ || !propose_blk_type_);

    //Mark the q_table location that agent used to update its value after processing the move outcome
    last_action_ = (!propose_blk_type_) ? move_type : move_type + (blk_type.index * num_available_moves_);

    t_propose_action propose_action;
    propose_action.move_type = (e_move_type)move_type;
    propose_action.blk_type = blk_type;

    return propose_action;
}

void SoftmaxAgent::set_block_ratio() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    int num_total_blocks = cluster_ctx.clb_nlist.blocks().size();

    /* Calculate ratio of each block as : (# blocks of each type / total blocks).
     * Each block type can have "num_available_moves_" different moves. Hence,
     * the ratio will be divided by num_available_moves_ at the end.
     */
    for (size_t itype = 0; itype < num_available_types_; itype++) {
        t_logical_block_type blk_type;
        blk_type.index = convert_agent_to_phys_blk_type(itype);
        auto num_blocks = cluster_ctx.clb_nlist.blocks_per_type(blk_type).size();
        block_type_ratio_[itype] = (float)num_blocks / num_total_blocks;
        block_type_ratio_[itype] /= num_available_moves_;
    }
}

void SoftmaxAgent::set_action_prob() {
    //calculate the scaled and clipped exponential function for the estimated q value for each action
    std::transform(q_.begin(), q_.end(), exp_q_.begin(), scaled_clipped_exp);

    //calculate the sum of all scaled clipped exponential q values
    float sum_q = accumulate(exp_q_.begin(), exp_q_.end(), 0.0);

    //calculate the probability of each action as the ratio of scaled_clipped_exp(action(i))/sum(scaled_clipped_exponential)
    for (size_t i = 0; i < num_available_moves_ * num_available_types_; ++i) {
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
