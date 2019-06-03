/* This loads and manipulates the routing budgets. The budgets can be set
 * using different algorithms. The user chooses which algorithm to use using
 * the --routing_budgets_algorithm option. The minimax-PERT algorithm [H. Youssef,
 * R. B. Lin and E. Shragowitz, "Bounds on net delays for VLSI circuits,"
 * in IEEE Transactions on Circuits and Systems II: Analog and Digital Signal
 * Processing, vol. 39, no. 11, pp. 815-824, Nov 1992.] uses weights
 * and slacks to calculate how much slack to allocate each connection. Slack
 * allocated = slack * weight / max_weight_of_all_paths_through_the_connection.
 * The weight here is the delay of the connection. The other method of slack allocating
 * is to set the max budgets as a scale of the delays by the pin criticality, and setting
 * the min budgets to zero.
 * This is implemented in order to consider hold time during routing.
 * With these minimum and maximum budgets, a target budget is found using RCV
 * (R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve
 * FPGA Timing While Repairing Short-Path Violations," in IEEE Transactions
 * on Computer-Aided Design of Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.)
 * The routing cost function tries to get the delay closest to the target.
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
#include "net_delay.h"
#include "route_budgets.h"

#define SHORT_PATH_EXP 0.5

route_budgets::route_budgets() {
    set = false;
}

route_budgets::~route_budgets() {
    free_budgets();
}

void route_budgets::free_budgets() {
    /*Free associated budget memory if set
     * if not set, only free the chunk memory that wasn't used*/
    if (set) {
        free_net_delay(delay_min_budget, &min_budget_delay_ch);
        free_net_delay(delay_max_budget, &max_budget_delay_ch);
        free_net_delay(delay_target, &target_budget_delay_ch);
        free_net_delay(delay_lower_bound, &lower_bound_delay_ch);
        free_net_delay(delay_upper_bound, &upper_bound_delay_ch);
        num_times_congested.clear();
    }
    set = false;
}

void route_budgets::alloc_budget_memory() {
    /*All the budgets are allocated similar to the net delay in order to pass into the delay calculator*/

    delay_min_budget = alloc_net_delay(&min_budget_delay_ch);
    delay_target = alloc_net_delay(&target_budget_delay_ch);
    delay_max_budget = alloc_net_delay(&max_budget_delay_ch);
    delay_lower_bound = alloc_net_delay(&lower_bound_delay_ch);
    delay_upper_bound = alloc_net_delay(&upper_bound_delay_ch);
}

void route_budgets::load_initial_budgets() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            delay_lower_bound[net_id][ipin] = 0;
            delay_upper_bound[net_id][ipin] = 100e-9;

            //before all iterations, delay max budget is equal to the lower bound
            delay_max_budget[net_id][ipin] = delay_lower_bound[net_id][ipin];
            delay_min_budget[net_id][ipin] = delay_lower_bound[net_id][ipin];
        }
    }
}

void route_budgets::load_route_budgets(vtr::vector<ClusterNetId, float*>& net_delay,
                                       std::shared_ptr<SetupTimingInfo> timing_info,
                                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                       t_router_opts router_opts) {
    /*This function loads the routing budgets depending on the option selected by the user
     * the default is to use the minimax algorithm. Other options include disabling this feature
     * or scale the delay by the criticality*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    num_times_congested.resize(cluster_ctx.clb_nlist.nets().size(), 0);

    /*if chosen to be disable, never set the budgets*/
    if (router_opts.routing_budgets_algorithm == DISABLE) {
        //disable budgets
        set = false;
        return;
    }

    /*allocate and load memory for budgets*/
    alloc_budget_memory();
    load_initial_budgets();

    /*go to the associated function depending on user input/default settings*/
    if (router_opts.routing_budgets_algorithm == MINIMAX) {
        allocate_slack_using_weights(net_delay, netlist_pin_lookup);
        calculate_delay_targets();
    } else if (router_opts.routing_budgets_algorithm == SCALE_DELAY) {
        allocate_slack_using_delays_and_criticalities(net_delay, timing_info, netlist_pin_lookup, router_opts);
    }
    set = true;
}

