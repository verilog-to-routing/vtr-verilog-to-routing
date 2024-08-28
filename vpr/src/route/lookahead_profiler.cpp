#include <sstream>

#include "globals.h"
#include "lookahead_profiler.h"
#include "re_cluster_util.h"
#include "router_lookahead_map_utils.h"
#include "vpr_utils.h"
#include "vtr_error.h"
#include "vtr_log.h"

void LookaheadProfiler::set_file_name(const std::string& file_name) {
    enabled_ = !file_name.empty();
    if (!enabled_)
        return;

#ifdef PROFILE_LOOKAHEAD
    lookahead_verifier_csv_.open(file_name, std::ios::out);

    if (!lookahead_verifier_csv_.is_open()) {
        throw vtr::VtrError("Could not open " + file_name, "lookahead_profiler.cpp", 21);
    }

    lookahead_verifier_csv_
        << "iteration no."
        << ",source node"
        << ",sink node"
        << ",sink block name"
        << ",sink atom block model"
        << ",sink cluster block type"
        << ",sink cluster tile height"
        << ",sink cluster tile width"
        << ",current node"
        << ",node type"
        << ",node length"
        << ",num. nodes from sink"
        << ",delta x"
        << ",delta y"
        << ",actual cost"
        << ",actual delay"
        << ",actual congestion"
        << ",predicted cost"
        << ",predicted delay"
        << ",predicted congestion"
        << ",criticality"
        << "\n";
#else
    throw vtr::VtrError("Profiler enabled, but PROFILE_LOOKAHEAD not defined.", "lookahead_profiler.cpp", 47);
#endif
}

