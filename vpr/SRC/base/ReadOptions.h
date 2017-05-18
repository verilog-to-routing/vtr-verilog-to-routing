#ifndef READOPTIONS_H
#define READOPTIONS_H

#include "OptionTokens.h"

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
	char *dump_rr_structs_file;

	/* General options */
    bool show_version;
    bool show_graphics; //Enable interactive graphics?
	int GraphPause;
	float constant_net_delay;
	bool TimingAnalysis;
	bool CreateEchoFile;
	bool Generate_Post_Synthesis_Netlist;
    char SlackDefinition;

    /* Atom netlist options */
	bool absorb_buffer_luts;
	bool sweep_dangling_primary_ios;
	bool sweep_dangling_nets;
	bool sweep_dangling_blocks;
	bool sweep_constant_primary_outputs;

	/* Clustering options */
	bool global_clocks;
	int cluster_size;
	int inputs_per_cluster;
	int lut_size;
	bool hill_climbing_flag;
	bool timing_driven;
	enum e_cluster_seed cluster_seed_type;
	float alpha;
	float beta;
	int recompute_timing_after;
	float block_delay;
	float inter_cluster_net_delay;
	bool skip_clustering;
	bool allow_unrelated_clustering;
	bool connection_driven;
	enum e_packer_algorithm packer_algorithm;

	/* Placement options */
	enum e_place_algorithm PlaceAlgorithm;
	float PlaceInitT;
	float PlaceExitT;
	float PlaceAlphaT;
	float PlaceInnerNum;
	int Seed;
	float place_cost_exp;
	int PlaceChanWidth;
	char *PinFile;
	bool ShowPlaceTiming;

	/* Timing-driven placement options only */
	float PlaceTimingTradeoff;
	int RecomputeCritIter;
	int inner_loop_recompute_divider;
	float place_exp_first;
	float place_exp_last;

	/* Router Options */
	int max_router_iterations;
	int min_incremental_reroute_fanout;
	int bb_factor;
	bool congestion_analysis;
	bool fanout_analysis;
    bool switch_usage_analysis;
	float initial_pres_fac;
	float pres_fac_mult;
	float acc_fac;
	float first_iter_pres_fac;
	float bend_cost;
	enum e_route_type RouteType;
	int RouteChanWidth;
	int min_route_chan_width_hint; //Hint to binary search router about what the min chan width is
	bool TrimEmptyChan;
	bool TrimObsChan;
	enum e_router_algorithm RouterAlgorithm;
	enum e_base_cost_type base_cost_type;
	bool rr_graph_to_file;

	/* Timing-driven router options only */
	float astar_fac;
	float criticality_exp;
	float max_criticality;
	enum e_routing_failure_predictor routing_failure_predictor;

	/* State and metadata about various settings */
	int Count[OT_BASE_UNKNOWN+1];
	int Provenance[OT_BASE_UNKNOWN+1];
};

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


void ReadOptions(int argc,
		const char **argv,
		t_options * Options);

bool getEchoEnabled(void);
void setEchoEnabled(bool echo_enabled);

void setAllEchoFileEnabled(bool value);
void setEchoFileEnabled(enum e_echo_files echo_option, bool value);
void setEchoFileName(enum e_echo_files echo_option, const char *name);

bool isEchoFileEnabled(enum e_echo_files echo_option);
char *getEchoFileName(enum e_echo_files echo_option);

void alloc_and_load_echo_file_info();
void free_echo_file_info();

void setOutputFileName(enum e_output_files ename, const char *name, const char* default_name);
char *getOutputFileName(enum e_output_files ename);
void alloc_and_load_output_file_names(const char* default_name);
void free_output_file_names();

bool IsTimingEnabled(const t_options *Options);
bool IsEchoEnabled(const t_options *Options);

#endif