void route_budgets::calculate_delay_targets() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    /*Delay target values are calculated based on the function outlined in the RCV algorithm*/
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            calculate_delay_targets(net_id, pin_id);
        }
    }
}

void route_budgets::calculate_delay_targets(ClusterNetId net_id, ClusterPinId pin_id) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

    /*Target delay is calculated using equation in the RCV algorithm*/
    delay_target[net_id][ipin] = min(0.5 * (delay_min_budget[net_id][ipin] + delay_max_budget[net_id][ipin]), delay_min_budget[net_id][ipin] + 0.1e-9);
}

void route_budgets::allocate_slack_using_weights(vtr::vector<ClusterNetId, float*>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup) {
    /*The minimax PERT algorithm uses a weight based approach to allocate slack for each connection
     * The formula used where c is a connection is
     * slack_allocated(c) = (slack(c)*weight(c)/max_weight_of_all_path_through(c)).
     * Weights here are defined as the delay for the connections
     * Values for conditions in the while loops are pulled from the RCV paper*/
    std::shared_ptr<SetupHoldTimingInfo> timing_info = nullptr;

    unsigned iteration;
    float max_budget_change;

    /*Preprocessing algorithm in order to consider short paths when setting initial maximum budgets.
     * Not necessary unless budgets are really hard to meet*/
    //process_negative_slack_using_minimax();

    iteration = 0;
    max_budget_change = 900e-12;

    /*This allocates long path slack and increases the budgets*/
    while ((iteration > 3 && max_budget_change > 800e-12) || iteration <= 3) {
        timing_info = perform_sta(delay_max_budget);
        max_budget_change = minimax_PERT(timing_info, delay_max_budget, net_delay, netlist_pin_lookup, SETUP, true);

        iteration++;
        if (iteration > 7)
            break;
    }

    /*Set the minimum budgets equal to the maximum budgets*/
    set_min_max_budgets_equal();

    iteration = 0;
    max_budget_change = 900e-12;

    /*Allocate the short path slack to decrease the budgets accordingly*/
    while ((iteration > 3 && max_budget_change > 800e-12) || iteration <= 3) {
        timing_info = perform_sta(delay_min_budget);
        max_budget_change = minimax_PERT(timing_info, delay_min_budget, net_delay, netlist_pin_lookup, HOLD, true);
        iteration++;

        if (iteration > 7)
            break;
    }

    /*Post basic algorithm processing
     *This prevents wasting resources by allowing the minimum budgets to go below
     * the lower bound so it will reflect absolute minimum delays in order to meet long path timing*/
    iteration = 0;
    max_budget_change = 900e-12;
    float bottom_range = -1e-9;
    while (iteration < 3 && max_budget_change > 800e-12) {
        /*budgets must be in bounds before timing analysis*/
        if (iteration != 0) {
            keep_budget_in_bounds(delay_min_budget);
        }
        timing_info = perform_sta(delay_min_budget);
        max_budget_change = minimax_PERT(timing_info, delay_min_budget, net_delay, netlist_pin_lookup, HOLD, false);
        iteration++;
    }
    /*budgets may go below minimum delay bound to optimize for setup time*/
    keep_budget_above_value(delay_min_budget, bottom_range);
}

void route_budgets::process_negative_slack_using_minimax(vtr::vector<ClusterNetId, float*>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup) {
    /*This function is an optional pre-processing for the maximum budgets.
     * This ensures that the short path slacks are also taken into account for the maximum budgets.
     * Ensures that maximum budgets will always be above minimum budgets.
     * Can be unnecessary for not so strict budgets*/
    unsigned iteration;
    float max_budget_change;
    std::shared_ptr<SetupHoldTimingInfo> timing_info = nullptr;

    iteration = 0;
    max_budget_change = 900e-12;
    float second_max_budget_change = 900e-12;

    while (iteration < 7 && max_budget_change > 5e-12) {
        timing_info = perform_sta(delay_max_budget);
        max_budget_change = minimax_PERT(timing_info, delay_max_budget, net_delay, netlist_pin_lookup, HOLD, true, NEGATIVE);

        timing_info = perform_sta(delay_max_budget);
        second_max_budget_change = minimax_PERT(timing_info, delay_max_budget, net_delay, netlist_pin_lookup, SETUP, true, NEGATIVE);
        max_budget_change = max(max_budget_change, second_max_budget_change);

        iteration++;
    }
}

