typedef struct s_options t_options;
struct s_options
{
    char *ArchFile;
    char *NetFile;
    char *PlaceFile;
    char *RouteFile;

	/* General options */
	int GraphPause;
	float constant_net_delay;
    boolean TimingAnalysis;
    char *OutFilePrefix;

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

void ReadOptions(IN int argc,
		 IN char **argv,
		 OUT t_options * Options);
