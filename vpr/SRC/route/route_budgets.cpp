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
#include "tatum/util/tatum_assert.hpp"

#include "tatum/timing_analyzers.hpp"
#include "tatum/graph_walkers.hpp"
#include "tatum/analyzer_factory.hpp"

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/TimingReporter.hpp"
#include "tatum/timing_paths.hpp"

#include "tatum/delay_calc/FixedDelayCalculator.hpp"

#include "tatum/report/graphviz_dot_writer.hpp"
#include "tatum/base/sta_util.hpp"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "slack_evaluation.h"
#include "tatum/TimingGraphFwd.hpp"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "route_timing.h"
#include "tatum/report/TimingPathFwd.hpp"
#include "tatum/base/TimingType.hpp"
#include "timing_info.h"
#include "tatum/echo_writer.hpp"
#include "path_delay.h"
#include "net_delay.h"
#include "route_budgets.h"

#define SHORT_PATH_EXP 0.5
#define MIN_DELAY_DECREMENT 1e-9

route_budgets::route_budgets() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    num_times_congested.resize(cluster_ctx.clbs_nlist.net.size(), 0);

    min_budget_delay_ch = {NULL, 0, NULL};
    max_budget_delay_ch = {NULL, 0, NULL};
    target_budget_delay_ch = {NULL, 0, NULL};
    lower_bound_delay_ch = {NULL, 0, NULL};
    upper_bound_delay_ch = {NULL, 0, NULL};

    set = false;
}

route_budgets::~route_budgets() {
    if (set) {
        free_net_delay(delay_min_budget, &min_budget_delay_ch);
        free_net_delay(delay_max_budget, &max_budget_delay_ch);
        free_net_delay(delay_target, &target_budget_delay_ch);
        free_net_delay(delay_lower_bound, &lower_bound_delay_ch);
        free_net_delay(delay_upper_bound, &upper_bound_delay_ch);
        num_times_congested.clear();
    } else {
        vtr::free_chunk_memory(&min_budget_delay_ch);
        vtr::free_chunk_memory(&max_budget_delay_ch);
        vtr::free_chunk_memory(&target_budget_delay_ch);
        vtr::free_chunk_memory(&lower_bound_delay_ch);
        vtr::free_chunk_memory(&upper_bound_delay_ch);
    }
    set = false;
}

void route_budgets::load_route_budgets(float ** net_delay,
        std::shared_ptr<const SetupTimingInfo> timing_info,
        const IntraLbPbPinLookup& pb_gpin_lookup, t_router_opts router_opts) {

    if (router_opts.routing_budgets_algorithm == DISABLE) {
        //disable budgets
        set = false;
        return;
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();

    //allocate memory for budgets
    delay_min_budget = alloc_net_delay(&min_budget_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
    delay_target = alloc_net_delay(&target_budget_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
    delay_max_budget = alloc_net_delay(&max_budget_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
    delay_lower_bound = alloc_net_delay(&lower_bound_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
    delay_upper_bound = alloc_net_delay(&upper_bound_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ++ipin) {
            delay_lower_bound[inet][ipin] = 0;
            delay_upper_bound[inet][ipin] = 100e-9;
            delay_max_budget[inet][ipin] = delay_lower_bound[inet][ipin];
        }
    }

    if (router_opts.routing_budgets_algorithm == MINIMAX) {
        allocate_slack_minimax_PERT(net_delay, pb_gpin_lookup);
        calculate_delay_tagets();
    } else if (router_opts.routing_budgets_algorithm == SCALE_DELAY) {
        allocate_slack_using_delays_and_criticalities(net_delay, timing_info, pb_gpin_lookup, router_opts);
    }
    set = true;
}

void route_budgets::calculate_delay_tagets() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            delay_target[inet][ipin] = min(0.5 * (delay_min_budget[inet][ipin] + delay_max_budget[inet][ipin]), delay_min_budget[inet][ipin] + 0.1e-9);
        }
    }
}

void route_budgets::allocate_slack_minimax_PERT(float ** net_delay, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::shared_ptr<SetupTimingInfo> long_path_timing_info = NULL;
    std::shared_ptr<HoldTimingInfo> short_path_timing_info = NULL;

    unsigned iteration;
    float max_budget_change;

    iteration = 0;
    max_budget_change = 6e-12;

    //problematic cause pointers
    //delay_max_budget = delay_lower_bound;

    //    while (iteration < 7 && max_budget_change > 5e-12) {
    //        //cout << "finished 1" << endl;
    //        short_path_timing_info = perform_short_path_sta(delay_max_budget);
    //        allocate_negative_short_path_slack(short_path_timing_info, delay_max_budget, net_delay, pb_gpin_lookup);
    //        keep_budget_in_bounds(MIN, delay_max_budget);
    //
    //        long_path_timing_info = perform_long_path_sta(delay_max_budget);
    //        allocate_negative_long_path_slack(long_path_timing_info, delay_max_budget, net_delay, pb_gpin_lookup);
    //        keep_budget_in_bounds(MAX, delay_max_budget);
    //
    //        iteration++;
    //    }

    iteration = 0;
    max_budget_change = 900e-12;

    while (iteration < 3 && max_budget_change > 800e-12) {
        //cout << "1" << endl;
        long_path_timing_info = perform_long_path_sta(delay_max_budget);
        allocate_long_path_slack(long_path_timing_info, delay_max_budget, net_delay, pb_gpin_lookup);
        keep_budget_in_bounds(MIN, delay_max_budget);

        iteration++;
        if (iteration > 7)
            break;
    }

    //print_temporary_budgets_to_file(delay_max_budget);

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            delay_min_budget[inet][ipin] = delay_max_budget[inet][ipin];

        }
    }

    iteration = 0;
    max_budget_change = 900e-12;

    while (iteration < 3 && max_budget_change > 800e-12) {
        //cout << "2" << endl;
        short_path_timing_info = perform_short_path_sta(delay_min_budget);
        allocate_short_path_slack(short_path_timing_info, delay_min_budget, net_delay, pb_gpin_lookup);
        keep_budget_in_bounds(MAX, delay_min_budget);
        iteration++;

        if (iteration > 7)
            break;
    }

    float bottom_range = -1e-9;
    short_path_timing_info = perform_short_path_sta(delay_min_budget);
    allocate_short_path_slack(short_path_timing_info, delay_min_budget, net_delay, pb_gpin_lookup);
    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            delay_min_budget[inet][ipin] = max(delay_min_budget[inet][ipin], bottom_range);
        }
    }

}

