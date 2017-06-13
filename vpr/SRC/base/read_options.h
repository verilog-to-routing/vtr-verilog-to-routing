#ifndef READ_OPTIONS_H
#define READ_OPTIONS_H

#include "vpr_types.h"

enum e_echo_files {
	E_ECHO_INITIAL_CLB_PLACEMENT = 0,
	E_ECHO_INITIAL_PLACEMENT_SLACK,
	E_ECHO_INITIAL_PLACEMENT_CRITICALITY,
	E_ECHO_END_CLB_PLACEMENT,
	E_ECHO_PLACEMENT_SINK_DELAYS,
	E_ECHO_FINAL_PLACEMENT_SLACK,
	E_ECHO_FINAL_PLACEMENT_CRITICALITY,
    E_ECHO_PLACEMENT_CRIT_PATH,
	E_ECHO_PLACEMENT_DELTA_DELAY_MODEL,
	E_ECHO_PB_GRAPH,
	E_ECHO_LB_TYPE_RR_GRAPH,
	E_ECHO_ARCH,
	E_ECHO_PLACEMENT_CRITICAL_PATH,
	E_ECHO_PLACEMENT_LOWER_BOUND_SINK_DELAYS,
	E_ECHO_PLACEMENT_LOGIC_SINK_DELAYS,
	E_ECHO_ROUTING_SINK_DELAYS,
	E_ECHO_POST_PACK_NETLIST,
	E_ECHO_BLIF_INPUT,
	E_ECHO_COMPRESSED_NETLIST,
	E_ECHO_NET_DELAY,
	E_ECHO_CLUSTERING_TIMING_INFO,
	E_ECHO_PRE_PACKING_SLACK,
	E_ECHO_PRE_PACKING_CRITICALITY,
	E_ECHO_CLUSTERING_BLOCK_CRITICALITIES,
	E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS,
	E_ECHO_MEM,
	E_ECHO_RR_GRAPH,
	E_ECHO_TIMING_CONSTRAINTS,
	E_ECHO_CRITICAL_PATH,
	E_ECHO_SLACK,
	E_ECHO_COMPLETE_NET_TRACE,
	E_ECHO_SEG_DETAILS,
	E_ECHO_CHAN_DETAILS,
	E_ECHO_SBLOCK_PATTERN,
    E_ECHO_ENDPOINT_TIMING,

    //Timing Graphs
	E_ECHO_PRE_PACKING_TIMING_GRAPH,
	E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH,
	E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH,
    E_ECHO_FINAL_ROUTING_TIMING_GRAPH,
	E_ECHO_ANALYSIS_TIMING_GRAPH,


	E_ECHO_END_TOKEN
};


enum e_output_files {
	E_CRIT_PATH_FILE,
	E_SLACK_FILE,
	E_CRITICALITY_FILE,
	E_FILE_END_TOKEN
};

struct t_options {
	/* File names */
	char *ArchFile;
	char *CircuitName;
	char *NetFile;
	char *PlaceFile;
	char *RouteFile;
	char *BlifFile;
	char *ActFile;
	char *PowerFile;
	char *CmosTechFile;
	char *out_file_prefix;
	char *SDCFile;
	char *pad_loc_file;

    /* Stage Options */
    bool do_packing;
    bool do_placement;
    bool do_routing;
    bool do_analysis;
    bool do_power;

    /* Graphics Options */
    bool show_graphics; //Enable interactive graphics?
	int GraphPause;
	/* General options */
    bool show_help;
    bool show_version;
	bool timing_analysis;
    const char* SlackDefinition;
	bool CreateEchoFile;
    bool verify_file_digests;

    /* Atom netlist options */
	bool absorb_buffer_luts;
	bool sweep_dangling_primary_ios;
	bool sweep_dangling_nets;
	bool sweep_dangling_blocks;
	bool sweep_constant_primary_outputs;

	/* Clustering options */
	//bool global_clocks;
	//int cluster_size;
	//int inputs_per_cluster;
	//int lut_size;
	//bool hill_climbing_flag;
	//bool timing_driven;
	bool connection_driven_clustering;
	bool allow_unrelated_clustering;
	float alpha_clustering;
	float beta_clustering;
    bool timing_driven_clustering;
	e_cluster_seed cluster_seed_type;
	//int recompute_timing_after;
	//float block_delay;
	//float inter_cluster_net_delay;
	//bool skip_clustering;
	//e_packer_algorithm packer_algorithm;

	/* Placement options */
	int Seed;
    bool ShowPlaceTiming;
	float PlaceInnerNum;
	float PlaceInitT;
	float PlaceExitT;
	float PlaceAlphaT;
    sched_type anneal_sched_type; 
	e_place_algorithm PlaceAlgorithm;
    e_pad_loc_type pad_loc_type;
	int PlaceChanWidth;
	//float place_cost_exp;

	/* Timing-driven placement options only */
	float PlaceTimingTradeoff;
	int RecomputeCritIter;
	int inner_loop_recompute_divider;
	float place_exp_first;
	float place_exp_last;

	/* Router Options */
	int max_router_iterations;
	float first_iter_pres_fac;
	float initial_pres_fac;
	float pres_fac_mult;
	float acc_fac;
	int bb_factor;
	e_base_cost_type base_cost_type;
	float bend_cost;
	e_route_type RouteType;
	int RouteChanWidth;
	int min_route_chan_width_hint; //Hint to binary search router about what the min chan width is
    bool verify_binary_search;
	e_router_algorithm RouterAlgorithm;
	int min_incremental_reroute_fanout;

	//bool congestion_analysis;
	//bool fanout_analysis;
    //bool switch_usage_analysis;
	//bool TrimEmptyChan;
	//bool TrimObsChan;
	char*  write_rr_graph_file;
    char * read_rr_graph_file;

	/* Timing-driven router options only */
	float astar_fac;
	float max_criticality;
	float criticality_exp;
	e_routing_failure_predictor routing_failure_predictor;

	//float constant_net_delay;
    bool full_stats;
	bool Generate_Post_Synthesis_Netlist;

};

t_options read_options(int argc, const char** argv);

#endif