void route_budgets::keep_budget_above_value(vtr::vector<ClusterNetId, float*>& temp_budgets, float bottom_range) {
    /*In post processing the minimum delay can go below the lower bound*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            temp_budgets[net_id][ipin] = max(temp_budgets[net_id][ipin], bottom_range);
        }
    }
}

void route_budgets::keep_budget_in_bounds(vtr::vector<ClusterNetId, float*>& temp_budgets) {
    /*Make sure the budget is between the lower and upper bounds*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            keep_budget_in_bounds(temp_budgets, net_id, pin_id);
        }
    }
}

void route_budgets::keep_budget_in_bounds(vtr::vector<ClusterNetId, float*>& temp_budgets, ClusterNetId net_id, ClusterPinId pin_id) {
    /*Make sure the budget is between the lower and upper bounds*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

    temp_budgets[net_id][ipin] = max(temp_budgets[net_id][ipin], delay_lower_bound[net_id][ipin]);
    temp_budgets[net_id][ipin] = min(temp_budgets[net_id][ipin], delay_upper_bound[net_id][ipin]);
}

void route_budgets::keep_min_below_max_budget() {
    /*Minimum budgets should always be below the maximum budgets.
     * Make them equal if minimum budget becomes bigger*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            if (delay_min_budget[net_id][ipin] > delay_max_budget[net_id][ipin]) {
                delay_max_budget[net_id][ipin] = delay_min_budget[net_id][ipin];
            }
        }
    }
}

void route_budgets::set_min_max_budgets_equal() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            delay_min_budget[net_id][ipin] = delay_max_budget[net_id][ipin];
        }
    }
}

float route_budgets::minimax_PERT(std::shared_ptr<SetupHoldTimingInfo> timing_info, vtr::vector<ClusterNetId, float*>& temp_budgets, vtr::vector<ClusterNetId, float*>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, analysis_type analysis_type, bool keep_in_bounds, slack_allocated_type slack_type) {
    /*This function uses weights to calculate how much slack to allocate to a connection.
     * The weights are deteremined by how much delay of the whole path is present in this connection*/

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> timing_analyzer = timing_info->setup_hold_analyzer();
    float total_path_delay = 0;
    float path_slack;
    float max_budget_change = 0;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            AtomPinId atom_pin;

            /*calculate slack, save the pin that has min slack to calculate total path delay*/
            if (analysis_type == HOLD) {
                path_slack = calculate_clb_pin_slack(net_id, ipin, timing_info, netlist_pin_lookup, HOLD, atom_pin);
            } else {
                path_slack = calculate_clb_pin_slack(net_id, ipin, timing_info, netlist_pin_lookup, SETUP, atom_pin);
            }

            tatum::NodeId timing_node = atom_ctx.lookup.atom_pin_tnode(atom_pin);
            total_path_delay = get_total_path_delay(timing_analyzer, analysis_type, timing_node);
            if (total_path_delay == -1) {
                /*Delay node is not valid, leave the budgets as is*/
                continue;
            }

            /*During hold analysis, increase the budgets when there is negative slack.
             * During setup analysis, decrease the budgets when there is negative slack*/
            if ((slack_type == NEGATIVE && path_slack < 0) || (slack_type == POSITIVE && path_slack > 0) || slack_type == BOTH) {
                if (analysis_type == HOLD) {
                    temp_budgets[net_id][ipin] += -1 * net_delay[net_id][ipin] * path_slack / total_path_delay;
                } else {
                    temp_budgets[net_id][ipin] += net_delay[net_id][ipin] * path_slack / total_path_delay;
                }
                max_budget_change = max(max_budget_change, abs(net_delay[net_id][ipin] * path_slack / total_path_delay));
            }

            /*Budgets need to be between maximum and minimum budgets*/
            if (keep_in_bounds) {
                keep_budget_in_bounds(temp_budgets, net_id, pin_id);
            }
        }
    }
    return max_budget_change;
}