void route_budgets::keep_budget_in_bounds(max_or_min _type, float ** temp_budgets) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            if (_type == MAX) {
                temp_budgets[inet][ipin] = max(temp_budgets[inet][ipin], delay_lower_bound[inet][ipin]);
            }
            if (_type == MIN) {
                temp_budgets[inet][ipin] = min(temp_budgets[inet][ipin], delay_upper_bound[inet][ipin]);
            }
        }
    }
}

void route_budgets::allocate_short_path_slack(std::shared_ptr<HoldTimingInfo> timing_info, float ** temp_budgets,
        float ** net_delay, const IntraLbPbPinLookup& pb_gpin_lookup) {

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<const tatum::HoldTimingAnalyzer> timing_analyzer = timing_info->hold_analyzer();
    int iteration = 0;
    float average_slack_difference = 0;
    float new_path_slack = 0;
    float path_slack = 0;
    float total_path_delay = 0;
    float final_required_time, future_path_delay, past_path_delay;

    do {
        for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
            for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

                //get delay value
                const t_net_pin& net_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin];
                std::vector<AtomPinId> atom_pins = find_clb_pin_connected_atom_pins(net_pin.block, net_pin.block_pin, pb_gpin_lookup);
                for (const AtomPinId atom_pin : atom_pins) {
                    tatum::NodeId timing_node = atom_ctx.lookup.atom_pin_tnode(atom_pin);

                    auto arrival_tags = timing_analyzer->hold_tags(timing_node, tatum::TagType::DATA_ARRIVAL);
                    auto required_tags = timing_analyzer->hold_tags(timing_node, tatum::TagType::DATA_REQUIRED);

                    if (arrival_tags.empty() || required_tags.empty()) {
                        continue;
                    }

                    auto min_arrival_tag_iter = find_minimum_tag(arrival_tags);
                    auto min_required_tag_iter = find_minimum_tag(required_tags);

                    tatum::NodeId sink_node = min_required_tag_iter->origin_node();
                    if (sink_node == tatum::NodeId::INVALID()) {
                        continue;
                    }
                    auto sink_node_tags = timing_analyzer->hold_tags(sink_node, tatum::TagType::DATA_REQUIRED);

                    if (sink_node_tags.empty()) {
                        continue;
                    }

                    auto min_sink_node_tag_iter = find_minimum_tag(sink_node_tags);

                    if (min_required_tag_iter != required_tags.end() && min_arrival_tag_iter != arrival_tags.end()
                            && min_required_tag_iter != sink_node_tags.end()) {

                        final_required_time = min_sink_node_tag_iter->time().value();

                        future_path_delay = final_required_time - min_required_tag_iter->time().value();
                        past_path_delay = min_arrival_tag_iter->time().value();

                        //cout << past_path_delay << " " << future_path_delay << endl;
                        total_path_delay = max(past_path_delay + future_path_delay, total_path_delay);

                    } else {
                        //No tags (e.g. driven by constant generator)
                        continue;
                    }

                }

                if (total_path_delay == 0) {
                    temp_budgets[inet][ipin] = delay_upper_bound[inet][ipin];
                    continue;
                }

                //calculate slack
                new_path_slack = calculate_hold_clb_pin_slack(inet, ipin, timing_info, pb_gpin_lookup);

                if (iteration == 0) {
                    average_slack_difference = 1e-12;
                } else {
                    average_slack_difference = new_path_slack - path_slack;
                }

                path_slack = new_path_slack;

                //cout << net_delay[inet][ipin] << " " << path_slack << " " << total_path_delay << endl;
                if (path_slack > 0) {
                    temp_budgets[inet][ipin] -= net_delay[inet][ipin] * path_slack / total_path_delay;
                }
            }
        }
        iteration++;
    } while (iteration < 5 && average_slack_difference < 1e-12);

}

