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

//using tatum::TimingGraph;
//using tatum::NodeId;
//using tatum::NodeType;
//using tatum::EdgeId;



#define SHORT_PATH_EXP 0.5
#define MIN_DELAY_DECREMENT 1e-9

route_budgets::route_budgets() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    num_times_congested.resize(cluster_ctx.clbs_nlist.net.size(), 0);

    set = false;
}

route_budgets::~route_budgets() {

    for (unsigned i = 0; i < delay_min_budget.size(); i++) {
        delay_min_budget[i].clear();
        delay_max_budget[i].clear();
        delay_target[i].clear();
        delay_lower_bound[i].clear();
        delay_upper_bound[i].clear();
    }
    delay_min_budget.clear();
    delay_max_budget.clear();
    delay_target.clear();
    delay_lower_bound.clear();
    delay_upper_bound.clear();

}

void route_budgets::load_route_budgets(float ** net_delay,
        std::shared_ptr<const SetupTimingInfo> timing_info,
        const IntraLbPbPinLookup& pb_gpin_lookup, t_router_opts router_opts) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //set lower bound to 0 and upper bound to net_delay
    delay_min_budget.resize(cluster_ctx.clbs_nlist.net.size());
    delay_target.resize(cluster_ctx.clbs_nlist.net.size());
    delay_max_budget.resize(cluster_ctx.clbs_nlist.net.size());
    delay_lower_bound.resize(cluster_ctx.clbs_nlist.net.size());
    delay_upper_bound.resize(cluster_ctx.clbs_nlist.net.size());
    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        delay_min_budget[inet].resize(cluster_ctx.clbs_nlist.net[inet].pins.size(), 0);
        delay_target[inet].resize(cluster_ctx.clbs_nlist.net[inet].pins.size(), 0);
        delay_max_budget[inet].resize(cluster_ctx.clbs_nlist.net[inet].pins.size(), 0);
        delay_lower_bound[inet].resize(cluster_ctx.clbs_nlist.net[inet].pins.size(), 0);
        delay_upper_bound[inet].resize(cluster_ctx.clbs_nlist.net[inet].pins.size(), 0);
    }

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
            delay_max_budget[inet][ipin] = min(net_delay[inet][ipin] / pin_criticality, delay_upper_bound[inet][ipin]);

            if (!isnan(delay_max_budget[inet][ipin])) {
                VTR_ASSERT_MSG(delay_max_budget[inet][ipin] >= delay_min_budget[inet][ipin]
                        && delay_lower_bound[inet][ipin] <= delay_min_budget[inet][ipin]
                        && delay_upper_bound[inet][ipin] >= delay_max_budget[inet][ipin]
                        && delay_upper_bound[inet][ipin] >= delay_lower_bound[inet][ipin],
                        "Delay budgets do not fit in delay bounds");
            }
        }
    }

    //Use RCV algorithm for delay target
    //Tend towards minimum to consider short path timing delay more

    for (unsigned inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

            delay_target[inet][ipin] = min(0.5 * (delay_min_budget[inet][ipin] + delay_max_budget[inet][ipin]), delay_min_budget[inet][ipin] + 0.1e-9);
        }
    }
    set = true;
}

void route_budgets::allocate_slack() {
    vector<vector<float>> temp_budgets;
    unsigned iteration = 0;
    float max_budget_change = 6e-12;

    while (iteration < 7 || max_budget_change > 5e-12) {
        short_path_sta(temp_budgets);
        keep_budget_in_bounds(MIN, temp_budgets);

        long_path_sta(temp_budgets);
        keep_budget_in_bounds(MAX, temp_budgets);

    }

    iteration = 0;
    max_budget_change = 900e-12;

    while (iteration < 7 || iteration > 3 || max_budget_change > 800e-12) {
        long_path_sta(temp_budgets);
        keep_budget_in_bounds(MIN, temp_budgets);
    }

    delay_max_budget = temp_budgets;

    iteration = 0;
    max_budget_change = 900e-12;

    while (iteration < 7 || iteration > 3 || max_budget_change > 800e-12) {
        short_path_sta(temp_budgets);
        keep_budget_in_bounds(MAX, temp_budgets);
    }


    iteration = 0;
    max_budget_change = 900e-12;

    float bottom_range = -1e-9;
    while (iteration < 3 || max_budget_change > 800e-12) {
        short_path_sta(temp_budgets);
        for (unsigned i = 0; i < temp_budgets.size(); i++) {
            for (unsigned j = 0; i < temp_budgets[i].size(); j++) {
                temp_budgets[i][j] = max(temp_budgets[i][j], bottom_range);

            }
        }
    }

    delay_min_budget = temp_budgets;
}

void route_budgets::keep_budget_in_bounds(max_or_min _type, vector<vector<float>> &temp_budgets) {
    for (unsigned i = 0; i < temp_budgets.size(); i++) {
        for (unsigned j = 0; i < temp_budgets[i].size(); j++) {
            if (_type == MAX) {
                temp_budgets[i][j] = max(temp_budgets[i][j], delay_lower_bound[i][j]);
            }
            if (_type == MIN) {
                temp_budgets[i][j] = min(temp_budgets[i][j], delay_upper_bound[i][j]);
            }
        }
    }

}

void route_budgets::short_path_sta(vector<vector<float>> &temp_budgets) {

}

void route_budgets::long_path_sta(vector<vector<float>> &temp_budgets) {

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
    unsigned inet, ipin;
    fstream fp;
    fp.open("route_budget.txt", fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                "could not open \"route_budget.txt\" for generating route budget file\n");
    }

    fp << "Minimum Delay Budgets:" << endl;
    for (inet = 0; inet < delay_min_budget.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 0; ipin < delay_min_budget[inet].size(); ipin++) {
            fp << delay_min_budget[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Maximum Delay Budgets:" << endl;
    for (inet = 0; inet < delay_max_budget.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 0; ipin < delay_max_budget[inet].size(); ipin++) {
            fp << delay_max_budget[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Target Delay Budgets:" << endl;

    for (inet = 0; inet < delay_target.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 0; ipin < delay_target[inet].size(); ipin++) {
            fp << delay_target[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Delay lower_bound:" << endl;
    for (inet = 0; inet < delay_lower_bound.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 0; ipin < delay_lower_bound[inet].size(); ipin++) {
            fp << delay_lower_bound[inet][ipin] << " ";
        }
    }

    fp << endl << endl << "Target Delay Budgets:" << endl;
    for (inet = 0; inet < delay_upper_bound.size(); inet++) {
        fp << endl << "Net: " << inet << "            ";
        for (ipin = 0; ipin < delay_upper_bound[inet].size(); ipin++) {
            fp << delay_upper_bound[inet][ipin] << " ";
        }
    }

    fp.close();
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
