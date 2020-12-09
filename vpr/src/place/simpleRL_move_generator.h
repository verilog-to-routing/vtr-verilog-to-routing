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
    virtual size_t propose_action() = 0;
    void process_outcome(double, e_reward_function);

  protected:
    float exp_alpha_ = -1;   //Step size for q_ updates (< 0 implies use incremental average)
    size_t k_;               //Number of arms
    std::vector<size_t> n_;  //Number of times each arm has been pulled
    std::vector<float> q_;   //Estimated value of each arm
    size_t last_action_ = 0; //The last action proposed
    /* Ratios of the average runtime to calculate each move type              */
    /* These ratios are useful for different reward functions                 *
     * The vector is calculated by averaging many runs on different circuits  */
    std::vector<double> time_elapsed_{1.0, 3.6, 5.4, 2.5, 2.1, 0.8, 2.2};

    FILE* f_ = nullptr;
};

class EpsilonGreedyAgent : public KArmedBanditAgent {
  public:
    EpsilonGreedyAgent(size_t k, float epsilon);
    ~EpsilonGreedyAgent();

    size_t propose_action() override; //Returns the next action the agent wants to

  public:
    void set_k(size_t k); //Sets the number of arms
    void set_epsilon(float epsilon);
    void set_epsilon_action_prob();
    void set_step(float gamma, int move_lim);

  private:
    float epsilon_ = 0.1; //How often to perform a non-greedy exploration action
    std::vector<float> cumm_epsilon_action_prob_;
};

class SoftmaxAgent : public KArmedBanditAgent {
  public:
    SoftmaxAgent(size_t k);
    ~SoftmaxAgent();

    //void process_outcome(double reward, std::string reward_fun) override; //Updates the agent based on the reward of the last proposed action
    size_t propose_action() override; //Returns the next action the agent wants to

  public:
    void set_k(size_t k); //Sets the number of arms
    void set_action_prob();
    void set_step(float gamma, int move_lim);

  private:
    std::vector<float> exp_q_; //Estimated value of each arm
    std::vector<float> action_prob_;
    std::vector<float> cumm_action_prob_;
};

class SimpleRLMoveGenerator : public MoveGenerator {
  private:
    std::vector<std::unique_ptr<MoveGenerator>> avail_moves;
    std::unique_ptr<KArmedBanditAgent> karmed_bandit_agent;

  public:
    SimpleRLMoveGenerator(std::unique_ptr<EpsilonGreedyAgent>& agent);
    SimpleRLMoveGenerator(std::unique_ptr<SoftmaxAgent>& agent);
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& move_type, MoveHelperData& move_helper, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities);
    void process_outcome(double reward, e_reward_function reward_fun);
};
#endif