void route_budgets::allocate_long_path_slack(std::shared_ptr<SetupTimingInfo> timing_info, float ** temp_budgets,
        float ** net_delay, const IntraLbPbPinLookup& pb_gpin_lookup) {


    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<const tatum::SetupTimingAnalyzer> timing_analyzer = timing_info->setup_analyzer();
    int iteration = 0;
    float average_slack_difference = 0;
    float new_path_slack = 0;
    float path_slack = 0;
    float total_path_delay = 0;
    float final_required_time, future_path_delay, past_path_delay;

    do {
        for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
            //AtomNetId atom_net = atom_ctx.lookup.atom_net(inet);
            //AtomPinId driver_pin = atom_ctx.nlist.net_driver(atom_net);
            for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

                //get delay value
                const t_net_pin& net_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin];
                std::vector<AtomPinId> atom_pins = find_clb_pin_connected_atom_pins(net_pin.block, net_pin.block_pin, pb_gpin_lookup);
                for (const AtomPinId atom_pin : atom_pins) {
                    tatum::NodeId timing_node = atom_ctx.lookup.atom_pin_tnode(atom_pin);

                    auto arrival_tags = timing_analyzer->setup_tags(timing_node, tatum::TagType::DATA_ARRIVAL);
                    auto required_tags = timing_analyzer->setup_tags(timing_node, tatum::TagType::DATA_REQUIRED);

                    if (arrival_tags.empty() || required_tags.empty()) {
                        continue;
                    }

                    auto min_arrival_tag_iter = find_minimum_tag(arrival_tags);
                    auto min_required_tag_iter = find_minimum_tag(required_tags);

                    tatum::NodeId sink_node = min_required_tag_iter->origin_node();
                    if (sink_node == tatum::NodeId::INVALID()) {
                        continue;
                    }
                    auto sink_node_tags = timing_analyzer->setup_tags(sink_node, tatum::TagType::DATA_REQUIRED);

                    if (sink_node_tags.empty()) {
                        continue;
                    }
                    auto min_sink_node_tag_iter = find_minimum_tag(sink_node_tags);

                    if (min_required_tag_iter != required_tags.end() && min_arrival_tag_iter != arrival_tags.end()
                            && min_required_tag_iter != sink_node_tags.end()) {

                        final_required_time = min_sink_node_tag_iter->time().value();

                        future_path_delay = final_required_time - min_required_tag_iter->time().value();
                        past_path_delay = min_arrival_tag_iter->time().value();

                        //cout << past_path_delay << " " << future_path_delay << endl;
                        total_path_delay = max(past_path_delay + future_path_delay, total_path_delay);

                    } else {
                        //No tags (e.g. driven by constant generator)
                        continue;
                    }
                }

                if (total_path_delay == 0) {
                    temp_budgets[inet][ipin] = delay_upper_bound[inet][ipin];
                    continue;
                }

                //calculate slack
                new_path_slack = calculate_setup_clb_pin_slack(inet, ipin, timing_info, pb_gpin_lookup);

                if (iteration == 0) {
                    average_slack_difference = 1e-12;
                } else {
                    average_slack_difference = new_path_slack - path_slack;
                }

                path_slack = new_path_slack;

                //cout << net_delay[inet][ipin] << " " << path_slack << " " << total_path_delay << endl;

                if (path_slack > 0) {
                    temp_budgets[inet][ipin] += net_delay[inet][ipin] * path_slack / total_path_delay;
                }

            }
        }
        iteration++;
    } while (iteration < 5 && average_slack_difference < 1e-12);

}

