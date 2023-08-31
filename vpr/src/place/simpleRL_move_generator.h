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

/**
 * @brief KArmedBanditAgent is the base class for RL agents that target the k-armed bandit problems
 */
class KArmedBanditAgent {
  public:
    KArmedBanditAgent(size_t num_moves, e_agent_space agent_space);
    virtual ~KArmedBanditAgent() = default;

    /**
     * @brief Choose a move type to perform and a block type that move should be performed with based on Q-table
     *
     * @return A move type and a block type as a "t_propose_action" struct
     * If the agent is set to only propose move type, then block type index in the struct will be set to -1
     */
    virtual t_propose_action propose_action() = 0;

    /**
     * @brief Update the agent Q-table based on the reward received by the SA algorithm
     *
     *   @param reward A double value calculated in "place.cpp" file showing how placement cost was affected by the prior action taken
     *   @param reward_func The reward function used by the agent, detail explanation can be found on "directed_moves_util.h" file
     */
    void process_outcome(double, e_reward_function);

    /**
     * @brief write all agent internal information (Q-table, reward for each performed action, ...) to a file (agent_info_file_)
     *
     *   @param last_action Last action performed by the RL-agent
     *   @param reward A double value calculated in "place.cpp" file showing how placement cost was affected by the prior action taken
     */
    void write_agent_info(int last_action, double reward);

    /**
     * @brief Set step size for q-table updates
     *
     *   @param gamma Controls how quickly the agent's memory decays, can be specified by the command-line option "--place_agent_gamma"
     *   Gamma default value is 0.05.
     *   @param move_lim Number of moves per temperature
     */
    void set_step(float gamma, int move_lim);

  protected:
    /**
     * @brief Converts an action index to a move type.
     *
     *   @param action_idx Specifies which action is selected by the agent.
     *
     * @return The move type associated with the selected action.
     */
    inline e_move_type action_to_move_type_(size_t action_idx);

    /**
     * @brief Converts an action index to a logical block type index.
     *
     *   @param action_idx Specifies which action is selected by the agent.
     *
     * @return The logical block type index associated with the selected action.
     */
    inline int action_to_blk_type_(size_t action_idx);

    /**
     * @brief Converts an agent block type index to a logical block type index.
     * Q-table only contains entries for block types that exist in the netlist.
     * Agent block type index ranges from 0 to num_types_in_netlist-1. The agent
     * chooses an agent block type and a move type. This function is used to
     * convert the agent block type index (only usable within this class) to
     * a logical block type index (usable by different move generators).
     *
     *   @param idx Specifies the index by which this calls refers to a logical block type index.
     *
     * @return The referred logical block type index.
     */
    inline int agent_to_phy_blk_type(int idx);

  protected:
    float exp_alpha_ = -1;                  //Step size for q_ updates (< 0 implies use incremental average)
    size_t num_available_moves_;            //Number of move types that agent can choose from to perform
    size_t num_available_types_;            //Number of block types that exist in the netlest. Agent may not choose the block type.
    size_t num_available_actions_;          //Total number of available actions
    bool propose_blk_type_ = false;         //Check if agent should propose both move and block type or only move type
    std::vector<size_t> num_action_chosen_; //Number of times each arm has been pulled (n)
    std::vector<float> q_;                  //Estimated value of each arm (Q)
    size_t last_action_;                    //type of the last action (move type) proposed
    /* Ratios of the average runtime to calculate each move type              */
    /* These ratios are useful for different reward functions                 *
     * The vector is calculated by averaging many runs on different circuits  */
    std::vector<double> time_elapsed_{1.0, 3.6, 5.4, 2.5, 2.1, 0.8, 2.2};

    FILE* agent_info_file_ = nullptr;

  private:
    /**
     * @brief Iterates over all logical block types and check whether they exist in the
     * netlist. Then, returns the logical block type indices found in the netlist.
     *
     * @return A vector containing all logical block type indices that exist in the netlist.
     */
    static std::vector<int> get_available_logical_blk_types_();

  private:
    std::vector<int> action_logical_blk_type_;
};

/**
 * @brief Epsilon-greedy agent implementation to address K-armed bandit problems
 *
 * In this class, the RL agent addresses the exploration-exploitation dilemma using epsilon- greedy
 * where the agent chooses the best action (the greedy) most of the time. For a small fraction of the time 
 * (epsilon), it randomly chooses any action to explore the solution space.
 */
