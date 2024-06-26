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

    void record(int iteration,
                int target_net_pin_index,
                RRNodeId source_inode,
                RRNodeId sink_inode,
                RRNodeId curr_inode,
                size_t nodes_from_sink,
                const t_conn_cost_params& cost_params,
                const RouterLookahead& router_lookahead,
                const ParentNetId& net_id,
                const Netlist<>& net_list);

  private:
    std::ofstream lookahead_verifier_csv;
};

extern LookaheadProfiler lookahead_profiler;

#endif //VTR_LOOKAHEAD_PROFILER_H