void route_budgets::allocate_negative_short_path_slack(std::shared_ptr<HoldTimingInfo> timing_info, float ** temp_budgets,
        float ** net_delay, const IntraLbPbPinLookup& pb_gpin_lookup) {


    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<const tatum::HoldTimingAnalyzer> timing_analyzer = timing_info->hold_analyzer();
    int iteration = 0;
    float average_slack_difference = 0;
    float new_path_slack = 0;
    float path_slack = 0;
    float total_path_delay = 0;

    do {
        for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
            //AtomNetId atom_net = atom_ctx.lookup.atom_net(inet);
            //AtomPinId driver_pin = atom_ctx.nlist.net_driver(atom_net);
            for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

                //get delay value
                const t_net_pin& net_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin];
                std::vector<AtomPinId> atom_pins = find_clb_pin_connected_atom_pins(net_pin.block, net_pin.block_pin, pb_gpin_lookup);
                for (const AtomPinId atom_pin : atom_pins) {
                    tatum::NodeId timing_node = atom_ctx.lookup.atom_pin_tnode(atom_pin);

                    auto arrival_tags = timing_analyzer->hold_tags(timing_node, tatum::TagType::DATA_ARRIVAL);
                    auto required_tags = timing_analyzer->hold_tags(timing_node, tatum::TagType::DATA_REQUIRED);

                    auto min_arrival_tag_iter = find_minimum_tag(arrival_tags);
                    auto min_required_tag_iter = find_minimum_tag(required_tags);

                    if (min_arrival_tag_iter->time().value() == std::numeric_limits<float>::infinity() ||
                            min_arrival_tag_iter->time().value() == -1 * std::numeric_limits<float>::infinity() ||
                            min_required_tag_iter->time().value() == std::numeric_limits<float>::infinity() ||
                            min_required_tag_iter->time().value() == -1 * std::numeric_limits<float>::infinity()) {
                        //invalid
                        continue;
                    } else if (min_required_tag_iter != required_tags.end() && min_arrival_tag_iter != arrival_tags.end()) {

                        //cout << min_arrival_tag_iter->time().value() << " " << -1 * min_required_tag_iter->time().value() << endl;
                        total_path_delay = max(min_arrival_tag_iter->time().value() + (-1 * min_required_tag_iter->time().value()), total_path_delay);
                    } else {
                        //No tags (e.g. driven by constant generator)
                        continue;
                    }
                }

                if (total_path_delay == 0) {
                    temp_budgets[inet][ipin] = delay_upper_bound[inet][ipin];
                    continue;
                }

                //calculate slack
                new_path_slack = calculate_hold_clb_pin_slack(inet, ipin, timing_info, pb_gpin_lookup);

                if (iteration == 0) {
                    average_slack_difference = 1e-12;
                } else {
                    average_slack_difference = new_path_slack - path_slack;
                }

                path_slack = new_path_slack;

                //cout << net_delay[inet][ipin] << " " << path_slack << " " << total_path_delay << endl;
                if (path_slack < 0) {
                    temp_budgets[inet][ipin] -= net_delay[inet][ipin] * path_slack / total_path_delay;
                }

            }
        }
        iteration++;
    } while (iteration < 5 && average_slack_difference < 1e-12);

}