float route_budgets::calculate_clb_pin_slack(ClusterNetId net_id, int ipin, std::shared_ptr<SetupHoldTimingInfo> timing_info, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, analysis_type type, AtomPinId& atom_pin) {
    /*Calculates the slack for the specific clb pin. Takes the minimum slack. Keeps track of the pin
     * used in this calculation so it can be used again for getting the total path delay*/
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto clb_pin = cluster_ctx.clb_nlist.net_pin(net_id, ipin);

    /*
     *There may be multiple atom netlist pins connected to this CLB pin. Iterate through them all
     * Take the minimum of the atom pin slack as the CLB pin slack
     * minimum slack is used since it is guarantee then to be freed from long path problems
     */
    float clb_min_slack = delay_upper_bound[net_id][ipin];
    for (const AtomPinId pin : netlist_pin_lookup.connected_atom_pins(clb_pin)) {
        if (timing_info->setup_pin_slack(pin) == std::numeric_limits<float>::infinity() && type == SETUP) {
            if (clb_min_slack == delay_upper_bound[net_id][ipin]) {
                atom_pin = pin;
            }
            continue;
        } else if (timing_info->hold_pin_slack(pin) == std::numeric_limits<float>::infinity() && type == HOLD) {
            if (clb_min_slack == delay_upper_bound[net_id][ipin]) {
                if (clb_min_slack == delay_upper_bound[net_id][ipin]) {
                    atom_pin = pin;
                }
            }
            continue;
        } else {
            if (type == HOLD) {
                if (clb_min_slack > timing_info->hold_pin_slack(pin)) {
                    clb_min_slack = timing_info->hold_pin_slack(pin);
                    atom_pin = pin;
                }
            } else {
                if (clb_min_slack > timing_info->setup_pin_slack(pin)) {
                    clb_min_slack = timing_info->setup_pin_slack(pin);
                    atom_pin = pin;
                }
            }
        }
    }

    return clb_min_slack;
}

float route_budgets::get_total_path_delay(std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> timing_analyzer,
                                          analysis_type analysis_type,
                                          tatum::NodeId timing_node) {
    /*The total path delay through a connection is calculated using the arrival and required time
     * Arrival time describes how long it took to arrive at this node and thus is the value for the
     * delay before this node. The required time describes the time it should arrive this node.
     * To get the future path delay, take the required time at the sink of the longest/shortest path
     * and subtract the required time of the current node from it. The combination of the past
     * and future path delays is the total path delay through this connection. Returns a value
     * of -1 if no total path is found*/

    auto arrival_tags = timing_analyzer->setup_tags(timing_node, tatum::TagType::DATA_ARRIVAL);
    auto required_tags = timing_analyzer->setup_tags(timing_node, tatum::TagType::DATA_REQUIRED);

    if (analysis_type == HOLD) {
        arrival_tags = timing_analyzer->hold_tags(timing_node, tatum::TagType::DATA_ARRIVAL);
        required_tags = timing_analyzer->hold_tags(timing_node, tatum::TagType::DATA_REQUIRED);
    }

    /*Check if valid*/
    if (arrival_tags.empty() || required_tags.empty()) {
        return -1;
    }

    /*To get the maximum path delay, we need
     * maximum arrival tag + (maximum sink required time - minimum current required time)*/

    auto max_arrival_tag_iter = find_maximum_tag(arrival_tags);
    auto min_required_tag_iter = find_minimum_tag(required_tags);

    auto& timing_ctx = g_vpr_ctx.timing();
    /*If its already a sink node, then the total path is the arrival time*/
    if (timing_ctx.graph->node_type(timing_node) == tatum::NodeType::SINK) {
        return max_arrival_tag_iter->time().value();
    }

    tatum::NodeId sink_node = min_required_tag_iter->origin_node();
    if (sink_node == tatum::NodeId::INVALID()) {
        return -1;
    }

    auto sink_node_tags = timing_analyzer->setup_tags(sink_node, tatum::TagType::DATA_REQUIRED);

    if (analysis_type == HOLD) {
        sink_node_tags = timing_analyzer->hold_tags(sink_node, tatum::TagType::DATA_REQUIRED);
    }

    if (sink_node_tags.empty()) {
        return -1;
    }

    auto max_sink_node_tag_iter = find_maximum_tag(sink_node_tags);

    if (min_required_tag_iter != required_tags.end() && max_arrival_tag_iter != arrival_tags.end()
        && min_required_tag_iter != sink_node_tags.end()) {
        float final_required_time = max_sink_node_tag_iter->time().value();

        float future_path_delay = final_required_time - min_required_tag_iter->time().value();
        float past_path_delay = max_arrival_tag_iter->time().value();

        return past_path_delay + future_path_delay;
    } else {
        return -1;
    }
}

