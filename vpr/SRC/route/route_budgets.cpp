/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   route_budgets.cpp
 * Author: Jia Min Wang
 * 
 * Created on July 14, 2017, 11:34 AM
 */

#include <algorithm>
#include "vpr_context.h"
#include <fstream>
#include "vpr_error.h"
#include "globals.h"

#include "route_budgets.h"

#define SHORT_PATH_EXP 0.5

route_budgets::route_budgets() {
}

route_budgets::~route_budgets() {
}

void route_budgets::load_route_budgets(float ** net_delay) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //set lower bound to 0 and upper bound to net_delay
    delay_min_budget.resize(cluster_ctx.clbs_nlist.net.size(), vector<float>(0));
    delay_target.resize(cluster_ctx.clbs_nlist.net.size(), vector<float>(0));
    delay_max_budget.resize(cluster_ctx.clbs_nlist.net.size(), vector<float>(0));
    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        delay_min_budget[inet].resize(cluster_ctx.clbs_nlist.net[inet].num_sinks(), 0);
        delay_target[inet].resize(cluster_ctx.clbs_nlist.net[inet].num_sinks(), 0);
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            delay_max_budget[inet].push_back(net_delay[inet][ipin]);
        }
    }

    //Use RCV algorithm for delay target
    //Tend towards minimum to consider short path timing delay more

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (int isink = 0; isink < cluster_ctx.clbs_nlist.net[inet].num_sinks(); isink++) {
            delay_target[inet][isink] = min(0.5 * (delay_min_budget[inet][isink] + delay_max_budget[inet][isink]), delay_min_budget[inet][isink] + 0.1e-9);
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

    return pow(((delay_target[inet][isink] - delay_lower_bound[inet][isink]) / delay_target[inet][isink]), SHORT_PATH_EXP);
}

void route_budgets::print_route_budget() {

    fstream fp;
    fp.open("route_budget.txt", fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                "couldn't open \"Route_budget.txt\" for generating route budget file\n");
    }

    fp << "Minimum Delay Budgets:" << endl;
    for (unsigned inet = 0; inet < delay_min_budget.size(); inet++) {
        fp << endl << "NET: " << inet << "            ";
        for (unsigned ipin = 0; ipin < delay_min_budget[inet].size(); ipin++) {
            fp << delay_min_budget[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Maximum Delay Budgets:" << endl;
    for (unsigned inet = 0; inet < delay_max_budget.size(); inet++) {
        fp << endl << "NET: " << inet << "            ";
        for (unsigned ipin = 0; ipin < delay_max_budget[inet].size(); ipin++) {
            fp << delay_max_budget[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Target Delay Budgets:" << endl;

    for (unsigned inet = 0; inet < delay_target.size(); inet++) {
        fp << endl << "NET: " << inet << "            ";
        for (unsigned ipin = 0; ipin < delay_target[inet].size(); ipin++) {
            fp << delay_target[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Delay lower_bound:" << endl;
    for (unsigned inet = 0; inet < delay_lower_bound.size(); inet++) {
        fp << endl << "NET: " << inet << "            ";
        for (unsigned ipin = 0; ipin < delay_lower_bound[inet].size(); ipin++) {
            fp << delay_lower_bound[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Target Delay Budgets:" << endl;
    for (unsigned inet = 0; inet < delay_upper_bound.size(); inet++) {
        fp << endl << "NET: " << inet << "            ";
        for (unsigned ipin = 0; ipin < delay_upper_bound[inet].size(); ipin++) {
            fp << delay_upper_bound[inet][ipin] << " ";
        }
    }

    fp.close();
}