void route_budgets::allocate_negative_long_path_slack(std::shared_ptr<SetupTimingInfo> timing_info, float ** temp_budgets,
        float ** net_delay, const IntraLbPbPinLookup& pb_gpin_lookup) {


    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<const tatum::SetupTimingAnalyzer> timing_analyzer = timing_info->setup_analyzer();
    int iteration = 0;
    float average_slack_difference = 0;
    float new_path_slack = 0;
    float path_slack = 0;
    float total_path_delay = 0;

    do {
        for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
            //AtomNetId atom_net = atom_ctx.lookup.atom_net(inet);
            //AtomPinId driver_pin = atom_ctx.nlist.net_driver(atom_net);
            for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

                //get delay value
                const t_net_pin& net_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin];
                std::vector<AtomPinId> atom_pins = find_clb_pin_connected_atom_pins(net_pin.block, net_pin.block_pin, pb_gpin_lookup);
                for (const AtomPinId atom_pin : atom_pins) {
                    tatum::NodeId timing_node = atom_ctx.lookup.atom_pin_tnode(atom_pin);

                    auto arrival_tags = timing_analyzer->setup_tags(timing_node, tatum::TagType::DATA_ARRIVAL);
                    auto required_tags = timing_analyzer->setup_tags(timing_node, tatum::TagType::DATA_REQUIRED);

                    auto min_arrival_tag_iter = find_minimum_tag(arrival_tags);
                    auto min_required_tag_iter = find_minimum_tag(required_tags);

                    if (min_arrival_tag_iter->time().value() == std::numeric_limits<float>::infinity() ||
                            min_arrival_tag_iter->time().value() == -1 * std::numeric_limits<float>::infinity() ||
                            min_required_tag_iter->time().value() == std::numeric_limits<float>::infinity() ||
                            min_required_tag_iter->time().value() == -1 * std::numeric_limits<float>::infinity()) {
                        continue;
                    } else if (min_required_tag_iter != required_tags.end() && min_arrival_tag_iter != arrival_tags.end()) {

                        //cout << min_arrival_tag_iter->time().value() << " " << -1 * min_required_tag_iter->time().value() << endl;
                        total_path_delay = max(min_arrival_tag_iter->time().value() + (-1 * min_required_tag_iter->time().value()), total_path_delay);
                    } else {
                        //No tags (e.g. driven by constant generator)
                        continue;
                    }
                }

                if (total_path_delay == 0) {
                    temp_budgets[inet][ipin] = delay_upper_bound[inet][ipin];
                    continue;
                }

                //calculate slack
                new_path_slack = calculate_setup_clb_pin_slack(inet, ipin, timing_info, pb_gpin_lookup);

                if (iteration == 0) {
                    average_slack_difference = 1e-12;
                } else {
                    average_slack_difference = new_path_slack - path_slack;
                }

                path_slack = new_path_slack;

                //cout << net_delay[inet][ipin] << " " << path_slack << " " << total_path_delay << endl;
                if (path_slack < 0) {
                    temp_budgets[inet][ipin] += net_delay[inet][ipin] * path_slack / total_path_delay;

                }
            }
        }
        iteration++;
    } while (iteration < 5 && average_slack_difference < 1e-12);
}

float route_budgets::calculate_setup_clb_pin_slack(int inet, int ipin, std::shared_ptr<SetupTimingInfo> timing_info, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const t_net_pin& net_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin];

    //There may be multiple atom netlist pins connected to this CLB pin
    std::vector<AtomPinId> atom_pins = find_clb_pin_connected_atom_pins(net_pin.block, net_pin.block_pin, pb_gpin_lookup);

    //Take the minimum of the atom pin slack as the CLB pin slack
    float clb_min_slack = delay_upper_bound[inet][ipin];
    for (const AtomPinId atom_pin : atom_pins) {
        if (timing_info->setup_pin_slack(atom_pin) == std::numeric_limits<float>::infinity()) {
            continue;
        } else {
            clb_min_slack = std::min(clb_min_slack, timing_info->setup_pin_slack(atom_pin));
        }

        //cout << clb_min_slack << " ";
    }
    atom_pins.clear();

    return clb_min_slack;
}

float route_budgets::calculate_hold_clb_pin_slack(int inet, int ipin, std::shared_ptr<HoldTimingInfo> timing_info, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const t_net_pin& net_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin];

    //There may be multiple atom netlist pins connected to this CLB pin
    std::vector<AtomPinId> atom_pins = find_clb_pin_connected_atom_pins(net_pin.block, net_pin.block_pin, pb_gpin_lookup);

    //Take the minimum of the atom pin slack as the CLB pin slack
    float clb_min_slack = delay_upper_bound[inet][ipin];
    for (const AtomPinId atom_pin : atom_pins) {
        if (timing_info->hold_pin_slack(atom_pin) == std::numeric_limits<float>::infinity()) {
            continue;
        } else {
            clb_min_slack = std::min(clb_min_slack, timing_info->hold_pin_slack(atom_pin));
        }

        //cout << clb_min_slack << " ";
    }
    atom_pins.clear();

    return clb_min_slack;
}

