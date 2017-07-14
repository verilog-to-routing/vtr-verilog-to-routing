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

using namespace std;

class route_budgets {
public:
    route_budgets();

    route_budgets(vector<vector<float>> net_delay);

    virtual ~route_budgets();

    float get_delay_target(int inet, int isink);
    float get_min_delay_target(int inet, int isink);
    float get_max_delay_target(int inet, int isink);
    float get_crit_short_path (int inet, int isink);
private:

    vector<vector<float>> delay_min_budget;
    vector<vector<float>> delay_max_budget;
    vector<vector<float>> delay_target;
    vector<vector<float>> delay_lower_bound;
    vector<vector<float>> delay_upper_bound;
};

#endif /* ROUTE_BUDGETS_H */

