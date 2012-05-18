#ifndef READOPTIONS_H
#define READOPTIONS_H

#include "OptionTokens.h"

typedef struct s_options t_options;
struct s_options
{
	/* File names */
    char *ArchFile;
    char *CircuitName;
    char *NetFile;
    char *PlaceFile;
    char *RouteFile;
	char *BlifFile;
	char *OutFilePrefix;

	/* General options */
	int GraphPause;
	float constant_net_delay;
    boolean TimingAnalysis;
    boolean CreateEchoFile;
    
	/* Clustering options */
	boolean global_clocks;
	int cluster_size;
	int inputs_per_cluster;
	int lut_size;
	boolean hill_climbing_flag;
	boolean sweep_hanging_nets_and_inputs;
	boolean timing_driven;
	enum e_cluster_seed cluster_seed_type;
	float alpha;
	float beta;
	int recompute_timing_after;
	float block_delay;
	float intra_cluster_net_delay;
	float inter_cluster_net_delay;
	boolean skip_clustering;
	boolean allow_unrelated_clustering;
	boolean allow_early_exit;
	boolean connection_driven;
	enum e_packer_algorithm packer_algorithm;

	/* Placement options */
    enum e_place_algorithm PlaceAlgorithm;
    float PlaceInitT;
    float PlaceExitT;
    float PlaceAlphaT;
    float PlaceInnerNum;
    int Seed;
	float place_cost_exp;
    enum place_c_types PlaceCostType;
    int PlaceChanWidth;
    int PlaceNonlinearRegions;
    char *PinFile;
    boolean ShowPlaceTiming;
    int block_dist;

	/* Timing-driven placement options only */
	float PlaceTimingTradeoff;
	int RecomputeCritIter;
	int inner_loop_recompute_divider;
	float place_exp_first;
	float place_exp_last;
    
	/* Router Options */
	int max_router_iterations;
	int bb_factor;
	float initial_pres_fac;
	float pres_fac_mult;
	float acc_fac;
	float first_iter_pres_fac;
	float bend_cost;
    enum e_route_type RouteType;
	int RouteChanWidth;
    enum e_router_algorithm RouterAlgorithm;
	enum e_base_cost_type base_cost_type;

	/* Timing-driven router options only */
	float astar_fac;
	float criticality_exp;
	float max_criticality;
	
    int Count[OT_BASE_UNKNOWN];
};

void ReadOptions(INP int argc,
		 INP char **argv,
		 OUTP t_options * Options);

boolean GetEchoOption(void);
void SetEchoOption(boolean echo_enabled);

#endif