void route_budgets::allocate_slack_using_delays_and_criticalities(vtr::vector<ClusterNetId, float*>& net_delay,
                                                                  std::shared_ptr<SetupTimingInfo> timing_info,
                                                                  const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                                  t_router_opts router_opts) {
    /*Simplifies the budget calculation. The pin criticality describes 1-slack ratio
     * which is deemed a valid way to arrive at the delay budget for a connection. Thus
     * the maximum delay budget = delay through this connection / pin criticality.
     * The minimum delay budget is set to 0 to promote finding the fastest path*/

    auto& cluster_ctx = g_vpr_ctx.clustering();
    float pin_criticality;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            pin_criticality = calculate_clb_net_pin_criticality(*timing_info, netlist_pin_lookup, pin_id);

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

            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            delay_min_budget[net_id][ipin] = 0;
            delay_lower_bound[net_id][ipin] = 0;
            delay_upper_bound[net_id][ipin] = 100e-9;

            if (pin_criticality == 0) {
                //prevent invalid division
                delay_max_budget[net_id][ipin] = delay_upper_bound[net_id][ipin];
            } else {
                delay_max_budget[net_id][ipin] = min(net_delay[net_id][ipin] / pin_criticality, delay_upper_bound[net_id][ipin]);
            }
            check_if_budgets_in_bounds(net_id, pin_id);
            /*Use RCV algorithm for delay target
             * Tend towards minimum to consider short path timing delay more*/
            delay_target[net_id][ipin] = min(0.5 * (delay_min_budget[net_id][ipin] + delay_max_budget[net_id][ipin]), delay_min_budget[net_id][ipin] + 0.1e-9);
        }
    }
}

void route_budgets::check_if_budgets_in_bounds(ClusterNetId net_id, ClusterPinId pin_id) {
    /*All budgets need to be between the minimum and maximum bound*/

    auto& cluster_ctx = g_vpr_ctx.clustering();
    int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

    VTR_ASSERT_MSG(delay_max_budget[net_id][ipin] >= delay_min_budget[net_id][ipin]
                       && delay_lower_bound[net_id][ipin] <= delay_min_budget[net_id][ipin]
                       && delay_upper_bound[net_id][ipin] >= delay_max_budget[net_id][ipin]
                       && delay_upper_bound[net_id][ipin] >= delay_lower_bound[net_id][ipin],
                   "Delay budgets do not fit in delay bounds");
}

void route_budgets::check_if_budgets_in_bounds() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            check_if_budgets_in_bounds(net_id, pin_id);
        }
    }
}

std::shared_ptr<SetupHoldTimingInfo> route_budgets::perform_sta(vtr::vector<ClusterNetId, float*>& temp_budgets) {
    auto& atom_ctx = g_vpr_ctx.atom();
    /*Perform static timing analysis to get the delay and path weights for slack allocation*/
    std::shared_ptr<RoutingDelayCalculator> routing_delay_calc = std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, temp_budgets);

    std::shared_ptr<SetupHoldTimingInfo> timing_info = make_setup_hold_timing_info(routing_delay_calc);
    /*Unconstrained nodes should be warned in the main routing function, do not report it here*/
    timing_info->set_warn_unconstrained(false);
    timing_info->update();

    return timing_info;
}

