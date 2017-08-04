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
    float get_min_delay_target(int source, int sink);
    float get_max_delay_target(int source, int sink);
    float get_crit_short_path(int source, int sink);
    void load_route_budgets(float ** net_delay);
    void print_route_budget();
    bool if_set();

private:

    std::shared_ptr<RoutingDelayCalculator> get_routing_calc(float ** net_delay);

    tatum::EdgeId get_edge_from_nets(int inet, int ipin);

    vector<vector<float>> delay_min_budget;
    vector<vector<float>> delay_max_budget;
    vector<vector<float>> delay_target;
    vector<vector<float>> delay_lower_bound;
    vector<vector<float>> delay_upper_bound;

    bool set;
};

#endif /* ROUTE_BUDGETS_H */

