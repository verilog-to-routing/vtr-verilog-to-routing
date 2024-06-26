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

LookaheadProfiler lookahead_profiler = LookaheadProfiler();

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

void LookaheadProfiler::record(const int iteration,
                               const int target_net_pin_index,
                               const RRNodeId source_inode,
                               const RRNodeId sink_inode,
                               const RRNodeId curr_inode,
                               const size_t nodes_from_sink,
                               const t_conn_cost_params& cost_params,
                               const RouterLookahead& router_lookahead,
                               const ParentNetId& net_id,
                               const Netlist<>& net_list) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

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

    float lookahead_cost = router_lookahead.get_expected_cost(curr_inode, sink_inode, cost_params,
                                                              0.0);
    float lookahead_delay, lookahead_congestion;
    std::tie(lookahead_delay, lookahead_congestion) = router_lookahead.get_expected_delay_and_cong(curr_inode, sink_inode, cost_params, 0.0, true);

    std::string block_name = "--";
    std::string atom_block_model = "--";
    std::string cluster_block_type = "--";
    std::string tile_height = "--";
    std::string tile_width = "--";

    if (net_id != ParentNetId::INVALID() && target_net_pin_index != OPEN) {
        block_name = net_list.block_name(net_list.net_pin_block(net_id, target_net_pin_index));

        AtomBlockId atom_block_id = g_vpr_ctx.atom().nlist.find_block(block_name);
        atom_block_model = g_vpr_ctx.atom().nlist.block_model(atom_block_id)->name;

        ClusterBlockId cluster_block_id = atom_to_cluster(atom_block_id);

        cluster_block_type = g_vpr_ctx.clustering().clb_nlist.block_type(cluster_block_id)->name;

        auto tile_type = physical_tile_type(cluster_block_id);
        tile_height = std::to_string(tile_type->height);
        tile_width = std::to_string(tile_type->width);
    }

    auto node_type = rr_graph.node_type(curr_inode);
    std::string node_type_str;
    std::string node_length;

    switch (node_type) {
        case SOURCE:
            node_type_str = "SOURCE";
            node_length = "--";
            break;
        case SINK:
            node_type_str = "SINK";
            node_length = "--";
            break;
        case IPIN:
            node_type_str = "IPIN";
            node_length = "--";
            break;
        case OPIN:
            node_type_str = "OPIN";
            node_length = "--";
            break;
        case CHANX:
            node_type_str = "CHANX";
            node_length = std::to_string(rr_graph.node_length(curr_inode));
            break;
        case CHANY:
            node_type_str = "CHANY";
            node_length = std::to_string(rr_graph.node_length(curr_inode));
            break;
        default:
            node_type_str = "--";
            node_length = "--";
            break;
    }

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
    lookahead_verifier_csv << nodes_from_sink << ",";      // num. nodes from sink
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