void LookaheadProfiler::record(int iteration,
                               int target_net_pin_index,
                               const t_conn_cost_params& cost_params,
                               const RouterLookahead& router_lookahead,
                               const ParentNetId& net_id,
                               const Netlist<>& net_list,
                               const std::vector<RRNodeId>& branch_inodes) {
    /**
     * This function records data into the output file created in set_file_name(). If that file name was
     * an empty string, this function returns. If the output file is not open, VPR will throw an error.
     *
     * First, we get the RRNodeIds of the sink and source nodes (assumed to be the beginning and end of
     * branch_inodes, respectively.
     *
     * Next, using net_id, net_list, and target_net_pin_index, we obtain the name of the atom block,
     * the name of the type of atom block, the name of the type of cluster block, and the tile dimensions
     * where the sink node is located.
     *
     * We then iterate through all other nodes in branch_inodes and calculate the expected and actual cost,
     * congestion, and delay from the current node to the sink node, as well as the current node type and
     * length, and the xy-deltas to the sink node.
     *
     * Finally, all this information is written to the output .csv file.
     */

    if (!enabled_)
        return;

#ifdef PROFILE_LOOKAHEAD
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    if (!lookahead_verifier_csv_.is_open()) {
        throw vtr::VtrError("Output file is not open.", "lookahead_profiler.cpp", 67);
    }

    // The default value in RouteTree::update_from_heap() is -1; only calls which descend from route_net()
    // pass in an iteration value, which is the only context in which we want to profile.
    if (iteration < 1)
        return;

    RRNodeId source_inode = branch_inodes.back(); // Not necessarily a SOURCE node.
    RRNodeId sink_inode = branch_inodes.front();

    VTR_ASSERT(rr_graph.node_type(sink_inode) == SINK);
    VTR_ASSERT(net_id != ParentNetId::INVALID());
    VTR_ASSERT(target_net_pin_index != OPEN);

    /* Get sink node attributes (atom block name, atom block model, cluster type, tile dimensions) */

    if (net_pin_blocks_.find(sink_inode) == net_pin_blocks_.end()) {
        net_pin_blocks_[sink_inode] = net_list.net_pin_block(net_id, target_net_pin_index);

        AtomBlockId atom_block_id = g_vpr_ctx.atom().nlist.find_block(net_list.block_name(net_pin_blocks_[sink_inode]));
        sink_atom_block_[sink_inode] = g_vpr_ctx.atom().nlist.block_model(atom_block_id);

        ClusterBlockId cluster_block_id = atom_to_cluster(atom_block_id);
        sink_cluster_block_[sink_inode] = g_vpr_ctx.clustering().clb_nlist.block_type(cluster_block_id);

        tile_types_[sink_inode] = physical_tile_type(atom_block_id);
    }

    VTR_ASSERT_SAFE(sink_atom_block_.find(sink_inode) != sink_atom_block_.end());
    VTR_ASSERT_SAFE(sink_cluster_block_.find(sink_inode) != sink_cluster_block_.end());
    VTR_ASSERT_SAFE(tile_types_.find(sink_inode) != tile_types_.end());

    const std::string& block_name = net_list.block_name(net_pin_blocks_[sink_inode]);
    const std::string atom_block_model = sink_atom_block_[sink_inode]->name;
    const std::string cluster_block_type = sink_cluster_block_[sink_inode]->name;
    const std::string tile_width = std::to_string(tile_types_[sink_inode]->width);
    const std::string tile_height = std::to_string(tile_types_[sink_inode]->height);

    /* Iterate through the given path and record information for each node */
    for (size_t nodes_from_sink = 2; nodes_from_sink < branch_inodes.size(); ++nodes_from_sink) { // Distance one node away is always 0. (IPIN->SINK)
        RRNodeId curr_inode = branch_inodes[nodes_from_sink];

        // Calculate the actual cost, delay, and congestion from the current node to the sink.
        const t_rr_node_route_inf& curr_node_info = route_ctx.rr_node_route_inf[curr_inode];
        const t_rr_node_route_inf& sink_node_info = route_ctx.rr_node_route_inf[sink_inode];

        float actual_cost = sink_node_info.backward_path_cost - curr_node_info.backward_path_cost;
        float actual_delay = sink_node_info.backward_path_delay - curr_node_info.backward_path_delay;
        float actual_congestion = sink_node_info.backward_path_congestion - curr_node_info.backward_path_congestion;

        // Get the cost, delay, and congestion estimates made by the lookahead.
        // Note: lookahead_cost = lookahead_delay * criticality + lookahead_congestion * (1. - criticality)
        float lookahead_cost = router_lookahead.get_expected_cost(curr_inode, sink_inode, cost_params, 0.0);
        auto [lookahead_delay, lookahead_congestion] = router_lookahead.get_expected_delay_and_cong_ignore_criticality(curr_inode, sink_inode, cost_params, 0.0);

        // Get the difference in the coordinates in the current and sink nodes.
        // Note: we are not using util::get_xy_deltas() because this always gives positive values
        // unless the current node is the source node. Using util::get_adjusted_rr_position therefore
        // gives us more information.
        auto [from_x, from_y] = util::get_adjusted_rr_position(curr_inode);
        auto [to_x, to_y] = util::get_adjusted_rr_position(sink_inode);

        int delta_x = to_x - from_x;
        int delta_y = to_y - from_y;

        // Get the current node's type and length
        std::string node_type_str = rr_graph.node_type_string(curr_inode);
        std::string node_length = (node_type_str == "CHANX" || node_type_str == "CHANY")
                                      ? std::to_string(rr_graph.node_length(curr_inode))
                                      : "--";

        /* Write out all info */

        lookahead_verifier_csv_ << iteration << ",";            // iteration no.
        lookahead_verifier_csv_ << source_inode << ",";         // source node
        lookahead_verifier_csv_ << sink_inode << ",";           // sink node
        lookahead_verifier_csv_ << block_name << ",";           // sink block name
        lookahead_verifier_csv_ << atom_block_model << ",";     // sink atom block model
        lookahead_verifier_csv_ << cluster_block_type << ",";   // sink cluster block type
        lookahead_verifier_csv_ << tile_height << ",";          // sink cluster tile height
        lookahead_verifier_csv_ << tile_width << ",";           // sink cluster tile width
        lookahead_verifier_csv_ << curr_inode << ",";           // current node
        lookahead_verifier_csv_ << node_type_str << ",";        // node type
        lookahead_verifier_csv_ << node_length << ",";          // node length
        lookahead_verifier_csv_ << nodes_from_sink << ",";      // num. nodes from sink
        lookahead_verifier_csv_ << delta_x << ",";              // delta x
        lookahead_verifier_csv_ << delta_y << ",";              // delta y
        lookahead_verifier_csv_ << actual_cost << ",";          // actual cost
        lookahead_verifier_csv_ << actual_delay << ",";         // actual delay
        lookahead_verifier_csv_ << actual_congestion << ",";    // actual congestion
        lookahead_verifier_csv_ << lookahead_cost << ",";       // predicted cost
        lookahead_verifier_csv_ << lookahead_delay << ",";      // predicted delay
        lookahead_verifier_csv_ << lookahead_congestion << ","; // predicted congestion
        lookahead_verifier_csv_ << cost_params.criticality;     // criticality
        lookahead_verifier_csv_ << "\n";
    }
#else
    throw vtr::VtrError("Profiler enabled, but PROFILE_LOOKAHEAD not defined.", "lookahead_profiler.cpp", 183);
    (void)iteration;
    (void)target_net_pin_index;
    (void)cost_params;
    (void)router_lookahead;
    (void)net_id;
    (void)net_list;
    (void)branch_inodes;
#endif
}

void LookaheadProfiler::clear() {
    vtr::release_memory(net_pin_blocks_);
    vtr::release_memory(sink_atom_block_);
    vtr::release_memory(sink_cluster_block_);
    vtr::release_memory(tile_types_);
}