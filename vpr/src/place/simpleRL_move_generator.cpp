#include "simpleRL_move_generator.h"
#include "globals.h"
#include <algorithm>
#include <numeric>

#include "vtr_random.h"


//EpsilonGreedyAgent member functions
EpsilonGreedyAgent::EpsilonGreedyAgent(size_t k, float epsilon) {

    //f_ = vtr::fopen("agent_info.txt", "w");
    set_epsilon(epsilon);
    set_k(k);
    set_epsilon_action_prob();

    //string log_file = "agent_info.txt";
    //mode = "w";
    //f_ = fopen(log_file, mode);
}

EpsilonGreedyAgent::~EpsilonGreedyAgent() {
    if (f_) vtr::fclose(f_);
}


void EpsilonGreedyAgent::set_step(float gamma, int move_lim) {
	//VTR_LOG("Setting egreedy step: %g\n", exp_alpha_);
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
            //VTR_LOG("K-armed bandit alpha: %g\n", alpha);
            exp_alpha_ = alpha;
        }
}

void EpsilonGreedyAgent::process_outcome(double reward) {
    ++n_[last_action_];
    reward = reward / time_elapsed[last_action_];
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

#if 0
    //Update the cummulative q array
    for (size_t i = last_action_; i < k_; ++i) {
        cumm_q_[i] += delta_q;
    }
#endif

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


size_t EpsilonGreedyAgent::propose_action() {
    size_t action = 0;
    if (vtr::frand() < epsilon_) {
        float p = vtr::frand();
        auto itr = std::lower_bound(cumm_epsilon_action_prob_.begin(), cumm_epsilon_action_prob_.end(), p);
        action = itr - cumm_epsilon_action_prob_.begin();
    } else {
        //Greedy
        auto itr = std::max_element(q_.begin(), q_.end());
        VTR_ASSERT(itr != q_.end());
        action = itr - q_.begin();

#if 0
        //Proportional to relative q
        auto itr = std::min_element(q_.begin(), q_.end());
        std::vector<float> cumm_q(k_, 0);

        float accum = 0.;
        for (size_t i = 0; i < k_; i++) {
            accum += q_[i];
            cumm_q[i] = accum;
        }

        float range = q_[k_ - 1] - q_[0];

        float p = vtr::frand();
        float val = q_[0] + p*range;
        itr = std::lower_bound(cumm_q.begin(), cumm_q.end());
        action = itr - cumm_q.begin();
#endif
    }
    VTR_ASSERT(action < k_);

    last_action_ = action;
    return action;
}

void EpsilonGreedyAgent::set_epsilon(float epsilon) {
    //VTR_LOG("Setting egreedy epsilon: %g\n", epsilon);
    epsilon_ = epsilon;
}

void EpsilonGreedyAgent::set_k(size_t k) {
    k_ = k;
    q_ = std::vector<float>(k, 0.);
    n_ = std::vector<size_t>(k, 0);

    cumm_epsilon_action_prob_ = std::vector<float>(k, 1.0 / k);
#if 0
/*
    if (f_) {
        vtr::fclose(f_);
        f_ = nullptr;
    }
*/

#endif
    //f_ = vtr::fopen("egreedy.csv", "w");
    if(f_){
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
    std::vector<float> epsilon_prob(k_, 1.0/k_);


    float accum = 0;
    for (size_t i = 0; i < k_; ++i) {
        accum += epsilon_prob[i];
        cumm_epsilon_action_prob_[i] = accum;
    }
}



//SimpleRL class member functions
SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<EpsilonGreedyAgent>& agent){
//SimpleRLMoveGenerator::SimpleRLMoveGenerator(std::unique_ptr<SoftmaxAgent>& agent){
	std::unique_ptr<MoveGenerator> move_generator;
	move_generator = std::make_unique<UniformMoveGenerator>();
	avail_moves.push_back(std::move(move_generator));

	std::unique_ptr<MoveGenerator> move_generator2;
	move_generator2 = std::make_unique<MedianMoveGenerator>();
	avail_moves.push_back(std::move(move_generator2));

	std::unique_ptr<MoveGenerator> move_generator3;
	move_generator3 = std::make_unique<WeightedMedianMoveGenerator>();
	avail_moves.push_back(std::move(move_generator3));

	std::unique_ptr<MoveGenerator> move_generator4;
	move_generator4 = std::make_unique<WeightedCentroidMoveGenerator>();
	avail_moves.push_back(std::move(move_generator4));

	std::unique_ptr<MoveGenerator> move_generator5;
	move_generator5 = std::make_unique<FeasibleRegionMoveGenerator>();
	avail_moves.push_back(std::move(move_generator5));
	
	std::unique_ptr<MoveGenerator> move_generator6;
	move_generator6 = std::make_unique<CriticalUniformMoveGenerator>();
	avail_moves.push_back(std::move(move_generator6));

	std::unique_ptr<MoveGenerator> move_generator7;
	move_generator7 = std::make_unique<CentroidMoveGenerator>();
	avail_moves.push_back(std::move(move_generator7));
    
    karmed_bandit_agent = std::move(agent);
}


e_create_move SimpleRLMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim
	, std::vector<int>& X_coord, std::vector<int>& Y_coord, std::vector<int>& num_moves, int& type, int high_fanout_net) {

	type = karmed_bandit_agent->propose_action();
	++num_moves[type];
	return avail_moves[type]->propose_move(blocks_affected, rlim, X_coord, Y_coord, num_moves, type, high_fanout_net);
}

