//
// Created by shrevena on 08/06/24.
//

#include <sstream>
#include "lookahead_profiler.h"
#include "vtr_error.h"
#include "vtr_log.h"
#include "globals.h"
#include "router_lookahead_map_utils.h"
#include "vpr_utils.h"
#include "re_cluster_util.h"

LookaheadProfiler::LookaheadProfiler() {
    lookahead_verifier_csv.open("lookahead_verifier_info.csv", std::ios::out);

    if (!lookahead_verifier_csv.is_open()) {
        VTR_LOG_ERROR("Could not open lookahead_verifier_info.csv", "error");
    }

    lookahead_verifier_csv
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
        << std::endl;
}

void LookaheadProfiler::record(int iteration,
                               int target_net_pin_index,
                               const t_conn_cost_params& cost_params,
                               const RouterLookahead& router_lookahead,
                               const ParentNetId& net_id,
                               const Netlist<>& net_list,
                               std::vector<RRNodeId> branch_inodes) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    if (iteration < 1)
        return;

    for (size_t i = 2; i < branch_inodes.size(); ++i) { /* Distance one node away is always 0.0 (IPIN->SINK) */
        RRNodeId source_inode = branch_inodes.back();
        RRNodeId sink_inode = branch_inodes.front();
        RRNodeId curr_inode = branch_inodes[i];

        float total_backward_cost = route_ctx.rr_node_route_inf[sink_inode].backward_path_cost;
        float total_backward_delay = route_ctx.rr_node_route_inf[sink_inode].backward_path_delay;
        float total_backward_congestion = route_ctx.rr_node_route_inf[sink_inode].backward_path_congestion;

        auto current_node = route_ctx.rr_node_route_inf[curr_inode];
        float current_backward_cost = current_node.backward_path_cost;
        float current_backward_delay = current_node.backward_path_delay;
        float current_backward_congestion = current_node.backward_path_congestion;

        auto [from_x, from_y] = util::get_adjusted_rr_position(curr_inode);
        auto [to_x, to_y] = util::get_adjusted_rr_position(sink_inode);

        int delta_x = to_x - from_x;
        int delta_y = to_y - from_y;

        float djikstra_cost = total_backward_cost - current_backward_cost;
        float djikstra_delay = total_backward_delay - current_backward_delay;
        float djikstra_congestion = total_backward_congestion - current_backward_congestion;

        float lookahead_cost = router_lookahead.get_expected_cost(curr_inode, sink_inode, cost_params, 0.0);
        auto [lookahead_delay, lookahead_congestion] = router_lookahead.get_expected_delay_and_cong(curr_inode, sink_inode, cost_params, 0.0);

        if (atom_block_names.find(sink_inode) == atom_block_names.end()) {
            if (net_id != ParentNetId::INVALID() && target_net_pin_index != OPEN) {
                atom_block_names[sink_inode] = net_list.block_name(net_list.net_pin_block(net_id, target_net_pin_index));

                AtomBlockId atom_block_id = g_vpr_ctx.atom().nlist.find_block(atom_block_names[sink_inode]);
                atom_block_models[sink_inode] = g_vpr_ctx.atom().nlist.block_model(atom_block_id)->name;

                ClusterBlockId cluster_block_id = atom_to_cluster(atom_block_id);

                cluster_block_types[sink_inode] = g_vpr_ctx.clustering().clb_nlist.block_type(cluster_block_id)->name;

                auto tile_type = physical_tile_type(cluster_block_id);
                tile_dimensions[sink_inode] = std::pair(std::to_string(tile_type->width), std::to_string(tile_type->height));
            } else {
                atom_block_names[sink_inode] = "--";
                atom_block_models[sink_inode] = "--";
                cluster_block_types[sink_inode] = "--";
                tile_dimensions[sink_inode] = {"--", "--"};
            }
        }

        VTR_ASSERT_SAFE(atom_block_names.find(sink_inode) != atom_block_names.end());
        VTR_ASSERT_SAFE(atom_block_models.find(sink_inode) != atom_block_models.end());
        VTR_ASSERT_SAFE(cluster_block_types.find(sink_inode) != cluster_block_types.end());
        VTR_ASSERT_SAFE(tile_dimensions.find(sink_inode) != tile_dimensions.end());

        std::string block_name = atom_block_names[sink_inode];
        std::string atom_block_model = atom_block_models[sink_inode];
        std::string cluster_block_type = cluster_block_types[sink_inode];
        auto [tile_width, tile_height] = tile_dimensions[sink_inode];

        std::string node_type_str = rr_graph.node_type_string(curr_inode);
        std::string node_length = (node_type_str == "CHANX" || node_type_str == "CHANX")
                                      ? std::to_string(rr_graph.node_length(curr_inode))
                                      : "--";

        lookahead_verifier_csv << iteration << ",";            // iteration no.
        lookahead_verifier_csv << source_inode << ",";         // source node
        lookahead_verifier_csv << sink_inode << ",";           // sink node
        lookahead_verifier_csv << block_name << ",";           // sink block name
        lookahead_verifier_csv << atom_block_model << ",";     // sink atom block model
        lookahead_verifier_csv << cluster_block_type << ",";   // sink cluster block type
        lookahead_verifier_csv << tile_height << ",";          // sink cluster tile height
        lookahead_verifier_csv << tile_width << ",";           // sink cluster tile width
        lookahead_verifier_csv << curr_inode << ",";           // current node
        lookahead_verifier_csv << node_type_str << ",";        // node type
        lookahead_verifier_csv << node_length << ",";          // node length
        lookahead_verifier_csv << i << ",";                    // num. nodes from sink
        lookahead_verifier_csv << delta_x << ",";              // delta x
        lookahead_verifier_csv << delta_y << ",";              // delta y
        lookahead_verifier_csv << djikstra_cost << ",";        // actual cost
        lookahead_verifier_csv << djikstra_delay << ",";       // actual delay
        lookahead_verifier_csv << djikstra_congestion << ",";  // actual congestion
        lookahead_verifier_csv << lookahead_cost << ",";       // predicted cost
        lookahead_verifier_csv << lookahead_delay << ",";      // predicted delay
        lookahead_verifier_csv << lookahead_congestion << ","; // predicted congestion
        lookahead_verifier_csv << cost_params.criticality;     // criticality
        lookahead_verifier_csv << std::endl;
    }
}