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
        : is_empty_(true) {}

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
    ///@brief A map from sink node IDs to their atom blocks' IDs.
    std::unordered_map<RRNodeId, ParentBlockId> net_pin_blocks_;
    ///@brief A map from sink node IDs to a pointer to the model of their atom blocks.
    std::unordered_map<RRNodeId, const t_model*> sink_atom_block_;
    ///@brief A map from sink node IDs to a pointer to their cluster blocks.
    std::unordered_map<RRNodeId, t_logical_block_type_ptr> sink_cluster_block_;
    ///@brief A map from sink node IDs to a pointer to their tiles' types.
    std::unordered_map<RRNodeId, t_physical_tile_type_ptr> tile_types_;
};

#endif //VTR_LOOKAHEAD_PROFILER_H