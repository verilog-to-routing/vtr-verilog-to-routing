#ifndef VPR_SIMPLERL_MOVE_GEN_H
#define VPR_SIMPLERL_MOVE_GEN_H
#include "move_generator.h"
#include "median_move_generator.h"
#include "weighted_median_move_generator.h"
#include "feasible_region_move_generator.h"
#include "weighted_centroid_move_generator.h"
#include "uniform_move_generator.h"
#include "critical_uniform_move_generator.h"
#include "centroid_move_generator.h"

class KArmedBanditAgent {
  public:
    virtual ~KArmedBanditAgent() {}
    virtual void set_k(size_t k) = 0;
    virtual void process_outcome(double ) = 0;
    virtual size_t propose_action() = 0;
#if 0
    virtual void debug() = 0;
#endif
};

class EpsilonGreedyAgent : public KArmedBanditAgent {
  public:
    EpsilonGreedyAgent(size_t k, float epsilon);
    ~EpsilonGreedyAgent();

    void process_outcome(double reward) override; //Updates the agent based on the reward of the last proposed action
    size_t propose_action() override;            //Returns the next action the agent wants to


  public:
    void set_k(size_t k); //Sets the number of arms
    void set_epsilon(float epsilon);
    void set_epsilon_action_prob();
    void set_step(float gamma, int move_lim);

    //void debug() override;

  private:
    size_t k_;             //Number of arms
    float epsilon_ = 0.1;  //How often to perform a non-greedy exploration action
    float exp_alpha_ = -1; //Step size for q_ updates (< 0 implies use incremental average)

    std::vector<size_t> n_; //Number of times each arm has been pulled
    std::vector<float> q_;  //Estimated value of each arm
    std::vector<float> q_cumm_;

    std::vector<float> cumm_epsilon_action_prob_;
    size_t last_action_ = 0; //The last action proposed
    //std::vector<double> time_elapsed {1.0,4.3,5.7,3.3};
    //std::vector<double> time_elapsed {1.0,3.87,6.4,2.8,2.29};
    std::vector<double> time_elapsed {1.0,3.6,5.4,2.5,2.1,0.8};
    //std::vector<double> time_elapsed {1.0, 4.11, 6.67, 3.22, 1.88, 0.81};
    //std::vector<int> time_elapsed {7,30,40,23};
    FILE* f_ = nullptr;
};

class SoftmaxAgent : public KArmedBanditAgent {
  public:
    SoftmaxAgent(size_t k);
    ~SoftmaxAgent();

    void process_outcome(double reward) override; //Updates the agent based on the reward of the last proposed action
    size_t propose_action() override;            //Returns the next action the agent wants to


  public:
    void set_k(size_t k); //Sets the number of arms
    void set_action_prob();
    void set_step(float gamma, int move_lim);

    //void debug() override;

  private:
    size_t k_;             //Number of arms
    float exp_alpha_ = -1; //Step size for q_ updates (< 0 implies use incremental average)

    std::vector<size_t> n_; //Number of times each arm has been pulled
    std::vector<float> q_;  //Estimated value of each arm

    std::vector<float> cumm_action_prob_;
    size_t last_action_ = 0; //The last action proposed
    //std::vector<double> time_elapsed {1.0,4.3,5.7,3.3};
    //std::vector<double> time_elapsed {1.0,3.87,6.4,2.8,2.29};
    std::vector<double> time_elapsed {1.0,3.6,5.4,2.5,2.1,0.8};
    //std::vector<double> time_elapsed {1.0, 4.11, 6.67, 3.22, 1.88, 0.81};
    //std::vector<int> time_elapsed {7,30,40,23};
    FILE* f_ = nullptr;
};

class SimpleRLMoveGenerator : public MoveGenerator {
private:
	std::vector<std::unique_ptr<MoveGenerator>> avail_moves;
    //std::unique_ptr<EpsilonGreedyAgent> karmed_bandit_agent;
    std::unique_ptr<SoftmaxAgent> karmed_bandit_agent;
public:
	//SimpleRLMoveGenerator(std::unique_ptr<EpsilonGreedyAgent>& agent);
	SimpleRLMoveGenerator(std::unique_ptr<SoftmaxAgent>& agent);
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim,
        std::vector<int>& X_coord, std::vector<int>& Y_coord, std::vector<int>& num_moves,
        int& type, int high_fanout_net);
    void process_outcome(double reward);
};
#endif
