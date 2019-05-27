/*Defines the route_budgets class that contains the minimum, maximum,
 * target, upper bound, and lower bound budgets. These information are
 * used by the router to optimize for hold time. */
#ifndef ROUTE_BUDGETS_H
#define ROUTE_BUDGETS_H

#include <iostream>
#include <vector>
#include "vtr_memory.h"
#include "RoutingDelayCalculator.h"

using namespace std;

enum analysis_type {
    SETUP,
    HOLD
};

enum slack_allocated_type {
    POSITIVE,
    NEGATIVE,
    BOTH
};

class route_budgets {
  public:
    route_budgets();

    route_budgets(vector<vector<float>> net_delay);

    virtual ~route_budgets();

    /*getter functions*/
    float get_delay_target(ClusterNetId net_id, int ipin);
    float get_min_delay_budget(ClusterNetId net_id, int ipin);
    float get_max_delay_budget(ClusterNetId net_id, int ipin);
    float get_crit_short_path(ClusterNetId net_id, int ipin);
    bool if_set() const;

    /*main loader function*/
    void load_route_budgets(vtr::vector<ClusterNetId, float*>& net_delay,
                            std::shared_ptr<SetupTimingInfo> timing_info,
                            const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                            t_router_opts router_opts);

    /*debugging tools*/
    void print_route_budget();

    /*lower budgets during congestion*/
    void update_congestion_times(ClusterNetId net_id);
    void lower_budgets(float delay_decrement);
    void not_congested_this_iteration(ClusterNetId net_id);

  private:
    /*For allocating and freeing memory*/
    void free_budgets();
    void alloc_budget_memory();
    void load_initial_budgets();

    /*different ways to set route budgets*/
    void allocate_slack_using_delays_and_criticalities(vtr::vector<ClusterNetId, float*>& net_delay,
                                                       std::shared_ptr<SetupTimingInfo> timing_info,
                                                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                       t_router_opts router_opts);
    void allocate_slack_using_weights(vtr::vector<ClusterNetId, float*>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);
    /*Sometimes want to allocate only positive or negative slack.
     * By default, allocate both*/
    float minimax_PERT(std::shared_ptr<SetupHoldTimingInfo> timing_info, vtr::vector<ClusterNetId, float*>& temp_budgets, vtr::vector<ClusterNetId, float*>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, analysis_type analysis_type, bool keep_in_bounds, slack_allocated_type slack_type = BOTH);
    void process_negative_slack_using_minimax(vtr::vector<ClusterNetId, float*>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);

    /*Perform static timing analysis*/
    std::shared_ptr<SetupHoldTimingInfo> perform_sta(vtr::vector<ClusterNetId, float*>& temp_budgets);

    /*checks*/
    void keep_budget_in_bounds(vtr::vector<ClusterNetId, float*>& temp_budgets);
    void keep_budget_in_bounds(vtr::vector<ClusterNetId, float*>& temp_budgets, ClusterNetId net_id, ClusterPinId ipin);
    void keep_min_below_max_budget();
    void check_if_budgets_in_bounds();
    void check_if_budgets_in_bounds(ClusterNetId net_id, ClusterPinId pin_id);
    void keep_budget_above_value(vtr::vector<ClusterNetId, float*>& temp_budgets, float bottom_range);

    /*helper functions*/
    float calculate_clb_pin_slack(ClusterNetId net_id, int ipin, std::shared_ptr<SetupHoldTimingInfo> timing_info, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, analysis_type type, AtomPinId& atom_pin);
    float get_total_path_delay(std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> timing_analyzer,
                               analysis_type analysis_type,
                               tatum::NodeId timing_node);
    void set_min_max_budgets_equal();
    std::shared_ptr<RoutingDelayCalculator> get_routing_calc(vtr::vector<ClusterNetId, float*>& net_delay);
    void calculate_delay_targets();
    void calculate_delay_targets(ClusterNetId net_id, ClusterPinId pin_id);
    tatum::EdgeId get_edge_from_nets(ClusterNetId net_id, int ipin);

    /*debugging tools*/
    void print_temporary_budgets_to_file(vtr::vector<ClusterNetId, float*>& temp_budgets);

    /*Budget variables*/
    vtr::vector<ClusterNetId, float*> delay_min_budget;  //[0..num_nets][0..clb_net[inet].pins]
    vtr::vector<ClusterNetId, float*> delay_max_budget;  //[0..num_nets][0..clb_net[inet].pins]
    vtr::vector<ClusterNetId, float*> delay_target;      //[0..num_nets][0..clb_net[inet].pins]
    vtr::vector<ClusterNetId, float*> delay_lower_bound; //[0..num_nets][0..clb_net[inet].pins]
    vtr::vector<ClusterNetId, float*> delay_upper_bound; //[0..num_nets][0..clb_net[inet].pins]

    /*used to keep count the number of continuous time this node was congested*/
    vtr::vector<ClusterNetId, int> num_times_congested; //[0..num_nets]

    /*memory management for the budgets*/
    vtr::t_chunk min_budget_delay_ch;
    vtr::t_chunk max_budget_delay_ch;
    vtr::t_chunk target_budget_delay_ch;
    vtr::t_chunk lower_bound_delay_ch;
    vtr::t_chunk upper_bound_delay_ch;

    /*budgets only valid when loaded*/
    bool set;
};

#endif /* ROUTE_BUDGETS_H */