void route_budgets::update_congestion_times(ClusterNetId net_id) {
    /*Calling this function indicates this net is congested in
     * this routing iteration. This vector keeps the number of
     * consecutive times this net is congested */
    num_times_congested[net_id]++;
}

void route_budgets::not_congested_this_iteration(ClusterNetId net_id) {
    /*Any time the net is not congested, clear this counter to only
     * count the /consecutive/ congested times*/
    num_times_congested[net_id] = 0;
}

void route_budgets::lower_budgets(float delay_decrement) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    /*Decrease the budgets by a delay increment when the congested times is high enough*/
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (num_times_congested[net_id] >= 3) {
            for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
                if (delay_min_budget[net_id][ipin] - delay_lower_bound[net_id][ipin] >= 1e-9) {
                    delay_min_budget[net_id][ipin] = delay_min_budget[net_id][ipin] - delay_decrement;
                    keep_budget_in_bounds(delay_min_budget, net_id, pin_id);
                }
            }
        }
    }
}

/*Getter functions*/
float route_budgets::get_delay_target(ClusterNetId net_id, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);

    return delay_target[net_id][ipin];
}

float route_budgets::get_min_delay_budget(ClusterNetId net_id, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);

    return delay_min_budget[net_id][ipin];
}

float route_budgets::get_max_delay_budget(ClusterNetId net_id, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);

    return delay_max_budget[net_id][ipin];
}

float route_budgets::get_crit_short_path(ClusterNetId net_id, int ipin) {
    //cannot get delay from a source
    VTR_ASSERT(ipin);
    if (delay_target[net_id][ipin] == 0) {
        return 0;
    }
    return pow(((delay_target[net_id][ipin] - delay_lower_bound[net_id][ipin]) / delay_target[net_id][ipin]), SHORT_PATH_EXP);
}

void route_budgets::print_route_budget() {
    /*Used for debugging. Prints out all the delay budget class variables to an external
     * file named route_budgets.txt*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    fstream fp;
    fp.open("route_budget.txt", fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                  "could not open \"route_budget.txt\" for generating route budget file\n");
    }

    fp << "Minimum Delay Budgets:" << endl;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        fp << endl
           << "Net: " << size_t(net_id) << "            ";
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            fp << delay_min_budget[net_id][ipin] << " ";
        }
    }

    fp << endl
       << endl
       << "Maximum Delay Budgets:" << endl;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        fp << endl
           << "Net: " << size_t(net_id) << "            ";
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            fp << delay_max_budget[net_id][ipin] << " ";
        }
    }

    fp << endl
       << endl
       << "Target Delay Budgets:" << endl;

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        fp << endl
           << "Net: " << size_t(net_id) << "            ";
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            fp << delay_target[net_id][ipin] << " ";
        }
    }

    fp << endl
       << endl
       << "Delay lower_bound:" << endl;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        fp << endl
           << "Net: " << size_t(net_id) << "            ";
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            fp << delay_lower_bound[net_id][ipin] << " ";
        }
    }

    fp << endl
       << endl
       << "Delay upper_bound:" << endl;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        fp << endl
           << "Net: " << size_t(net_id) << "            ";
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            fp << delay_upper_bound[net_id][ipin] << " ";
        }
    }

    fp.close();
}

void route_budgets::print_temporary_budgets_to_file(vtr::vector<ClusterNetId, float*>& temp_budgets) {
    /*Used for debugging. Print one specific budget to an external file called
     * temporary_budgets.txt. This can be used to see how the budgets change between
     * each minimax PERT iteration*/
    auto& cluster_ctx = g_vpr_ctx.clustering();
    fstream fp;
    fp.open("temporary_budgets.txt", fstream::out | fstream::trunc);

    fp << "Temporary Budgets:" << endl;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        fp << endl
           << "Net: " << size_t(net_id) << "            ";
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            fp << temp_budgets[net_id][ipin] << " ";
        }
    }
}

bool route_budgets::if_set() const {
    /*Returns if the budgets have been loaded yet*/
    return set;
}