void route_budgets::allocate_slack_using_delays_and_criticalities(float ** net_delay,
        std::shared_ptr<const SetupTimingInfo> timing_info,
        const IntraLbPbPinLookup& pb_gpin_lookup, t_router_opts router_opts) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    float pin_criticality;
    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            pin_criticality = calculate_clb_net_pin_criticality(*timing_info, pb_gpin_lookup, inet, ipin);

            /* Pin criticality is between 0 and 1. 
             * Shift it downwards by 1 - max_criticality (max_criticality is 0.99 by default, 
             * so shift down by 0.01) and cut off at 0.  This means that all pins with small 
             * criticalities (<0.01) get criticality 0 and are ignored entirely, and everything 
             * else becomes a bit less critical. This effect becomes more pronounced if 
             * max_criticality is set lower. */
            // VTR_ASSERT(pin_criticality[ipin] > -0.01 && pin_criticality[ipin] < 1.01);
            pin_criticality = max(pin_criticality - (1.0 - router_opts.max_criticality), 0.0);

            /* Take pin criticality to some power (1 by default). */
            pin_criticality = pow(pin_criticality, router_opts.criticality_exp);

            /* Cut off pin criticality at max_criticality. */
            pin_criticality = min(pin_criticality, router_opts.max_criticality);


            delay_min_budget[inet][ipin] = 0;
            delay_lower_bound[inet][ipin] = 0;
            delay_upper_bound[inet][ipin] = 100e-9;

            if (pin_criticality == 0) {
                //prevent invalid division
                delay_max_budget[inet][ipin] = delay_upper_bound[inet][ipin];
            } else {
                delay_max_budget[inet][ipin] = min(net_delay[inet][ipin] / pin_criticality, delay_upper_bound[inet][ipin]);
            }

            VTR_ASSERT_MSG(delay_max_budget[inet][ipin] >= delay_min_budget[inet][ipin]
                    && delay_lower_bound[inet][ipin] <= delay_min_budget[inet][ipin]
                    && delay_upper_bound[inet][ipin] >= delay_max_budget[inet][ipin]
                    && delay_upper_bound[inet][ipin] >= delay_lower_bound[inet][ipin],
                    "Delay budgets do not fit in delay bounds");

            //Use RCV algorithm for delay target
            //Tend towards minimum to consider short path timing delay more
            delay_target[inet][ipin] = min(0.5 * (delay_min_budget[inet][ipin] + delay_max_budget[inet][ipin]), delay_min_budget[inet][ipin] + 0.1e-9);

        }
    }
}

std::shared_ptr<SetupTimingInfo> route_budgets::perform_long_path_sta(float ** temp_budgets) {
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<RoutingDelayCalculator> routing_delay_calc =
            std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, temp_budgets);

    std::shared_ptr<SetupTimingInfo> timing_info = make_setup_hold_timing_info(routing_delay_calc);
    timing_info->update();

    return timing_info;
}

std::shared_ptr<HoldTimingInfo> route_budgets::perform_short_path_sta(float ** temp_budgets) {
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<RoutingDelayCalculator> routing_delay_calc =
            std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, temp_budgets);

    std::shared_ptr<HoldTimingInfo> timing_info = make_setup_hold_timing_info(routing_delay_calc);
    timing_info->update();

    return timing_info;
}

void route_budgets::update_congestion_times(int inet) {

    num_times_congested[inet]++;
}

void route_budgets::not_congested_this_iteration(int inet) {

    num_times_congested[inet] = 0;
}

void route_budgets::lower_budgets() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        if (num_times_congested[inet] >= 3) {
            for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
                if (delay_min_budget[inet][ipin] - delay_lower_bound[inet][ipin] >= 1e-9) {

                    delay_min_budget[inet][ipin] = delay_min_budget[inet][ipin] - MIN_DELAY_DECREMENT;
                }
            }
        }
    }
}

float route_budgets::get_delay_target(int inet, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);

    return delay_target[inet][ipin];
}

float route_budgets::get_min_delay_budget(int inet, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);

    return delay_min_budget[inet][ipin];
}

float route_budgets::get_max_delay_budget(int inet, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);

    return delay_max_budget[inet][ipin];
}

float route_budgets::get_crit_short_path(int inet, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);
    if (delay_target[inet][ipin] == 0) {

        return 0;
    }
    return pow(((delay_target[inet][ipin] - delay_lower_bound[inet][ipin]) / delay_target[inet][ipin]), SHORT_PATH_EXP);
}

void route_budgets::print_route_budget() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    unsigned inet, ipin;
    fstream fp;
    fp.open("route_budget.txt", fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                "could not open \"route_budget.txt\" for generating route budget file\n");
    }

    fp << "Minimum Delay Budgets:" << endl;
    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            fp << delay_min_budget[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Maximum Delay Budgets:" << endl;
    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            fp << delay_max_budget[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Target Delay Budgets:" << endl;

    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            fp << delay_target[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Delay lower_bound:" << endl;
    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            fp << delay_lower_bound[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Target Delay Budgets:" << endl;
    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

            fp << delay_upper_bound[inet][ipin] << " ";
        }
    }

    fp.close();
}

void route_budgets::print_temporary_budgets_to_file(float ** temp_budgets) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    unsigned inet, ipin;
    fstream fp;
    fp.open("temporary_budgets.txt", fstream::out | fstream::trunc);


    fp << "Temporary Budgets:" << endl;
    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

            fp << temp_budgets[inet][ipin] << " ";
        }
    }
}

