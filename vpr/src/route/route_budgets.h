/*Defines the route_budgets class that contains the minimum, maximum,
 * target, upper bound, and lower bound budgets. These information are
 * used by the router to optimize for hold time. */
#ifndef ROUTE_BUDGETS_H
#define ROUTE_BUDGETS_H

#include <iostream>
#include <vector>
#include <queue>
#include "RoutingDelayCalculator.h"

enum analysis_type {
    SETUP,
    HOLD
};

enum slack_allocated_type {
    POSITIVE,
    NEGATIVE,
    BOTH
};

#define UNINITIALIZED_PATH_DELAY -2.

class route_budgets {
  public:
    route_budgets();

    route_budgets(std::vector<std::vector<float>> net_delay);

    virtual ~route_budgets();

    /*getter functions*/
    float get_delay_target(ClusterNetId net_id, int ipin);
    float get_min_delay_budget(ClusterNetId net_id, int ipin);
    float get_max_delay_budget(ClusterNetId net_id, int ipin);
    float get_crit_short_path(ClusterNetId net_id, int ipin);
    bool if_set() const;

    /*main loader function*/
    void load_route_budgets(ClbNetPinsMatrix<float>& net_delay,
                            std::shared_ptr<SetupTimingInfo> timing_info,
                            const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                            const t_router_opts& router_opts);

    /*debugging tools*/
    void print_route_budget(std::string filename, ClbNetPinsMatrix<float>& net_delay);

    /*lower budgets during congestion*/
    void update_congestion_times(ClusterNetId net_id);
    bool increase_min_budgets_if_struggling(float delay_decrement, std::shared_ptr<SetupHoldTimingInfo> timing_info, float worst_neg_slack, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);
    void increase_short_crit(ClusterNetId net_id, float delay_decs);
    void not_congested_this_iteration(ClusterNetId net_id);

    bool get_should_reroute(ClusterNetId net_id);
    void set_should_reroute(ClusterNetId net_id, bool value);
    int get_hold_fac(ClusterNetId net_id);
    void set_hold_fac(ClusterNetId net_id, int value);

  private:
    /*For allocating and freeing memory*/
    void free_budgets();
    void alloc_budget_memory();
    void load_initial_budgets();

    /*different ways to set route budgets*/
    void allocate_slack_using_delays_and_criticalities(ClbNetPinsMatrix<float>& net_delay,
                                                       std::shared_ptr<SetupTimingInfo> timing_info,
                                                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                       const t_router_opts& router_opts);
    void allocate_slack_using_weights(ClbNetPinsMatrix<float>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, bool negative_hold_slack);
    /*Sometimes want to allocate only positive or negative slack.
     * By default, allocate both*/
    float minimax_PERT(std::shared_ptr<SetupHoldTimingInfo> orig_timing_info,
                       std::shared_ptr<SetupHoldTimingInfo> timing_info,
                       ClbNetPinsMatrix<float>& temp_budgets,
                       ClbNetPinsMatrix<float>& net_delay,
                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                       analysis_type analysis_type,
                       bool keep_in_bounds,
                       slack_allocated_type slack_type = BOTH);

    void process_negative_slack_using_minimax(ClbNetPinsMatrix<float>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);

    /*Perform static timing analysis*/
    std::shared_ptr<SetupHoldTimingInfo> perform_sta(ClbNetPinsMatrix<float>& temp_budgets);

    /*checks*/
    void keep_budget_in_bounds(ClbNetPinsMatrix<float>& temp_budgets);
    void keep_budget_in_bounds(ClbNetPinsMatrix<float>& temp_budgets, ClusterNetId net_id, ClusterPinId ipin);
    void keep_min_below_max_budget();
    void check_if_budgets_in_bounds();
    void check_if_budgets_in_bounds(ClusterNetId net_id, ClusterPinId pin_id);
    void keep_budget_above_value(ClbNetPinsMatrix<float>& temp_budgets, float bottom_range);

    /*helper functions*/
    float calculate_clb_pin_slack(ClusterNetId net_id,
                                  int ipin,
                                  std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                  const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                  analysis_type type,
                                  AtomPinId& atom_pin);

    float get_total_path_delay(std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> timing_analyzer,
                               analysis_type analysis_type,
                               ClusterNetId net_id,
                               int ipin,
                               AtomPinId& atom_pin);
    void set_min_max_budgets_equal();
    std::shared_ptr<RoutingDelayCalculator> get_routing_calc(ClbNetPinsMatrix<float>& net_delay);
    void calculate_delay_targets();
    void calculate_delay_targets(ClusterNetId net_id, ClusterPinId pin_id);
    tatum::EdgeId get_edge_from_nets(ClusterNetId net_id, int ipin);

    /*debugging tools*/
    void print_temporary_budgets_to_file(ClbNetPinsMatrix<float>& temp_budgets);

    /*Budget variables*/
    ClbNetPinsMatrix<float> delay_min_budget;  //[0..num_nets][0..clb_net[inet].pins]
    ClbNetPinsMatrix<float> delay_max_budget;  //[0..num_nets][0..clb_net[inet].pins]
    ClbNetPinsMatrix<float> delay_target;      //[0..num_nets][0..clb_net[inet].pins]
    ClbNetPinsMatrix<float> delay_lower_bound; //[0..num_nets][0..clb_net[inet].pins]
    ClbNetPinsMatrix<float> delay_upper_bound; //[0..num_nets][0..clb_net[inet].pins]
    ClbNetPinsMatrix<float> short_path_crit;   //[0..num_nets][0..clb_net[inet].pins]

    ClbNetPinsMatrix<float> total_path_delays_hold;
    ClbNetPinsMatrix<float> total_path_delays_setup;

    /*used to keep count the number of continuous time this node was congested*/
    vtr::vector<ClusterNetId, int> num_times_congested; //[0..num_nets]
    std::queue<float> negative_hold_slacks;

    /*budgets only valid when loaded*/
    bool set;

    /*flag to reroute each net for hold violation*/
    std::map<ClusterNetId, bool> should_reroute_for_hold;
    std::map<ClusterNetId, int> hold_fac;
};

#endif /* ROUTE_BUDGETS_H */
