/* 
 * File:   route_budgets.h
 * Author: jasmine
 *
 * Created on July 14, 2017, 11:34 AM
 */

#ifndef ROUTE_BUDGETS_H
#define ROUTE_BUDGETS_H

#include <iostream>
#include <vector>
#include "vtr_memory.h"
#include "RoutingDelayCalculator.h"

using namespace std;

enum max_or_min {
    MAX, MIN
};

enum analysis_type {
    SETUP, HOLD
};

enum slack_type_to_allocate {
    POSITIVE, NEGATIVE
};

class route_budgets {
public:
    route_budgets();

    route_budgets(vector<vector<float>> net_delay);

    virtual ~route_budgets();

    //getter functions
    float get_delay_target(int inet, int ipin);
    float get_min_delay_budget(int inet, int ipin);
    float get_max_delay_budget(int inet, int ipin);
    float get_crit_short_path(int inet, int ipin);
    bool if_set();

    //main loader function
    void load_route_budgets(float ** net_delay,
            std::shared_ptr<const SetupTimingInfo> timing_info,
            const IntraLbPbPinLookup& pb_gpin_lookup,
            t_router_opts router_opts);

    void allocate_slack_using_delays_and_criticalities(float ** net_delay,
            std::shared_ptr<const SetupTimingInfo> timing_info,
            const IntraLbPbPinLookup& pb_gpin_lookup, t_router_opts router_opts);
    void allocate_slack_minimax_PERT(float **net_delay, const IntraLbPbPinLookup& pb_gpin_lookup);

    std::shared_ptr<SetupHoldTimingInfo> perform_sta(float ** temp_budgets);

    void keep_budget_in_bounds(max_or_min _type, float ** temp_budgets);
    void keep_min_below_max_budget();

    void allocate_slack(std::shared_ptr<SetupHoldTimingInfo> timing_info, float ** temp_budgets,
            float ** net_delay, const IntraLbPbPinLookup& pb_gpin_lookup, analysis_type analysis_type,
            slack_type_to_allocate slack_type);

    float calculate_clb_pin_slack(int inet, int ipin, std::shared_ptr<SetupHoldTimingInfo> timing_info,
            const IntraLbPbPinLookup& pb_gpin_lookup, analysis_type type);

    float get_total_path_delay(std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> timing_analyzer,
            analysis_type analysis_type, tatum::NodeId timing_node);

    //debugging tools
    void print_route_budget();
    void print_temporary_budgets_to_file(float ** temp_budgets);

    //lower budgets during congestion
    void update_congestion_times(int inet);
    void lower_budgets();
    void not_congested_this_iteration(int inet);
private:

    std::shared_ptr<RoutingDelayCalculator> get_routing_calc(float ** net_delay);
    void calculate_delay_tagets();
    tatum::EdgeId get_edge_from_nets(int inet, int ipin);

    float ** delay_min_budget; //[0..num_nets][0..clb_net[inet].pins]
    float ** delay_max_budget; //[0..num_nets][0..clb_net[inet].pins]
    float ** delay_target; //[0..num_nets][0..clb_net[inet].pins]
    float ** delay_lower_bound; //[0..num_nets][0..clb_net[inet].pins]
    float ** delay_upper_bound; //[0..num_nets][0..clb_net[inet].pins]

    vector <int> num_times_congested; //[0..num_nets]

    vtr::t_chunk min_budget_delay_ch;
    vtr::t_chunk max_budget_delay_ch;
    vtr::t_chunk target_budget_delay_ch;
    vtr::t_chunk lower_bound_delay_ch;
    vtr::t_chunk upper_bound_delay_ch;


    bool set;
};

#endif /* ROUTE_BUDGETS_H */

