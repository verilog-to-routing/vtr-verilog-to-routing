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

#include "RoutingDelayCalculator.h"

using namespace std;

class route_budgets {
public:
    route_budgets();

    route_budgets(vector<vector<float>> net_delay);

    virtual ~route_budgets();

    float get_delay_target(int source, int sink);
    float get_min_delay_budget(int source, int sink);
    float get_max_delay_budget(int source, int sink);
    float get_crit_short_path(int source, int sink);
    void load_route_budgets(float ** net_delay);
    void print_route_budget();
    bool if_set();
    void update_congestion_times(int inet);
    void lower_budgets();
    void not_congested_this_iteration(int inet);

private:

    std::shared_ptr<RoutingDelayCalculator> get_routing_calc(float ** net_delay);

    tatum::EdgeId get_edge_from_nets(int inet, int ipin);

    vector<vector<float>> delay_min_budget;//[0..num_nets][0..clb_net[inet].pins]
    vector<vector<float>> delay_max_budget;//[0..num_nets][0..clb_net[inet].pins]
    vector<vector<float>> delay_target;//[0..num_nets][0..clb_net[inet].pins]
    vector<vector<float>> delay_lower_bound;//[0..num_nets][0..clb_net[inet].pins]
    vector<vector<float>> delay_upper_bound;//[0..num_nets][0..clb_net[inet].pins]
    
    vector <int> num_times_congested; //[0..num_nets]
    

    bool set;
};

#endif /* ROUTE_BUDGETS_H */

