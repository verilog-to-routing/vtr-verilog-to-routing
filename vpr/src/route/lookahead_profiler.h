#ifndef VTR_LOOKAHEAD_PROFILER_H
#define VTR_LOOKAHEAD_PROFILER_H

#include <fstream>

#include "connection_router_interface.h"
#include "router_lookahead.h"
#include "rr_graph_fwd.h"

/**
 * @brief A class which records information used to profile the router lookahead: most importantly,
 * the actual cost (delay and congestion) from nodes to the sink to which they have been routed, as
 * well as the lookahead's estimation of this cost.
 */
class LookaheadProfiler {
  public:
    LookaheadProfiler()
        : is_empty_(true)
        , toggle_warn_(true) {}

    LookaheadProfiler(const LookaheadProfiler&) = delete;
    LookaheadProfiler& operator=(const LookaheadProfiler&) = delete;

    /**
     * @brief Record information on nodes on a path from a source to a sink.
     *
     * @param iteration The router iteration.
     * @param target_net_pin_index Target pin of this sink in the net.
     * @param cost_params
     * @param router_lookahead
     * @param net_id
     * @param net_list
     * @param branch_inodes A backwards path of nodes, starting at a sink.
     *
     * @warning
     * branch_inodes must be a backwards path, starting at a sink node.
     */
    void record(int iteration,
                int target_net_pin_index,
                const t_conn_cost_params& cost_params,
                const RouterLookahead& router_lookahead,
                const ParentNetId& net_id,
                const Netlist<>& net_list,
                const std::vector<RRNodeId>& branch_inodes);

  private:
    ///@brief The output filestream.
    std::ofstream lookahead_verifier_csv_;
    ///@brief Whether the output file is empty/not yet opened.
    bool is_empty_;
    ///@brief Whether to creat a warning if the output csv is not open. Used to avoid repeated warnings.
    bool toggle_warn_;
    ///@brief A map from sink node IDs to the names of their atom blocks.
    std::unordered_map<RRNodeId, std::string> atom_block_names_;
    ///@brief A map from sink node IDs to the names of the models of their atom blocks.
    std::unordered_map<RRNodeId, std::string> atom_block_models_;
    ///@brief A map from sink node IDs to the names of the types of their clusters.
    std::unordered_map<RRNodeId, std::string> cluster_block_types_;
    ///@brief A map from sink node IDs to the dimensions of their tiles (width, height).
    std::unordered_map<RRNodeId, std::pair<std::string, std::string>> tile_dimensions_;
};

#endif //VTR_LOOKAHEAD_PROFILER_H