bool route_budgets::if_set() {
    return set;
}


//std::shared_ptr<RoutingDelayCalculator> route_budgets::get_routing_calc(float ** net_delay) {
//    auto& timing_ctx = g_vpr_ctx.timing();
//    auto& atom_ctx = g_vpr_ctx.atom();
//    std::shared_ptr<tatum::TimingGraph> timing_graph = timing_ctx.graph;
//
//    //vtr::t_chunk net_delay_ch = {NULL, 0, NULL};
//    //tatum::FixedDelayCalculator delay_calculator = tatum::FixedDelayCalculator(max_delay_edges_, setup_times_, min_delay_edges_, hold_times_);
//    //float **net_delay = alloc_net_delay(&net_delay_ch, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());
//    std::shared_ptr<RoutingDelayCalculator> routing_delay_calc = std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay);
//
//    cout << "delay_model:\n";
//    for (auto edge_id : timing_graph->edges()) {
//        NodeId src_node = timing_graph->edge_src_node(edge_id);
//        NodeId sink_node = timing_graph->edge_sink_node(edge_id);
//
//        cout << " edge: " << size_t(edge_id);
//        if (timing_graph->node_type(src_node) == NodeType::CPIN && timing_graph->node_type(sink_node) == NodeType::SINK) {
//            cout << " setup_time: " << routing_delay_calc->setup_time(*timing_graph, edge_id).value();
//            cout << " hold_time: " << routing_delay_calc->hold_time(*timing_graph, edge_id).value();
//        } else {
//            cout << " min_delay: " << routing_delay_calc->min_edge_delay(*timing_graph, edge_id).value();
//            cout << " max_delay: " << routing_delay_calc->max_edge_delay(*timing_graph, edge_id).value();
//        }
//        cout << "\n";
//    }
//    cout << "\n";
//
//    return routing_delay_calc;
//}
//
//EdgeId route_budgets::get_edge_from_nets(int inet, int ipin) {
//    auto& cluster_ctx = g_vpr_ctx.clustering();
//    auto& timing_ctx = g_vpr_ctx.timing();
//    auto& atom_ctx = g_vpr_ctx.atom();
//    EdgeId edge_id;
//    AtomNetId atom_net_id = atom_ctx.lookup.atom_net(inet);
//
//    AtomPinId driver_pin = atom_ctx.nlist.net_driver(atom_net_id);
//    NodeId driver_tnode = atom_ctx.lookup.atom_pin_tnode(driver_pin);
//    VTR_ASSERT(driver_tnode);
//
//    for (AtomPinId sink_pin : atom_ctx.nlist.net_sinks(atom_net_id)) {
//        NodeId sink_tnode = atom_ctx.lookup.atom_pin_tnode(sink_pin);
//        VTR_ASSERT(sink_tnode);
//
//        edge_id = timing_ctx.graph->find_edge(driver_tnode, sink_tnode);
//    }
//
//    //    for (auto& block : atom_ctx.nlist.blocks()){
//    //        //Returns the clb index associated with blk_id
//    //        int atom_clb(const AtomBlockId blk_id) const;
//    //        
//    //        if (block.id == )
//    //    }
//    //    int iblk;
//    //    
//    //    const t_pb_graph_node* pb_graph_node = cluster_ctx.blocks[iblk].pb->pb_graph_node;
//    //    int cluster_pin_idx = pb_graph_node->input_pins[0][ipin].pin_count_in_cluster; //Unique pin index in cluster
//    //    int sink_cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
//    //
//    //    tatum::NodeId src_tnode_id = find_tnode(atom, cluster_pin_idx);
//    //    tatum::NodeId sink_tnode_id = find_tnode(atom, sink_cluster_pin_idx);
//    //
//
//
//    return edge_id;
//}
//
//void route_budgets::load_route_budgets(float ** net_delay) {
//    auto& timing_ctx = g_vpr_ctx.timing();
//    auto& device_ctx = g_vpr_ctx.device();
//    auto& route_ctx = g_vpr_ctx.routing();
//    auto& cluster_ctx = g_vpr_ctx.clustering();
//    std::shared_ptr<tatum::TimingGraph> timing_graph = timing_ctx.graph;
//
//    //set lower bound to 0 and upper bound to net_delay
//    delay_min_budget.resize(device_ctx.num_rr_nodes);
//    delay_target.resize(device_ctx.num_rr_nodes);
//    delay_max_budget.resize(device_ctx.num_rr_nodes);
//    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
//        delay_min_budget[inode].resize(device_ctx.rr_nodes[inode].num_edges(), 0);
//        delay_target[inode].resize(device_ctx.rr_nodes[inode].num_edges(), 0);
//        delay_max_budget[inode].resize(device_ctx.rr_nodes[inode].num_edges(), 0);
//    }
//
//    //load
//    std::shared_ptr<RoutingDelayCalculator> routing_delay_calc = get_routing_calc(net_delay);
//
//    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
//        for (int isink = 0; isink < device_ctx.rr_nodes[inode].num_edges; isink++) {
//            
//            
//            EdgeId edge_id = get_edge_from_nets(inet, ipin);
//            
//            EdgeId edge_id = get_edge_from_nets(inode, device_ctx.rr_nodes[inode].edge_sink_node(isink));
//            delay_min_budget[inode][isink] = routing_delay_calc->min_edge_delay(*timing_graph, edge_id).value();
//            delay_max_budget[inode][isink] = routing_delay_calc->max_edge_delay(*timing_graph, edge_id).value();
//        }
//    }
//    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
//        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
//
//            int source_node = route_ctx.net_rr_terminals[inet][0];
//            int sink_node = route_ctx.net_rr_terminals[inet][ipin];
//
//            int sink_edge = -1;
//            for (int iedge = 0; iedge < device_ctx.rr_nodes[source_node].num_edges(); iedge++) {
//                cout << source_node << " " <<iedge << " " << device_ctx.rr_nodes[source_node].edge_sink_node(iedge) <<
//                        " " << sink_node << endl;
//                if (device_ctx.rr_nodes[source_node].edge_sink_node(iedge) == sink_node) {
//                    sink_edge = iedge;
//                }
//            }
//            VTR_ASSERT(sink_edge != -1);
//
//            //int edge_id = get_edge_from_nets(source_node, device_ctx.rr_nodes[source_node].edge_sink_node(sink_edge));
//            EdgeId edge_id = get_edge_from_nets(inet, ipin);
//
//            delay_min_budget[source_node][sink_edge] = routing_delay_calc->min_edge_delay(*timing_graph, edge_id).value();
//            delay_max_budget[source_node][sink_edge] = routing_delay_calc->max_edge_delay(*timing_graph, edge_id).value();
//        }
//    }
//
//
//    //Use RCV algorithm for delay target
//    //Tend towards minimum to consider short path timing delay more
//
//    for (unsigned inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
//        for (int isink = 0; isink < device_ctx.rr_nodes[inode].num_edges(); isink++) {
//            delay_target[inode][isink] = min(0.5 * (delay_min_budget[inode][isink] + delay_max_budget[inode][isink]), delay_min_budget[inode][isink] + 0.1e-9);
//        }
//    }
//}

