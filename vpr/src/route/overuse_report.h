#pragma once

#include "rr_graph_storage.h"
#include <map>

void log_overused_nodes_status(int max_logged_overused_rr_nodes);
void report_overused_nodes();
void generate_overused_nodes_to_congested_net_lookup(std::map<RRNodeId, std::set<ClusterNetId>>& nodes_to_nets_lookup);