class EpsilonGreedyAgent : public KArmedBanditAgent {
  public:
    EpsilonGreedyAgent(size_t num_moves, e_agent_space agent_space, float epsilon);
    ~EpsilonGreedyAgent() override;

    t_propose_action propose_action() override; //Returns the type of the next action as well as the block type the agent wishes to perform

  public:
    /**
     * @brief Set the user-specified epsilon for the E-greedy agent
     *
     *   @param epsilon Epsilon value for the agent, can be specified by the command-line option "--place_agent_epsilon"
     *   Epsilon default value is 0.3.
     */
    void set_epsilon(float epsilon);

    /**
     * @brief Set equal action probability to all available actions.
     */
    void set_epsilon_action_prob();

  private:
    /**
     * @brief Initialize agent's Q-table and internal variable to zero (RL-agent learns everything throughout the placement run and has no prior knowledge)
     */
    void init_q_scores_();

  private:
    float epsilon_ = 0.1;                         //How often to perform a non-greedy exploration action
    std::vector<float> cumm_epsilon_action_prob_; //The accumulative probability of choosing each action
};

/**
 * @brief Softmax agent implementation to address K-armed bandit problems
 *
 * In this class, the RL agent addresses the exploration-exploitation dilemma using Softmax
 * where the probability of choosing each action proportional to the estimated Q value of
 * this action. 
 */
class SoftmaxAgent : public KArmedBanditAgent {
  public:
    SoftmaxAgent(size_t num_moves, e_agent_space agent_space);
    ~SoftmaxAgent() override;

    //void process_outcome(double reward, std::string reward_fun) override; //Updates the agent based on the reward of the last proposed action
    t_propose_action propose_action() override; //Returns the type of the next action as well as the block type the agent wishes to perform

  private:
    /**
     * @brief Initialize agent's Q-table and internal variable to zero (RL-agent learns everything throughout the placement run and has no prior knowledge)
     */
    void init_q_scores_();

    /**
     * @brief Calculate the fraction of total netlist blocks for each agent block type and will be used by the "set_action_prob" function.
     */
    void set_block_ratio_();

    /**
     * @brief Set action probability for all available actions.
     * If agent only proposes move type, the action probabilities would be equal for all move types at the beginning.
     * If agent proposes both move and block type, the action_prob for each action would be based on its block type count in the netlist.
     */
    void set_action_prob_();

  private:
    std::vector<float> exp_q_;            //The clipped and scaled exponential of the estimated Q value for each action
    std::vector<float> action_prob_;      //The probability of choosing each action
    std::vector<float> cumm_action_prob_; //The accumulative probability of choosing each action
    std::vector<float> block_type_ratio_; //Fraction of total netlist blocks for each block type (size: [0..agent_blk_type-1])
};

/**
 * @brief This class represents the move generator that uses simple RL agent
 * 
 * It is a derived class from MoveGenerator that utilizes a simple RL agent to
 * dynamically select the optimal probability of each action (move type)
 * 
 */
class SimpleRLMoveGenerator : public MoveGenerator {
  private:
    std::vector<std::unique_ptr<MoveGenerator>> avail_moves; // list of pointers to the available move generators (the different move types)
    std::unique_ptr<KArmedBanditAgent> karmed_bandit_agent;  // a pointer to the specific agent used (e.g. Softmax)

  public:
    // constructor using a pointer to the agent used
    // the constructor
    /**
     * @brief Constructs an RL move generator using the passed agent
     *
     *   @param agent std::unique_ptr to the agent. Only EpsilonGreedyAgent and SoftmaxAgent types are accepted
     *   by the constructor. If other types are passed, a compile error would be thrown.
     */
    template<class T,
             class = typename std::enable_if<std::is_same<T, EpsilonGreedyAgent>::value || std::is_same<T, SoftmaxAgent>::value>::type>
    explicit SimpleRLMoveGenerator(std::unique_ptr<T>& agent);

    // Updates affected_blocks with the proposed move, while respecting the current rlim
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& proposed_action, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) override;

    // Receives feedback about the outcome of the previously proposed move
    void process_outcome(double reward, e_reward_function reward_fun) override;
};

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
#endif