void SimpleRLMoveGenerator::process_outcome(double reward){
	karmed_bandit_agent->process_outcome(reward);
}
/*                                  *
 *                                  *
 *  Softmax agent implementation    *
 *                                  *
 *                                  */

SoftmaxAgent::SoftmaxAgent(size_t k){
    set_k(k);
    set_action_prob();

    //f_ = vtr::fopen("agent_info.txt", "w");
}


SoftmaxAgent::~SoftmaxAgent() {
    if (f_) vtr::fclose(f_);
}

void SoftmaxAgent::process_outcome(double reward){
    
    ++n_[last_action_];
    reward = reward / time_elapsed[last_action_];
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

#if 0
    //Update the cummulative q array
    for (size_t i = last_action_; i < k_; ++i) {
        cumm_q_[i] += delta_q;
    }
#endif

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

size_t SoftmaxAgent::propose_action(){
    set_action_prob();
    size_t action = 0;
    float p = vtr::frand();
    //VTR_LOG("#####%f,%f,%f,%f,%f,%f,%f\n",action_prob_[0],action_prob_[1],action_prob_[2],action_prob_[3],action_prob_[4],action_prob_[5],action_prob_[6]);
    //VTR_LOG("@@@@@%f,%f,%f,%f,%f,%f,%f\n",cumm_action_prob_[0],cumm_action_prob_[1],cumm_action_prob_[2],cumm_action_prob_[3],cumm_action_prob_[4],cumm_action_prob_[5],cumm_action_prob_[6]);
    auto itr = std::lower_bound(cumm_action_prob_.begin(), cumm_action_prob_.end(), p);
    action = itr - cumm_action_prob_.begin();
    VTR_ASSERT(action < k_);

    last_action_ = action;
    return action;
}

void SoftmaxAgent::set_k(size_t k) {
    k_ = k;
    q_ = std::vector<float>(k, 0.);
    n_ = std::vector<size_t>(k, 0);
    action_prob_ = std::vector<float>(k,0.);

    cumm_action_prob_ = std::vector<float>(k);
    if(f_){
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

void SoftmaxAgent::set_action_prob() {
    float sum_q = accumulate(q_.begin(),q_.end(),0.0);
    if(sum_q == 0.0){
        std::fill(action_prob_.begin(),action_prob_.end(),1.0/k_);        
    }
    else{
        for(size_t i=0; i<k_; ++i){
            action_prob_[i] = std::max(std::min(q_[i]/sum_q, float(0.9)),float(0.02));
        }
    }

    float sum_prob = std::accumulate(action_prob_.begin(), action_prob_.end(),0.0);
    std::transform(action_prob_.begin(), action_prob_.end(), action_prob_.begin(),
          bind2nd(std::plus<float>(),(1.0-sum_prob)/k_ ));  
    float accum = 0;
    for (size_t i = 0; i < k_; ++i) {
        accum += action_prob_[i];
        cumm_action_prob_[i] = accum;
    }
}

void SoftmaxAgent::set_step(float gamma, int move_lim){
	//VTR_LOG("Setting softmax step: %g\n", exp_alpha_);
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
            //VTR_LOG("K-armed bandit alpha: %g\n", alpha);
            exp_alpha_ = alpha;
        }
}