//void route_budgets::load_route_budgets(float ** net_delay) {
//    auto& device_ctx = g_vpr_ctx.device();
//
//    //set lower bound to 0 and upper bound to net_delay
//    delay_min_budget.resize(device_ctx.num_rr_nodes);
//    delay_target.resize(device_ctx.num_rr_nodes);
//    delay_max_budget.resize(device_ctx.num_rr_nodes);
//    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
//        delay_min_budget[inode].resize(device_ctx.rr_nodes[inode].num_edges(), 0);
//        delay_target[inode].resize(device_ctx.rr_nodes[inode].num_edges(), 0);
//        delay_max_budget[inode].resize(device_ctx.rr_nodes[inode].num_edges(), 0);
//    }
//
//    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
//        for (int isink = 0; isink < device_ctx.rr_nodes[inode].num_edges(); isink++) {
//            delay_min_budget[inode][isink] = 0;
//            delay_max_budget[inode][isink] = 0.1e-12;
//        }
//    }
//
//    //Use RCV algorithm for delay target
//    //Tend towards minimum to consider short path timing delay more
//
//    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
//        for (int isink = 0; isink < device_ctx.rr_nodes[inode].num_edges(); isink++) {
//            delay_target[inode][isink] = min(0.5 * (delay_min_budget[inode][isink] + delay_max_budget[inode][isink]), delay_min_budget[inode][isink] + 0.1e-9);
//        }
//    }
//    set = true;
//}
