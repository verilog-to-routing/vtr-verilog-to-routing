/* The order of this does NOT matter, but do not give things specific values
 * or you will screw up the ability to count things properly */
enum e_OptionBaseToken
{
    OT_NODISP,
    OT_AUTO,
    OT_RECOMPUTE_CRIT_ITER,
    OT_INNER_LOOP_RECOMPUTE_DIVIDER,
    OT_FIX_PINS,
    OT_FULL_STATS,
    OT_READ_PLACE_ONLY,
    OT_FAST,
    OT_TIMING_ANALYSIS,
    OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY,
    OT_INIT_T,
    OT_ALPHA_T,
    OT_EXIT_T,
    OT_INNER_NUM,
    OT_SEED,
    OT_PLACE_COST_EXP,
    OT_TD_PLACE_EXP_FIRST,
    OT_TD_PLACE_EXP_LAST,
    OT_PLACE_ALGORITHM,
    OT_TIMING_TRADEOFF,
    OT_ENABLE_TIMING_COMPUTATIONS,
    OT_BLOCK_DIST,
    OT_PLACE_COST_TYPE,
    OT_NUM_REGIONS,
    OT_PLACE_CHAN_WIDTH,
    OT_MAX_ROUTER_ITERATIONS,
    OT_BB_FACTOR,
    OT_ROUTER_ALGORITHM,
    OT_FIRST_ITER_PRES_FAC,
    OT_INITIAL_PRES_FAC,
    OT_PRES_FAC_MULT,
    OT_ACC_FAC,
    OT_ASTAR_FAC,
    OT_MAX_CRITICALITY,
    OT_CRITICALITY_EXP,
    OT_BASE_COST_TYPE,
    OT_BEND_COST,
    OT_ROUTE_TYPE,
    OT_ROUTE_CHAN_WIDTH,
    OT_ROUTE_ONLY,
    OT_PLACE_ONLY,
    OT_VERIFY_BINARY_SEARCH,
    OT_OUTFILE_PREFIX,
    OT_BASE_UNKNOWN		/* Must be last since used for counting enum items */
};


enum e_OptionArgToken
{
    OT_ON,
    OT_OFF,
    OT_RANDOM,
    OT_BOUNDING_BOX,
    OT_NET_TIMING_DRIVEN,
    OT_PATH_TIMING_DRIVEN,
    OT_LINEAR,
    OT_NONLINEAR,
    OT_BREADTH_FIRST,
    OT_TIMING_DRIVEN,
    OT_DIRECTED_SEARCH,
	OT_INTRINSIC_DELAY,
	OT_DELAY_NORMALIZED,
	OT_DEMAND_ONLY,
    OT_GLOBAL,
    OT_DETAILED,
    OT_ARG_UNKNOWN		/* Must be last since used for counting enum items */
};


extern const struct s_TokenPair OptionBaseTokenList[];
extern const struct s_TokenPair OptionArgTokenList[];
