#include "cluster_util.h"

#include "cluster_router.h"
#include "cluster_placement.h"

//calculate the initial timing at the start of packing stage
void calc_init_packing_timing (const t_packer_opts& packer_opts, 
							   const t_analysis_opts& analysis_opts,
							   const std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode, 
							   std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc, 
							   std::shared_ptr<SetupTimingInfo>& timing_info, 
							   vtr::vector<AtomBlockId, float>& atom_criticality) {

    auto& atom_ctx = g_vpr_ctx.atom();

    /*
     * Initialize the timing analyzer
     */
    clustering_delay_calc = std::make_shared<PreClusterDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, packer_opts.inter_cluster_net_delay, expected_lowest_cost_pb_gnode);
    timing_info = make_setup_timing_info(clustering_delay_calc, packer_opts.timing_update_type);

    //Calculate the initial timing
    timing_info->update();

    if (isEchoFileEnabled(E_ECHO_PRE_PACKING_TIMING_GRAPH)) {
        auto& timing_ctx = g_vpr_ctx.timing();
        tatum::write_echo(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH),
                          *timing_ctx.graph, *timing_ctx.constraints, *clustering_delay_calc, timing_info->analyzer());

        tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);
        write_setup_timing_graph_dot(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH) + std::string(".dot"),
                                     *timing_info, debug_tnode);
    }

    {
        auto& timing_ctx = g_vpr_ctx.timing();
        PreClusterTimingGraphResolver resolver(atom_ctx.nlist,
                                               atom_ctx.lookup, *timing_ctx.graph, *clustering_delay_calc);
        resolver.set_detail_level(analysis_opts.timing_report_detail);

        tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph,
                                              *timing_ctx.constraints);

        timing_reporter.report_timing_setup(
            "pre_pack.report_timing.setup.rpt",
            *timing_info->setup_analyzer(),
            analysis_opts.timing_report_npaths);
    }

    //Calculate true criticalities of each block
    for (AtomBlockId blk : atom_ctx.nlist.blocks()) {
        for (AtomPinId in_pin : atom_ctx.nlist.block_input_pins(blk)) {
            //Max criticality over incoming nets
            float crit = timing_info->setup_pin_criticality(in_pin);
            atom_criticality[blk] = std::max(atom_criticality[blk], crit);
        }
    }

}

//Free the clustering data structures
void free_clustering_data(const t_packer_opts& packer_opts, 
						  vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing, 
						  int* hill_climbing_inputs_avail, 
						  t_cluster_placement_stats* cluster_placement_stats, 
						  t_molecule_link* unclustered_list_head, 
						  t_molecule_link* memory_pool, 
						  t_pb_graph_node** primitives_list) {

	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        free_intra_lb_nets(intra_lb_routing[blk_id]);

    intra_lb_routing.clear();

    if (packer_opts.hill_climbing_flag)
        free(hill_climbing_inputs_avail);

    free_cluster_placement_stats(cluster_placement_stats);

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        cluster_ctx.clb_nlist.remove_block(blk_id);

    cluster_ctx.clb_nlist = ClusteredNetlist();

    free(unclustered_list_head);
    free(memory_pool);

    free(primitives_list);
}