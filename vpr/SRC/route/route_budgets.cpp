/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   route_budgets.cpp
 * Author: jasmine
 * 
 * Created on July 14, 2017, 11:34 AM
 */

#include <algorithm>
#include "vpr_context.h"
#include "globals.h"

#include "route_budgets.h"

#define SHORT_PATH_EXP 0.5

route_budgets::route_budgets() {
}

route_budgets::~route_budgets() {
}

route_budgets::route_budgets(vector<vector<float>> net_delay) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //set lower bound to 0
    delay_min_budget.resize(cluster_ctx.clbs_nlist.net.size(), vector<float>(0));
    delay_target.resize(cluster_ctx.clbs_nlist.net.size(), vector<float>(0));

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        delay_min_budget[inet].resize(cluster_ctx.clbs_nlist.net[inet].num_sinks(), 0);
        delay_target[inet].resize(cluster_ctx.clbs_nlist.net.size(), 0);
    }

    //set upper bound
    delay_max_budget = net_delay;

    //Use RCV algorithm for delay target
    //Tend towards minimum to consider short path timing delay more

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (int isink = 0; isink < cluster_ctx.clbs_nlist.net[inet].num_sinks(); isink++) {
            delay_target[inet][isink] = min(0.5 * delay_min_budget[inet][isink] + delay_max_budget[inet][isink], delay_min_budget[inet][isink] + 0.1);
        }
    }
}

float route_budgets::get_delay_target(int inet, int isink) {
    return delay_target[inet][isink];
}

float route_budgets::get_min_delay_target(int inet, int isink) {
    return delay_min_budget[inet][isink];
}

float route_budgets::get_max_delay_target(int inet, int isink) {
    return delay_max_budget[inet][isink];
}

float route_budgets::get_crit_short_path(int inet, int isink) {

    return pow(((delay_target[inet][isink] - delay_lower_bound[inet][isink])/delay_target[inet][isink]), SHORT_PATH_EXP);
}