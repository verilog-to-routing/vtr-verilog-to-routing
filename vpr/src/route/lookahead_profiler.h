//
// Created by shrevena on 08/06/24.
//

#ifndef VTR_LOOKAHEAD_PROFILER_H
#define VTR_LOOKAHEAD_PROFILER_H

#include <fstream>
#include <thread>
#include "rr_graph_fwd.h"
#include "connection_router_interface.h"
#include "router_lookahead.h"

class LookaheadProfiler {
  public:
    LookaheadProfiler();

    void record(int iteration, int target_net_pin_index, const t_conn_cost_params& cost_params, const RouterLookahead& router_lookahead, const ParentNetId& net_id, const Netlist<>& net_list, std::vector<RRNodeId> branch_inodes);

  private:
    std::ofstream lookahead_verifier_csv;
    std::unordered_map<RRNodeId, std::string> atom_block_names;
    std::unordered_map<RRNodeId, std::string> atom_block_models;
    std::unordered_map<RRNodeId, std::string> cluster_block_types;
    std::unordered_map<RRNodeId, std::pair<std::string, std::string>> tile_dimensions;
};

#endif //VTR_LOOKAHEAD_PROFILER_H