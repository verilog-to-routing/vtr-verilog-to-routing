#pragma once

#include "rr_graph_storage.h"
#include "rr_graph_view.h"
#include "clustered_netlist_fwd.h"
#include <map>
#include <set>

/**
 * @brief Global routines related to displaying RR node overuse info.
 *
 * This file contains all the routines that print out the information on overused RR nodes
 * and congested nets. The main purpose of these routines is to aid the debugging process
 * should the VPR fail to implement the circuit. Functionalities that resolve these circuit
 * issues should NOT be included here or in overuse_report.cpp
 *
 * An RR node is overused when the number of nets passing through it exceed the node's
 * routing net capacity. A successfully routed circuit is void of these overused nodes.
 *
 * All the nets passing through an overused RR node are flagged as congested nets.
 */

///@brief Print out RR node overuse info in the VPR logfile.
void log_overused_nodes_status(int max_logged_overused_rr_nodes);

///@brief Print out RR node overuse info in a post-VPR report file.
void report_overused_nodes(const RRGraphView& rr_graph);

///@brief Generate a overused RR nodes to congested nets lookup table.
void generate_overused_nodes_to_congested_net_lookup(std::map<RRNodeId, std::set<ClusterNetId>>& nodes_to_nets_lookup);
