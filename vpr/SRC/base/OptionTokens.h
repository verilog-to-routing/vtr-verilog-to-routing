#ifndef OPTIONTOKENS_H
#define OPTIONTOKENS_H
/*
 * This enumeration is used to identify the different command line arguments
 * supplied to VPR.
 *
 * The ordering of this enum is important and used for error checking.  The OT__START_* and
 * OT__END_* items are used to delimit options that apply to specific stages (i.e. packing, 
 * placement, routing, analysis).
 *
 * If you have a general option that applies to more than one stage 
 * it should go in the general options section before the OT_PACK token.
 *
 * If you have a stage-specific option (i.e. it effects only that stage with no side effects for other
 * stages) it should go in the stage-specific section (e.g. between OT__START_PACK_OPTIONS and 
 * OT__END_PACK_OPTIONS for a packing-related option).  
 *
 * If an option only applies when timing-driven compilation is applied it should go in the timing driven 
 * options section for that stage (e.g. between OT__START_TIMING_PLACE_OPTIONS and 
 * OT__END_TIMING_PLACE_OPTIONS for an option only relevant during timing-driven placement).
 *
 * This layout allows for easy checking of stage-specific options by checking if an option
 * falls within a sections OT_START_ and OT_END_ items.  For example detecting (and warning 
 * the user) when an option will have no effect because the relevant stage is not being run (see 
 * CheckOptions.c for details).
 *
 * In general this layout supports specifying option groups in a tree-like structure, which
 * is encoded here as the tree's pre-order traversal, and made more explicit in the source 
 * code by intentation.
 */
enum e_OptionBaseToken {
	OT_NODISP,
	OT_AUTO,
	OT_GEN_NELIST_AS_BLIF,
	OT_CREATE_ECHO_FILE,
	OT_TIMING_ANALYSIS,
    OT_SLACK_DEFINITION,
	OT_OUTFILE_PREFIX,
	OT_BLIF_FILE,
	OT_NET_FILE,
	OT_PLACE_FILE,
	OT_ROUTE_FILE,
	OT_SDC_FILE,

    //Netlist processing related options
    OT_ABSORB_BUFFER_LUTS,
    OT_SWEEP_DANGLING_PRIMARY_IOS,
    OT_SWEEP_DANGLING_NETS,
    OT_SWEEP_DANGLING_BLOCKS,
    OT_SWEEP_CONSTANT_PRIMARY_OUTPUTS,

    //Packing options
	OT_PACK,
        OT__START_PACK_OPTIONS, //Marker
            OT_PACKER_ALGORITHM,
            OT_CONNECTION_DRIVEN_CLUSTERING,
            OT_ALLOW_UNRELATED_CLUSTERING,
            OT_ALPHA_CLUSTERING,
            OT_BETA_CLUSTERING,
            OT_CLUSTER_SEED,
            OT_HILL_CLIMBING_FLAG,
            OT_SKIP_CLUSTERING,
            OT_GLOBAL_CLOCKS,
            OT__START_TIMING_PACK_OPTIONS, //Marker
                OT_INTER_CLUSTER_NET_DELAY,
                OT_TIMING_DRIVEN_CLUSTERING,
            OT__END_TIMING_PACK_OPTIONS, //Marker
        OT__END_PACK_OPTIONS = OT__END_TIMING_PACK_OPTIONS, //Marker

    //Placement options
	OT_PLACE,
        OT__START_PLACE_OPTIONS, //Marker
            OT_PLACE_ALGORITHM,
            OT_INIT_T,
            OT_ALPHA_T,
            OT_EXIT_T,
            OT_INNER_NUM,
            OT_INNER_LOOP_RECOMPUTE_DIVIDER,
            OT_RECOMPUTE_CRIT_ITER,
            OT_SEED,
            OT_PLACE_COST_EXP,
            OT_PLACE_CHAN_WIDTH,
            OT_FIX_PINS,
            OT_READ_PLACE_ONLY,
            OT__START_TIMING_PLACE_OPTIONS, //Marker
                OT_TD_PLACE_EXP_FIRST,
                OT_TD_PLACE_EXP_LAST,
                OT_TIMING_TRADEOFF,
                OT_ENABLE_TIMING_COMPUTATIONS,
            OT__END_PLACE_OPTIONS, //Marker
        OT__END_TIMING_PLACE_OPTIONS = OT__END_PLACE_OPTIONS, //Marker

    //Router options
	OT_ROUTE,
        OT__START_ROUTE_OPTIONS, //Marker
            OT_MAX_ROUTER_ITERATIONS,
            OT_MIN_INCREMENTAL_REROUTE_FANOUT,
            OT_BB_FACTOR,
            OT_ROUTER_ALGORITHM,
            OT_FIRST_ITER_PRES_FAC,
            OT_INITIAL_PRES_FAC,
            OT_PRES_FAC_MULT,
            OT_ACC_FAC,
            OT_ASTAR_FAC,
            OT_BASE_COST_TYPE,
            OT_BEND_COST,
            OT_ROUTE_TYPE,
            OT_ROUTE_CHAN_WIDTH,
            OT_MIN_ROUTE_CHAN_WIDTH_HINT,
            OT_TRIM_EMPTY_CHAN,
            OT_TRIM_OBS_CHAN,
            OT_VERIFY_BINARY_SEARCH,
            OT_ROUTING_FAILURE_PREDICTOR,
            OT_DUMP_RR_STRUCTS_FILE,
            OT_CONGESTION_ANALYSIS,
            OT_FANOUT_ANALYSIS,
            OT_SWITCH_USAGE_ANALYSIS,
            OT_FULL_STATS,
            OT_FAST,
            OT__START_TIMING_ROUTE_OPTIONS, //Marker
                OT_MAX_CRITICALITY,
                OT_CRITICALITY_EXP,
            OT__END_TIMING_ROUTE_OPTIONS, //Marker
        OT__END_ROUTE_OPTIONS = OT__END_TIMING_ROUTE_OPTIONS, //Marker

    //Analysis options (post routing)
    OT_ANALYSIS,
        OT__START_ANALYSIS_OPTIONS, //Marker
            OT_GENERATE_POST_SYNTHESIS_NETLIST,
            OT_POWER,
            OT__START_ANALYSIS_POWER_OPTIONS, //Marker
                OT_ACTIVITY_FILE,
                OT_POWER_OUT_FILE,
                OT_CMOS_TECH_BEHAVIOR_FILE,
            OT__END_ANALYSIS_POWER_OPTIONS, //Marker
        OT__END_ANALYSIS_OPTIONS, //Marker
    OT_BASE_UNKNOWN = OT__END_ANALYSIS_OPTIONS //Marker
};

enum e_OptionArgToken {
	OT_ON,
	OT_OFF,
	OT_RANDOM,
	OT_BOUNDING_BOX,
	OT_PATH_TIMING_DRIVEN,
	OT_BREADTH_FIRST,
	OT_TIMING_DRIVEN,
	OT_NO_TIMING,
	OT_DELAY_NORMALIZED,
	OT_DEMAND_ONLY,
	OT_GLOBAL,
	OT_DETAILED,
	OT_TIMING,
	OT_MAX_INPUTS,
	OT_BLEND,
	OT_GREEDY,
	OT_LP,
	OT_BRUTE_FORCE,
	OT_ROUTING_FAILURE_SAFE,
	OT_ROUTING_FAILURE_AGGRESSIVE,
	OT_ARG_UNKNOWN /* Must be last since used for counting enum items */
};

extern struct s_TokenPair OptionBaseTokenList[];
extern struct s_TokenPair OptionArgTokenList[];

#endif
